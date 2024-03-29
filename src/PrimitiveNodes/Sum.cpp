//
// Created by Christopher Yarp on 7/6/18.
//

#include "Sum.h"
#include "GraphCore/NodeFactory.h"
#include "General/GeneralHelper.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"

Sum::Sum() : collapseDimension(-1) {

}

Sum::Sum(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), collapseDimension(-1) {

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

        //Import Collapse Dim
        std::string collapseDimensionStr = dataKeyValueMap.at("CollapseDimension");
        int collapseDimension = std::stoi(collapseDimensionStr);
        newNode->setCollapseDimension(collapseDimension);

    } else if(dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Name -- Inputs
        inputSigns = dataKeyValueMap.at("Inputs");

        //For legacy support, CollapseMode may not be exported.  In this case, default to All Dimensions
        std::string collapseMode;

        auto collapseModeEntry = dataKeyValueMap.find("CollapseMode");
        if(collapseModeEntry != dataKeyValueMap.end()) {
            collapseMode = dataKeyValueMap.at("CollapseMode");
        }else{
            collapseMode = "All dimensions";
        }

        if(collapseMode == "All dimensions"){
            newNode->setCollapseDimension(-1);
        }else if(collapseMode == "Specified dimension"){
            std::string collapseDimensionStr = dataKeyValueMap.at("Numeric.CollapseDim");
            int collapseDimension = std::stoi(collapseDimensionStr);
            collapseDimension -= 1; //Due to Matlab/Simulink index from 1
            newNode->setCollapseDimension(collapseDimension);
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown CollapseMode: " + collapseMode, newNode));
        }
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
        //Parameter is a number
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
    parameters.insert(GraphMLParameter("CollapseDimension", "int", true));

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

    GraphMLHelper::addDataNode(doc, thisNode, "CollapseDimension", GeneralHelper::to_string(collapseDimension));

    return thisNode;
}

std::string Sum::typeNameStr(){
    return "Sum";
}

std::string Sum::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() +
             "\nInputOps: " + GeneralHelper::vectorToString(inputSign, "+", "-", "", false) +
             "\nCollapseDimension: " + GeneralHelper::to_string(collapseDimension);

    return label;
}

void Sum::validate() {
    Node::validate();

    if(collapseDimension < 0) {
        if (inputPorts.size() < 2) {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Sum - Should Have 2 or More Input Ports when Summing Across All Dimensions",getSharedPointer()));
        }
    }else{
        if (inputPorts.size() != 1) {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Sum - Should Have 1 Input Ports When Summing Across a Specified Dimension",getSharedPointer()));
        }
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Sum - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    if(collapseDimension < 0) {
        //Check that the input dimensions are the same as the output dimensions or are scalar
        std::vector<int> outputDim = outputPorts[0]->getDataType().getDimensions();
        for (unsigned long i = 0; i < inputPorts.size(); i++) {
            DataType inputDT = inputPorts[i]->getDataType();

            //If the input is a scalar, do not check the dimensions
            //Otherwise, check that the dimensions match
            if (!inputDT.isScalar()) {
                std::vector<int> inputDim = inputPorts[i]->getDataType().getDimensions();
                if (inputDim != outputDim) {
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "Validation Failed - Sum - The dimensions of input port " + GeneralHelper::to_string(i) +
                            " did not match the output", getSharedPointer()));
                }
            }
        }
    }else{
        //The sum is collapsing along the specified dimension.  The given dimension should be reduced to 1
        std::vector<int> outputDim = outputPorts[0]->getDataType().getDimensions();
        for (unsigned long i = 0; i < inputPorts.size(); i++) {
            DataType inputDT = inputPorts[i]->getDataType();

            //If the input is a scalar, do not check the dimensions
            //Otherwise, check that the dimensions match
            if (inputDT.isScalar()) {
                //Needs to be summed across dimension 0.  Actually does nothing since there are no elements to collapse
                if(collapseDimension != 0){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Collapse dimension must be 0 for scalar input", getSharedPointer()));
                }

            }else{
                std::vector<int> inputDim = inputPorts[i]->getDataType().getDimensions();

                if(collapseDimension >= inputDim.size()-1){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Collapse Dimension must be one of the dimensions present in the input", getSharedPointer()));
                }

                //Check that the specified dimension is reduced to 1 with all dimensions remaining the same
                inputDim[collapseDimension] = 1;
                std::vector<int> outputDim = outputPorts[0]->getDataType().getDimensions();
                if(inputDim != outputDim){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Output dimensions should be the same as the input dimensions except that the collapsed dimension should be reduced to 1", getSharedPointer()));
                }

            }
        }
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
    //Get the expressions for each input
    std::vector<CExpr> inputExprs;

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
    bool foundSignedInt = false;
    DataType largestInt;

    for(unsigned long i = 0; i<numInputPorts; i++){
        DataType portDataType = getInputPort(i)->getDataType();
        if(portDataType.isFloatingPt()){
            if(!foundFloat){
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
            if(!foundInt){
                largestInt = portDataType;
                foundInt = true;
            }else{
                if(largestInt.getTotalBits() < portDataType.getTotalBits()){
                    largestInt = portDataType;
                }
            }
            if(portDataType.isSignedType()){
                foundSignedInt = true;
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

            if(foundSignedInt) {
                if(collapseDimension < 0) {
                    //Not collapsing dimensions, thereforw, grow based on number of inputs
                    accumType.setTotalBits(accumType.getTotalBits() + ((int) ceil(
                            log2(numInputPorts)))); //Grow by the log2 of the number of inputs.  This is actually a little conservative
                }else{
                    std::vector<int> inputDim = inputPorts[0]->getDataType().getDimensions();
                    accumType.setTotalBits(accumType.getTotalBits() + ((int) ceil(
                            log2(inputDim[collapseDimension])))); //Grow by the log2 of the number of inputs.  This is actually a little conservative
                }
            }else{
                if(collapseDimension < 0) {
                    //If unsigned, only grow for positive inputs
                    int numAdds = 0;
                    for (bool op : inputSign) {
                        if (op) {
                            numAdds++;
                        }
                    }
                    accumType.setTotalBits(accumType.getTotalBits() + ((int) ceil(log2(numAdds))));
                }else{
                    std::vector<int> inputDim = inputPorts[0]->getDataType().getDimensions();
                    accumType.setTotalBits(accumType.getTotalBits() + ((int) ceil(log2(inputDim[collapseDimension]))));
                }
            }
        }

        //floating point numbers

        //TODO: Possibly introduce an explicit vector intrinsic version or a version utilizing a linear alg library

        DataType outputDT = getOutputPort(0)->getDataType();

        if(collapseDimension < 0) {
            //==== No Collapsing ====
            Variable vecOutVar;
            std::vector<std::string> forLoopIndexVars;
            std::vector<std::string> forLoopClose;
            //If the output is a vector, construct a for loop which puts the results in a temporary array
            if (!outputDT.isScalar()) {
                //Declare tmp array and write to file
                std::string vecOutName = name + "_n" + GeneralHelper::to_string(id) + "_outVec"; //Changed to
                vecOutVar = Variable(vecOutName, outputDT);
                cStatementQueue.push_back(vecOutVar.getCVarDecl(imag, true, false, true, false) + ";");

                //Create nested loops for a given array
                std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                        EmitterHelpers::generateVectorMatrixForLoops(outputDT.getDimensions());

                std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
                forLoopIndexVars = std::get<1>(forLoopStrs);
                forLoopClose = std::get<2>(forLoopStrs);

                cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());
            }

            std::string expr = "";

            for (unsigned long i = 0; i < numInputPorts; i++) {
                if (i == 0) {
                    //Handle if first element is negated
                    if (!inputSign[0]) {
                        expr = "(-";
                    }
                } else {
                    if (inputSign[i]) {
                        expr += "+";
                    } else {
                        expr += "-";
                    }
                }

                //if a vector/matrix type, need to dereference the input expr
                std::vector<std::string> emptyArr;
                std::string inputExprDeref = inputExprs[i].getExprIndexed(
                        getInputPort(i)->getDataType().isScalar() ? emptyArr : forLoopIndexVars, true);

                expr += "(" + DataType::cConvertType(inputExprDeref, getInputPort(i)->getDataType(), accumType) + ")";

                //Finish handling if the first element is negated
                if (i == 0 && !inputSign[0]) {
                    expr += ")";
                }
            }

            //Cast the expression to the correct output type (if necessary)
            expr = DataType::cConvertType(expr, accumType, getOutputPort(0)->getDataType());

            //if a vector, turn the expression into an assignment and write it to the file.  Also close the for loop
            if (!outputDT.isScalar()) {
                cStatementQueue.push_back(
                        vecOutVar.getCVarName(imag) + EmitterHelpers::generateIndexOperation(forLoopIndexVars) + " = " +
                        expr + ";");

                //Close for loop
                cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());

                //If a vector, return the temporary variable and flag it as a variable.
                return CExpr(vecOutVar.getCVarName(imag), CExpr::ExprType::ARRAY);
            } else {
                //If a scalar, just return the expression
                return CExpr(expr, CExpr::ExprType::SCALAR_EXPR);
            }
        }else{
            //====Collapse Along Dimension ====
            if (outputDT.isScalar()) {
                //Nothing really to do here
                //The input only had 1 dimension and all
                return inputExprs[0];
            }else{
                Variable matAccumVar;
                std::vector<std::string> forLoopIndexVars;
                std::vector<std::string> forLoopClose;

                //Declare tmp array and write to file
                accumType.setDimensions(outputDT.getDimensions());
                std::string matAccumName = name + "_n" + GeneralHelper::to_string(id) + "_AccumMat";
                matAccumVar = Variable(matAccumName, accumType);
                //Set elements to 0
                std::vector<NumericValue> initConds = {NumericValue(0, 0, std::complex<double>(0, 0), outputDT.isComplex(), outputDT.isFloatingPt())};
                matAccumVar.setInitValue(initConds);
                cStatementQueue.push_back(matAccumVar.getCVarDecl(imag, true, true, true, false) + ";");

                //Create nested loops for a given array
                //We do this for the input type (which has not yet been collapsed)
                DataType inputDT = getInputPort(0)->getDataType();

                std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                        EmitterHelpers::generateVectorMatrixForLoops(inputDT.getDimensions());

                std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
                forLoopIndexVars = std::get<1>(forLoopStrs);
                forLoopClose = std::get<2>(forLoopStrs);

                cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());

                //Perform the accumulation across the dimension

                //if a vector/matrix type, need to dereference the input expr
                std::string inputExprDeref = inputExprs[0].getExprIndexed(forLoopIndexVars, true);
                std::vector<std::string> forLoopIndexAssign = forLoopIndexVars;
                //Accumulator has a constant index in the collaps dimension
                forLoopIndexAssign[collapseDimension] = GeneralHelper::to_string((long) 0);
                std::string accumAssignExpr = matAccumVar.getCVarName(imag) + EmitterHelpers::generateIndexOperation(forLoopIndexAssign) + " += " + inputExprDeref + ";";
                cStatementQueue.push_back(accumAssignExpr);
                cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());

                //If the output type is different, declare an output array of the appropriate type
                //Copy to relevant type if accumulator type is different from output type
                if(outputDT != accumType){
                    std::string matOutputName = name + "_n" + GeneralHelper::to_string(id) + "_OutMat";
                    Variable matOutputVar = Variable(matOutputName, outputDT);
                    cStatementQueue.push_back(matOutputVar.getCVarDecl(imag, true, true, true, false) + ";");

                    std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> outputForLoopStrs =
                            EmitterHelpers::generateVectorMatrixForLoops(inputDT.getDimensions());

                    std::vector<std::string> outputForLoopOpen = std::get<0>(outputForLoopStrs);
                    std::vector<std::string> outputForLoopIndexVars = std::get<1>(outputForLoopStrs);
                    std::vector<std::string> outputForLoopClose = std::get<2>(outputForLoopStrs);

                    cStatementQueue.insert(cStatementQueue.end(), outputForLoopOpen.begin(), outputForLoopOpen.end());
                    std::string outAssignExpr = matOutputVar.getCVarName(imag) + EmitterHelpers::generateIndexOperation(outputForLoopIndexVars) + " = " + matAccumVar.getCVarName(imag) + EmitterHelpers::generateIndexOperation(outputForLoopIndexVars) + ";";
                    cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());

                    return CExpr(matOutputVar.getCVarName(imag), CExpr::ExprType::ARRAY);
                }else{
                    return CExpr(matAccumVar.getCVarName(imag), CExpr::ExprType::ARRAY);
                }
            }
        }
    }
    else{
        //TODO: Fixed Point Support
        throw std::runtime_error(ErrorHelpers::genErrorStr("C Emit Error - Fixed Point Not Yet Implemented for Sum", getSharedPointer()));
    }

    return CExpr("", CExpr::ExprType::SCALAR_EXPR);
}

Sum::Sum(std::shared_ptr<SubSystem> parent, Sum* orig) : PrimitiveNode(parent, orig), inputSign(orig->inputSign), collapseDimension(orig->collapseDimension){

}

std::shared_ptr<Node> Sum::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Sum>(parent, this);
}

int Sum::getCollapseDimension() const {
    return collapseDimension;
}

void Sum::setCollapseDimension(int collapseDimension) {
    Sum::collapseDimension = collapseDimension;
}
