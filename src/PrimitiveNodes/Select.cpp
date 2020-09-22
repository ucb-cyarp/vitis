//
// Created by Christopher Yarp on 8/2/20.
//

#include "Select.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"

Select::Select() : mode(SelectMode::INDEX_VECTOR), outputLen(0) {

}

Select::Select(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), mode(SelectMode::INDEX_VECTOR), outputLen(0) {

}

Select::Select(std::shared_ptr<SubSystem> parent, Select *orig) : PrimitiveNode(parent, orig), mode(orig->mode), outputLen(orig->outputLen){

}

std::shared_ptr<Select>
Select::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                          std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<Select> newNode = NodeFactory::createNode<Select>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important property ====
    std::string modeStr;
    std::string outputLenStr;

    //This is a vitis only block.  When importing from simulink it uses SimulinkSelect
    if(dialect == GraphMLDialect::VITIS){
        //Vitis Name -- InputSigns
        modeStr = dataKeyValueMap.at("Mode");
        outputLenStr = dataKeyValueMap.at("OutputLen");
    } else if(dialect == GraphMLDialect::SIMULINK_EXPORT) {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Select", newNode));
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Select", newNode));
    }

    newNode->mode = parseSelectModeStr(modeStr);
    newNode->outputLen = std::stoi(outputLenStr);

    return newNode;
}

Select::SelectMode Select::parseSelectModeStr(std::string str) {
    if(str == "OffsetLen"){
        return SelectMode::OFFSET_LENGTH;
    }else if(str == "IndexVector"){
        return SelectMode::INDEX_VECTOR;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown SelectMode"));
    }
}

std::string Select::selectModeToStr(Select::SelectMode mode) {
    if(mode == SelectMode::OFFSET_LENGTH){
        return "OffsetLen";
    }else if(mode == SelectMode::INDEX_VECTOR){
        return "IndexVector";
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown SelectMode"));
    }
}

std::set<GraphMLParameter> Select::graphMLParameters() {
    std::set<GraphMLParameter> parameters;
    parameters.insert(GraphMLParameter("Mode", "string", true));
    parameters.insert(GraphMLParameter("OutputLen", "int", true));

    return parameters;
}

xercesc::DOMElement *
Select::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Select");

    GraphMLHelper::addDataNode(doc, thisNode, "Mode", selectModeToStr(mode));
    GraphMLHelper::addDataNode(doc, thisNode, "OutputLen", GeneralHelper::to_string(outputLen));

    return thisNode;
}

std::string Select::typeNameStr() {
    return "Select";
}

std::string Select::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() + "\nMode:" + selectModeToStr(mode);
    if(mode == SelectMode::OFFSET_LENGTH){
        label += "\nOutputLen:" + GeneralHelper::to_string(outputLen);
    }

    return label;
}

std::shared_ptr<Node> Select::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Select>(parent, this);
}

void Select::validate() {
    Node::validate();

    if(inputPorts.size() != 2){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Should Have Exactly 2 Input Ports", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    DataType outputDT = getOutputPort(0)->getDataType();
    DataType outputDTScalar = outputDT;
    outputDTScalar.setDimensions({1});

    DataType inputDT = getInputPort(0)->getDataType();
    DataType inputDTScalar = outputDT;
    inputDTScalar.setDimensions({1});

    DataType indexDT = getInputPort(1)->getDataType();

    if(!inputDT.isVector()){
        //TODO: Support Multidimensional
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Currently, Only Vector Inputs are Supported", getSharedPointer()));
    }

    if(outputDTScalar != inputDTScalar){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Input and Output Should Have the Same Type", getSharedPointer()));
    }

    if(indexDT.isFloatingPt()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Index Port Should be an Integer (or Vector of Integers)", getSharedPointer()));
    }

    if(indexDT.isComplex()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Index Port Should be Real", getSharedPointer()));
    }

    if(mode == SelectMode::OFFSET_LENGTH){
        if(!indexDT.isScalar()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - The Index Input Should be a Scalar When Operating in OffsetLength Mode", getSharedPointer()));
        }

        if(outputDT.numberOfElements() != outputLen){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Output vector length should match outputLen parameter when operating in OffsetLength mode", getSharedPointer()));
        }
    }else if(mode == SelectMode::INDEX_VECTOR){
        if(!indexDT.isScalar() && !indexDT.isVector()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - The Index Input Should be a Scalar or Vector When Operating in IndexVector Mode", getSharedPointer()));
        }

        if(outputDT.numberOfElements() != indexDT.numberOfElements()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Dimensions of Output Vector and Index Vector Should Match When Operating in IndexVector Mode", getSharedPointer()));
        }
    }

    if(outputDT.numberOfElements() > inputDT.numberOfElements()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Dimensions of Output Vector Must be Smaller or Equal to Dimensions of Input Vector", getSharedPointer()));
    }
}

CExpr Select::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                        bool imag) {
    //==== Get the expressions for each input ====
    //Input
    std::shared_ptr<OutputPort> inputSrcOutputPort = getInputPort(0)->getSrcOutputPort();
    int inputSrcOutputPortNum = inputSrcOutputPort->getPortNum();
    std::shared_ptr<Node> inputSrcNode = inputSrcOutputPort->getParent();
    std::string inputExpr = inputSrcNode->emitC(cStatementQueue, schedType, inputSrcOutputPortNum, imag);

    //Index
    std::shared_ptr<OutputPort> indSrcOutputPort = getInputPort(1)->getSrcOutputPort();
    int indSrcOutputPortNum = indSrcOutputPort->getPortNum();
    std::shared_ptr<Node> indSrcNode = indSrcOutputPort->getParent();
    std::string indExpr = indSrcNode->emitC(cStatementQueue, schedType, indSrcOutputPortNum, false);

    //==== Emit Output Var ====
    std::string outputVarName = name + "_n" + GeneralHelper::to_string(id) + "_out";
    DataType outputDT = getOutputPort(0)->getDataType();

    Variable outputVar = Variable(outputVarName, outputDT);
    //Emit variable declaration
    cStatementQueue.push_back(outputVar.getCVarDecl(imag, true, false, true, false) + ";");

    //==== Emit Select ====
    if(mode == SelectMode::INDEX_VECTOR){
        DataType indexDT = getInputPort(1)->getDataType();

        //For loop if index is a vector
        std::vector<std::string> forLoopIndexVars;
        std::vector<std::string> forLoopClose;
        if(!indexDT.isScalar()){
            //Create nested loops for a given array
            std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                    EmitterHelpers::generateVectorMatrixForLoops(indexDT.getDimensions());

            std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
            forLoopIndexVars = std::get<1>(forLoopStrs);
            forLoopClose = std::get<2>(forLoopStrs);

            cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());
        }

        //--- Copy Relevant Elements---
        //The index into the input comes from the input vector.  If it is indeed a vector and not a scalar, we need to index into
        //the index vector
        std::string indTermDeref = indExpr + (indexDT.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars));
        //Note: this currently relies on the input being a vector and not a scalar (was validated above)
        //TODO: support multidimensional
        std::vector<std::string> indTermDerefVec = {indTermDeref};
        std::string srcTerm = inputExpr + EmitterHelpers::generateIndexOperation(indTermDerefVec);//Deref with the index term
        std::string dstTerm = outputVar.getCVarName(imag) + (indexDT.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars));

        cStatementQueue.push_back(dstTerm + " = " + srcTerm + ";");

        //Close for loop
        if(!indexDT.isScalar()){
            cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
        }
    }else if(mode == SelectMode::OFFSET_LENGTH){
        //For loop if length is > 1
        std::vector<std::string> forLoopIndexVars;
        std::vector<std::string> forLoopClose;
        //TODO: support multidimensional
        if(outputLen > 1){
            //Create nested loops for a given array
            std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                    EmitterHelpers::generateVectorMatrixForLoops({outputLen});

            std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
            forLoopIndexVars = std::get<1>(forLoopStrs);
            forLoopClose = std::get<2>(forLoopStrs);

            cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());
        }

        //--- Copy relevant elements ---
        //The index is the indexExpr + the offset from the for loop
        //TODO: support multidimensional, currently relies on input being a vector
        std::string indTerm = indExpr + (outputLen > 1 ? " + " + forLoopIndexVars[0] : "");
        std::vector<std::string> indTermVec = {indTerm};
        std::string srcTerm = inputExpr + EmitterHelpers::generateIndexOperation(indTermVec);
        std::string dstTerm = outputVar.getCVarName(imag) + (outputLen > 1 ? EmitterHelpers::generateIndexOperation(forLoopIndexVars) : "");

        cStatementQueue.push_back(dstTerm + " = " + srcTerm + ";");

        //Close for loop
        if(outputLen > 1){
            cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
        }
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown Select Mode", getSharedPointer()));
    }

    return CExpr(outputVar.getCVarName(imag), true);
}

Select::SelectMode Select::getMode() const {
    return mode;
}

void Select::setMode(Select::SelectMode mode) {
    Select::mode = mode;
}

int Select::getOutputLen() const {
    return outputLen;
}

void Select::setOutputLen(int outputLen) {
    Select::outputLen = outputLen;
}

