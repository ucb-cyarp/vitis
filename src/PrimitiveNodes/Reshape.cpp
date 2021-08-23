//
// Created by Christopher Yarp on 6/2/21.
//

#include "Reshape.h"
#include "General/ErrorHelpers.h"

Reshape::ReshapeMode Reshape::getMode() const {
    return mode;
}

void Reshape::setMode(Reshape::ReshapeMode mode) {
    Reshape::mode = mode;
}

std::vector<int> Reshape::getTargetDimensions() const {
    return targetDimensions;
}

void Reshape::setTargetDimensions(const std::vector<int> &targetDimensions) {
    Reshape::targetDimensions = targetDimensions;
}

Reshape::ReshapeMode Reshape::parseReshapeMode(std::string str) {
    if(str == "VEC_1D"){
        return ReshapeMode::VEC_1D;
    }else if(str == "ROW_VEC"){
        return ReshapeMode::ROW_VEC;
    }else if(str == "COL_VEC"){
        return ReshapeMode::COL_VEC;
    }else if(str == "MANUAL"){
        return ReshapeMode::MANUAL;
    }else if(str == "REF_INPUT"){
        return ReshapeMode::REF_INPUT;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown ReshapeMode: " + str));
    }
}

std::string Reshape::reshapeModeToStr(Reshape::ReshapeMode mode) {
    switch (mode) {
        case ReshapeMode::VEC_1D:
            return "VEC_1D";
        case ReshapeMode::ROW_VEC:
            return "ROW_VEC";
        case ReshapeMode::COL_VEC:
            return "COL_VEC";
        case ReshapeMode::MANUAL:
            return "MANUAL";
        case ReshapeMode::REF_INPUT:
            return "REF_INPUT";
        default:
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown ReshapeMode"));
    }
}

Reshape::Reshape() : mode(ReshapeMode::VEC_1D) {

}

Reshape::Reshape(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), mode(ReshapeMode::VEC_1D) {

}

Reshape::Reshape(std::shared_ptr<SubSystem> parent, Reshape *orig) : PrimitiveNode(parent, orig), mode(orig->mode), targetDimensions(orig->targetDimensions) {

}

std::shared_ptr<Reshape>
Reshape::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                           std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<Reshape> newNode = NodeFactory::createNode<Reshape>(parent);
    newNode->setId(id);
    newNode->setName(name);

    ReshapeMode mode;
    std::string targetDimName;
    if(dialect == GraphMLDialect::VITIS){
        std::string modeStr = dataKeyValueMap.at("Mode");
        mode = parseReshapeMode(modeStr);

        targetDimName = "TargetDimensions";
    } else if(dialect == GraphMLDialect::SIMULINK_EXPORT) {
        std::string modeStr = dataKeyValueMap.at("OutputDimensionality");
        if(modeStr == "1-D array"){
            mode = ReshapeMode::VEC_1D;
        }else if(modeStr == "Column vector (2-D)"){
            mode = ReshapeMode::COL_VEC;
        }else if(modeStr == "Row vector (2-D)"){
            mode = ReshapeMode::ROW_VEC;
        }else if(modeStr == "Customize"){
            mode = ReshapeMode::MANUAL;
        }else if(modeStr == "Derive from reference input port"){
            mode = ReshapeMode::REF_INPUT;
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown OutputDimensionality when Importing From Simulink", newNode));
        }

        targetDimName = "Numeric.OutputDimensions";
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Reshape", newNode));
    }

    std::string targetDimsStr = dataKeyValueMap.at(targetDimName);
    std::vector<NumericValue> targetDimsNumericVal = NumericValue::parseXMLString(targetDimsStr);

    std::vector<int> targetDimensions;
    for(int i = 0; i<targetDimsNumericVal.size(); i++){
        if(targetDimsNumericVal[i].isComplex() || targetDimsNumericVal[i].isFractional()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Target dimension is expected to be composed of real integers"));
        }
        targetDimensions.push_back((int) targetDimsNumericVal[i].getRealInt());
    }

    newNode->setMode(mode);
    newNode->setTargetDimensions(targetDimensions);

    return newNode;
}

std::set<GraphMLParameter> Reshape::graphMLParameters() {
    std::set<GraphMLParameter> parameters;
    parameters.insert(GraphMLParameter("Mode", "string", true));
    parameters.insert(GraphMLParameter("TargetDimensions", "string", true));

    return parameters;
}

std::string Reshape::typeNameStr() {
    return "Reshape";
}

xercesc::DOMElement *
Reshape::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Reshape");

    GraphMLHelper::addDataNode(doc, thisNode, "Mode", reshapeModeToStr(mode));

    std::string targetDimStr = "[";
    for(int i = 0; i<targetDimensions.size(); i++){
        if(i>0){
            targetDimStr += ",";
        }
        targetDimStr += GeneralHelper::to_string(targetDimensions[i]);
    }
    targetDimStr += "]";

    GraphMLHelper::addDataNode(doc, thisNode, "TargetDimensions", targetDimStr);

    return thisNode;
}

std::string Reshape::labelStr() {
    std::string label = Node::labelStr();

    std::string targetDimStr = "[";
    for(int i = 0; i<targetDimensions.size(); i++){
        if(i>0){
            targetDimStr += ",";
        }
        targetDimStr += GeneralHelper::to_string(targetDimensions[i]);
    }
    targetDimStr += "]";

    label += "\nFunction: " + typeNameStr() +
             "\nMode: " + reshapeModeToStr(mode) +
             "\nTargetDimensions: " + targetDimStr;

    return label;
}

std::shared_ptr<Node> Reshape::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Reshape>(parent, this);
}

void Reshape::propagateProperties() {
    if(inputPorts.empty()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Expected at least 1 Port for Reshape", getSharedPointer()));
    }

    if(mode == ReshapeMode::REF_INPUT){
        //Get the target dimensions from the second input
        if(inputPorts.size() < 2) {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Expected 2 Ports for Reshape when Mode is REF_INPUT",getSharedPointer()));
        }
            targetDimensions = getInputPort(1)->getDataType().getDimensions();
    }else if(mode == ReshapeMode::VEC_1D){
        int elements = getInputPort(0)->getDataType().numberOfElements();
        targetDimensions = {elements};
    }else if(mode == ReshapeMode::ROW_VEC){
        int elements = getInputPort(0)->getDataType().numberOfElements();
        targetDimensions = {1, elements};
    }else if(mode == ReshapeMode::COL_VEC){
        int elements = getInputPort(0)->getDataType().numberOfElements();
        targetDimensions = {elements, 1};
    }else if(mode == ReshapeMode::MANUAL){
        //Already set
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown Reshape Mode", getSharedPointer()));
    }
}

void Reshape::validate() {
    Node::validate();

    if(inputPorts.empty()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Expected at least 1 Port for Reshape", getSharedPointer()));
    }

    if(mode == ReshapeMode::REF_INPUT) {
        //Get the target dimensions from the second input
        if (inputPorts.size() < 2) {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Expected 2 Ports for Reshape when Mode is REF_INPUT",
                                                               getSharedPointer()));
        }
    }

    DataType inputDT = getInputPort(0)->getDataType();
    DataType inputDTScalar = inputDT;
    inputDTScalar.setDimensions({1});

    DataType outputDT = getOutputPort(0)->getDataType();
    DataType outputDTScalar = outputDT;
    outputDTScalar.setDimensions({1});

    if(inputDTScalar != outputDTScalar){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Input and Output Base Types Expected to be the Same", getSharedPointer()));
    }

    if(targetDimensions != outputDT.getDimensions()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Output Dimensions does not Match Expected Dimensions", getSharedPointer()));
    }

    if(inputDT.numberOfElements() != outputDT.numberOfElements()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Input and Output are Expected to have Same Number of Elements when Reshaping", getSharedPointer()));
    }
}

CExpr Reshape::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                         bool imag) {
    //Can be emitted as a simple memcpy as reshape does not re-arrange the order of elements stored in memory.
    //However, emitting as a for loop to hopefully make it easier for the compiler to analyze
    //Also avoiding casting to a pointer to avoid any ambiguity with array use

    //TODO: Re-evaluate if compiler not properly inferring these semantics

    //==== Get the expressions for the input ====
    std::shared_ptr<OutputPort> inputSrcOutputPort = getInputPort(0)->getSrcOutputPort();
    int inputSrcOutputPortNum = inputSrcOutputPort->getPortNum();
    std::shared_ptr<Node> inputSrcNode = inputSrcOutputPort->getParent();
    CExpr inputExpr = inputSrcNode->emitC(cStatementQueue, schedType, inputSrcOutputPortNum, imag);

    DataType inputDT = getInputPort(0)->getDataType();

    //Validated that dimension of output matches the target dimensions and the number of elements in input and output are the same
    DataType outputDT = getOutputPort(0)->getDataType();

    int numElements = inputDT.numberOfElements();

    //==== Emit Output Var ====
    std::string outputVarName = name + "_n" + GeneralHelper::to_string(id) + "_out";
    Variable outputVar = Variable(outputVarName, outputDT);
    //Emit variable declaration
    cStatementQueue.push_back(outputVar.getCVarDecl(imag, true, false, true, false) + ";");

    if(inputDT.isScalar()){
        cStatementQueue.push_back(outputVar.getCVarName(imag) + " = " + inputExpr.getExpr() + ";");
        return CExpr(outputVar.getCVarName(imag), CExpr::ExprType::SCALAR_VAR);
    }else {
        std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                EmitterHelpers::generateVectorMatrixForLoops({numElements});

        //Emit for loop
        std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
        std::vector<std::string> forLoopIndexVars = std::get<1>(forLoopStrs);
        std::vector<std::string> forLoopClose = std::get<2>(forLoopStrs);

        cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());

        std::string forLoopVar = forLoopIndexVars[0];

        //Generate Indexes for Src
        std::vector<std::string> srcIndexExprs;
        if (inputDT.isVector()) {
            srcIndexExprs.push_back(forLoopVar);
        } else {
            //Will have at least 2 dimensions
            std::vector<int> inputDims = inputDT.getDimensions();
            srcIndexExprs.push_back(forLoopVar + "%" + GeneralHelper::to_string(inputDims[inputDims.size()-1])); //No need to divide this entry
            int divisor = inputDims[inputDims.size()-1];
            for(int i = inputDims.size()-2; i>0; i--){
                srcIndexExprs.insert(srcIndexExprs.begin(), "(" + forLoopVar + "/" + GeneralHelper::to_string(divisor) + ")%" + GeneralHelper::to_string(inputDims[i]));
                divisor *= inputDims[i];
            }
            srcIndexExprs.insert(srcIndexExprs.begin(), "(" + forLoopVar + "/" + GeneralHelper::to_string(divisor) + ")"); //No need to mod this entry
        }
        std::string srcExpr = inputExpr.getExprIndexed(srcIndexExprs, true);

        //Generate Indexes for Dst
        std::vector<std::string> dstIndexExprs;
        if (outputDT.isVector()) {
            dstIndexExprs.push_back(forLoopVar);
        } else {
            //Will have at least 2 dimensions
            dstIndexExprs.push_back(forLoopVar + "%" + GeneralHelper::to_string(targetDimensions[targetDimensions.size()-1])); //No need to divide this entry
            int divisor = targetDimensions[targetDimensions.size()-1];
            for(int i = targetDimensions.size()-2; i>0; i--){
                dstIndexExprs.insert(dstIndexExprs.begin(), "(" + forLoopVar + "/" + GeneralHelper::to_string(divisor) + ")%" + GeneralHelper::to_string(targetDimensions[i]));
                divisor *= targetDimensions[i];
            }
            dstIndexExprs.insert(dstIndexExprs.begin(), "(" + forLoopVar + "/" + GeneralHelper::to_string(divisor) + ")"); //No need to mod this entry
        }
        std::string dstExpr = outputVar.getCVarName(imag) + EmitterHelpers::generateIndexOperation(dstIndexExprs);

        //Assign
        cStatementQueue.push_back(dstExpr + " = " + srcExpr + ";");

        //Close for loop
        cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());

        return CExpr(outputVar.getCVarName(imag), CExpr::ExprType::ARRAY);
    }
}

// [a][b][c]
// [x][y][z]
// z = i%c
// y = (i/c)%b
// x = (i/(c*b)) can optionally mode with a