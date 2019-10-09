//
// Created by Christopher Yarp on 7/6/18.
//

#include "Sum.h"
#include "GraphCore/NodeFactory.h"
#include "General/GeneralHelper.h"
#include "General/ErrorHelpers.h"

Sum::Sum() {

}

Sum::Sum(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent) {

}

std::vector<bool> Sum::getInputSign() const {
    return inputSign;
}

void Sum::setInputSign(const std::vector<bool> &inputSign) {
    Sum::inputSign = inputSign;
}

std::shared_ptr<Sum> Sum::createFromGraphML(int id, std::string name,
                                            std::map<std::string, std::string> dataKeyValueMap,
                                            std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<Sum> newNode = NodeFactory::createNode<Sum>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important property ====
    std::string inputSigns;

    if(dialect == GraphMLDialect::VITIS){
        //Vitis Name -- InputSigns
        inputSigns = dataKeyValueMap.at("InputSigns");
    } else if(dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Name -- Inputs
        inputSigns = dataKeyValueMap.at("Inputs");
    } else
    {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Sum", newNode));
    }

    //There are multiple cases for inputs.  One is a string of + or - signs.  The other is a number.
    std::vector<bool> signs;

    if(inputSigns.empty()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Empty Inputs parameter passed to Sum", newNode));
    }else if(inputSigns[0] == '+' || inputSigns[0] == '-' || inputSigns[0] == '|'){
        //An array of +,-,|
        unsigned long inputLength = inputSigns.size();
        for(unsigned long i = 0; i<inputLength; i++){
            if(inputSigns[i] == '+'){
                signs.push_back(true);
            }else if(inputSigns[i] == '-'){
                signs.push_back(false);
            }else if(inputSigns[i] == '|'){
                //This is is a placeholder character that changes the position of the ports in the GUI but does not effect their numbering
            }else{
                throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown format for Sum Input Parameter", newNode));
            }
        }
    }else{
        //Parmater is a number
        int numInputs = std::stoi(inputSigns);

        for(int i = 0; i<numInputs; i++){
            signs.push_back(true);
        }
    }

    newNode->setInputSign(signs);

    return newNode;
}

std::set<GraphMLParameter> Sum::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("InputSigns", "string", true));

    return parameters;
}

xercesc::DOMElement *
Sum::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Sum");

    GraphMLHelper::addDataNode(doc, thisNode, "InputSigns", GeneralHelper::vectorToString(inputSign, "+", "-", "", false));

    return thisNode;
}

std::string Sum::typeNameStr(){
    return "Sum";
}

std::string Sum::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() + "\nInputOps:" + GeneralHelper::vectorToString(inputSign, "+", "-", "", false);

    return label;
}

void Sum::validate() {
    Node::validate();

    if(inputPorts.size() < 2){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Sum - Should Have 2 or More Input Ports", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Sum - Should Have Exactly 1 Output Port", getSharedPointer()));
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
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Sum - An Input Port is Complex but Output is Real", getSharedPointer()));
        }
    }

    if(inputSign.size() != inputPorts.size()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Sum - The number of signs (" + GeneralHelper::to_string(inputSign.size()) + ") does not match the number of inputs (" + GeneralHelper::to_string(inputPorts.size()) + ")", getSharedPointer()));
    }
}

CExpr Sum::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    //TODO: Implement Vector Support
    if(getOutputPort(0)->getDataType().getWidth()>1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("C Emit Error - Sum Support for Vector Types has Not Yet Been Implemented", getSharedPointer()));
    }

    //Get the expressions for each input
    std::vector<std::string> inputExprs;

    unsigned long numInputPorts = inputPorts.size();
    for(unsigned long i = 0; i<numInputPorts; i++){
        std::shared_ptr<OutputPort> srcOutputPort = getInputPort(i)->getSrcOutputPort();
        int srcOutputPortNum = srcOutputPort->getPortNum();
        std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

        inputExprs.push_back(srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, imag));
    }

    //Check if any of the inputs are floating point & if so, find the largest
    //Also check for any fixed point types.  Find the max integer type
    bool foundFloat = false;
    DataType largestFloat;
    bool foundFixedPt = false;
    bool foundInt = false;
    DataType largestInt;


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
            if(foundInt == false){
                largestInt = portDataType;
                foundInt = true;
            }else{
                if(largestInt.getTotalBits() < portDataType.getTotalBits()){
                    largestInt = portDataType;
                }
            }
        }
    }

    //TODO: Implement Fixed Point

    if(!foundFixedPt){
        DataType accumType;
        if(foundFloat){
            accumType = largestFloat; //Floating point types do not grow
        }else{
            //Integer
            accumType = largestInt;
            accumType.setTotalBits(accumType.getTotalBits()+numInputPorts-1); //Grow 1 bit per input.  Since this is a promotion, masking will not occur
        }

        //floating point numbers
        std::string expr = DataType::cConvertType(inputExprs[0], getInputPort(0)->getDataType(), accumType);

        if(!inputSign[0]){
            expr = "(-(" + expr + "))";
        }else{
            expr = "(" + expr + ")";
        }

        for(unsigned long i = 1; i<numInputPorts; i++) {
            if (inputSign[i]) {
                expr += "+";
            } else {
                expr += "-";
            }
            expr += "(" + DataType::cConvertType(inputExprs[i], getInputPort(i)->getDataType(), accumType) + ")";
        }

        expr = DataType::cConvertType(expr, accumType, getOutputPort(0)->getDataType());//Convert to output if nessisary

        return CExpr(expr, false);
    }
    else{
        //TODO: Fixed Point Support
        throw std::runtime_error(ErrorHelpers::genErrorStr("C Emit Error - Fixed Point Not Yet Implemented for Sum", getSharedPointer()));
    }

    return CExpr("", false);
}

Sum::Sum(std::shared_ptr<SubSystem> parent, Sum* orig) : PrimitiveNode(parent, orig), inputSign(orig->inputSign){

}

std::shared_ptr<Node> Sum::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Sum>(parent, this);
}
