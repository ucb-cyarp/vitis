//
// Created by Christopher Yarp on 7/6/18.
//

#include "Product.h"
#include "GraphCore/NodeFactory.h"
#include "GraphMLTools/GraphMLHelper.h"
#include "General/GeneralHelper.h"

Product::Product() {

}

Product::Product(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent) {

}

std::vector<bool> Product::getInputOp() const {
    return inputOp;
}

void Product::setInputOp(const std::vector<bool> &inputOp) {
    Product::inputOp = inputOp;
}

std::shared_ptr<Product> Product::createFromGraphML(int id, std::string name,
                                                    std::map<std::string, std::string> dataKeyValueMap,
                                                    std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<Product> newNode = NodeFactory::createNode<Product>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important property -- Inputs ====
    std::string inputOperations;

    if(dialect == GraphMLDialect::VITIS){
        //Vitis Name -- InputOps
        inputOperations = dataKeyValueMap.at("InputOps");
    } else if(dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Name -- Inputs
        inputOperations = dataKeyValueMap.at("Inputs");
    } else
    {
        throw std::runtime_error("Unsupported Dialect when parsing XML - Product");
    }

    //There are multiple cases for inputs.  One is a string of * or /.  The other is a number.
    std::vector<bool> ops;

    if(inputOperations.empty()){
        throw std::runtime_error("Empty Inputs parameter passed to Product");
    }else if(inputOperations[0] == '*' || inputOperations[0] == '/' || inputOperations[0] == '|'){
        //An array of *,/
        unsigned long inputLength = inputOperations.size();
        for(unsigned long i = 0; i<inputLength; i++){
            if(inputOperations[i] == '*'){
                ops.push_back(true);
            }else if(inputOperations[i] == '/'){
                ops.push_back(false);
            }else if(inputOperations[i] == '|'){
                //This is is a placeholder character that changes the position of the ports in the GUI but does not effect their numbering
            }else{
                throw std::runtime_error("Unknown format for Product Input Parameter");
            }
        }
    }else{
        //Parmater is a number
        int numInputs = std::stoi(inputOperations);

        for(int i = 0; i<numInputs; i++){
            ops.push_back(true);
        }
    }

    newNode->setInputOp(ops);

    return newNode;
}

std::set<GraphMLParameter> Product::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("InputOps", "string", true));

    return parameters;
}

xercesc::DOMElement *
Product::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Product");

    GraphMLHelper::addDataNode(doc, thisNode, "InputOps", GeneralHelper::vectorToString(inputOp, "*", "/", "", false));

    return thisNode;
}

std::string Product::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: Product\nInputOps:" + GeneralHelper::vectorToString(inputOp, "*", "/", "", false);

    return label;
}

void Product::validate() {
    Node::validate();

    if(inputPorts.size() < 2){
        throw std::runtime_error("Validation Failed - Product - Should Have 2 or More Input Ports");
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - Product - Should Have Exactly 1 Output Port");
    }

    //Check that if any input is complex, the result is complex
    unsigned long numInputPorts = inputPorts.size();
    bool foundComplex = false;

    for(unsigned long i = 0; i<numInputPorts; i++){
        DataType inType = getInputPort(i)->getDataType();
        if(inType.isComplex()){
            foundComplex = true;
            break;
        }

    }

    if(foundComplex) {
        DataType outType = getOutputPort(0)->getDataType();
        if(!outType.isComplex()){
            throw std::runtime_error("Validation Failed - Product - An Input Port is Complex but Output is Real");
        }
    }

    if(inputOp.size() != inputPorts.size()){
        throw std::runtime_error("Validation Failed - Product - The number of operators (" + std::to_string(inputOp.size()) + ") does not match the number of inputs (" + std::to_string(inputPorts.size()) + ")");
    }
}

CExpr Product::emitCExpr(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag) {
    //TODO: Implement Vector Support
    if(getOutputPort(0)->getDataType().getWidth()>1){
        throw std::runtime_error("C Emit Error - Product Support for Vector Types has Not Yet Been Implemented");
    }

    //TODO: Implement Complex Support
    if(getOutputPort(0)->getDataType().isComplex() || getInputPort(1)->getDataType().isComplex()){
        throw std::runtime_error("C Emit Error - Product Support for Complex has Not Yet Been Implemented");
    }

    //Get the expressions for each input
    std::vector<std::string> inputExprs;

    unsigned long numInputPorts = inputPorts.size();
    for(unsigned long i = 0; i<numInputPorts; i++){
        std::shared_ptr<OutputPort> srcOutputPort = getInputPort(i)->getSrcOutputPort();
        int srcOutputPortNum = srcOutputPort->getPortNum();
        std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

        inputExprs.push_back(srcNode->emitC(cStatementQueue, srcOutputPortNum, imag));
    }

    //Check if any of the inputs are floating point & if so, find the largest
    //Also check for any fixed point types.  Find the integer final width
    bool foundFloat = false;
    DataType largestFloat;
    bool foundFixedPt = false;
    unsigned long intFinalWidth = 0;


    for(unsigned long i = 0; i<numInputPorts; i++){
        DataType portDataType = getInputPort(i)->getDataType();
        if(portDataType.isFloatingPt()){
            if(foundFloat == false){
                foundFloat = true;
                largestFloat = portDataType;
            }else{
                if(largestFloat.getTotalBits() < portDataType.getTotalBits()){
                    largestFloat = portDataType;
                }
            }
        }else if(portDataType.getFractionalBits() != 0){
            foundFixedPt = true;
        }else{
            //Integer
            intFinalWidth += portDataType.getTotalBits(); //For multiply, bit growth is the sum of
        }
    }

    if(!foundFixedPt){
        DataType intermediateType;
        if(foundFloat){
            intermediateType = largestFloat; //Floating point types do not grow
        }else{
            //Integer
            intermediateType = getInputPort(0)->getDataType(); //Get the base datatype of the 1st input to modify (we did not find a float or fixed pt so this is an int)
            intermediateType.setTotalBits(intFinalWidth); //Since this is a promotion, masking will not occur in the datatype convert
        };

        //floating point numbers
        std::string expr = DataType::cConvertType(inputExprs[0], getInputPort(0)->getDataType(), intermediateType);

        if(!inputOp[0]){
            expr = "(( (" + intermediateType.toString(DataType::StringStyle::C, false) + " ) 1.0)/(" + expr + "))";
        }else{
            expr = "(" + expr + ")";
        }

        for(unsigned long i = 1; i<numInputPorts; i++) {
            if (inputOp[i]) {
                expr += "*";
            } else {
                expr += "/";
            }
            expr += "(" + DataType::cConvertType(inputExprs[i], getInputPort(i)->getDataType(), intermediateType) + ")";
        }

        expr = DataType::cConvertType(expr, intermediateType, getOutputPort(0)->getDataType());//Convert to output if nessisary

        return CExpr(expr, false);
    }
    else{
        //TODO: Finish
        throw std::runtime_error("C Emit Error - Fixed Point Not Yet Implemented for Product");
    }

    return CExpr("", false);
}

Product::Product(std::shared_ptr<SubSystem> parent, std::shared_ptr<Product> orig) : PrimitiveNode(parent, orig), inputOp(orig->inputOp){

}
