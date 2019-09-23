//
// Created by Christopher Yarp on 9/3/19.
//

#include <General/GeneralHelper.h>
#include "ThreadCrossingFIFO.h"
#include "General/ErrorHelpers.h"

int ThreadCrossingFIFO::getFifoLength() const {
    return fifoLength;
}

void ThreadCrossingFIFO::setFifoLength(int fifoLength) {
    ThreadCrossingFIFO::fifoLength = fifoLength;
}

std::vector<NumericValue> ThreadCrossingFIFO::getInitConditions() const {
    return initConditions;
}

void ThreadCrossingFIFO::setInitConditions(const std::vector<NumericValue> &initConditions) {
    ThreadCrossingFIFO::initConditions = initConditions;
}

Variable ThreadCrossingFIFO::getCStateVar() {
    ThreadCrossingFIFO::initializeVarIfNotAlready(getSharedPointer(), cStateVar, cStateVarInitialized, "src");

    return cStateVar;
}

void ThreadCrossingFIFO::setCStateVar(const Variable &cStateVar) {
    ThreadCrossingFIFO::cStateVar = cStateVar;
    cStateVarInitialized = true;
}

Variable ThreadCrossingFIFO::getCStateInputVar() {
    ThreadCrossingFIFO::initializeVarIfNotAlready(getSharedPointer(), cStateInputVar, cStateInputVarInitialized, "dst");

    return cStateInputVar;
}

void ThreadCrossingFIFO::setCStateInputVar(const Variable &cStateInputVar) {
    ThreadCrossingFIFO::cStateInputVar = cStateInputVar;
    cStateInputVarInitialized = true;
}

int ThreadCrossingFIFO::getBlockSize() const {
    return blockSize;
}

void ThreadCrossingFIFO::setBlockSize(int blockSize) {
    ThreadCrossingFIFO::blockSize = blockSize;
}

ThreadCrossingFIFO::ThreadCrossingFIFO() : fifoLength(8), blockSize(1), cStateVarInitialized(false),
                                           cStateInputVarInitialized(false){}

ThreadCrossingFIFO::ThreadCrossingFIFO(std::shared_ptr<SubSystem> parent) : Node(parent), fifoLength(8), blockSize(1),
                                                                            cStateVarInitialized(false),
                                                                            cStateInputVarInitialized(false){}

ThreadCrossingFIFO::ThreadCrossingFIFO(std::shared_ptr<SubSystem> parent, ThreadCrossingFIFO *orig) : Node(parent, orig),
                                       fifoLength(orig->fifoLength), initConditions(orig->initConditions),
                                       cStateVar(orig->cStateVar), cStateInputVar(orig->cStateInputVar),
                                       blockSize(orig->blockSize), cStateVarInitialized(orig->cStateVarInitialized),
                                       cStateInputVarInitialized(orig->cStateInputVarInitialized){}

std::set<GraphMLParameter> ThreadCrossingFIFO::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("FIFO_Length", "int", true));
    parameters.insert(GraphMLParameter("InitialCondition", "string", true));
    parameters.insert(GraphMLParameter("BlockSize", "int", true));

//    parameters.insert(GraphMLParameter("cStateVar_Name", "string", true));
//    parameters.insert(GraphMLParameter("cStateVar_DataType", "string", true));
//    parameters.insert(GraphMLParameter("cStateVar_Complex", "string", true));
//    parameters.insert(GraphMLParameter("cStateVar_Width", "string", true));
//    parameters.insert(GraphMLParameter("cStateVar_InitialCondition", "string", true));
//    parameters.insert(GraphMLParameter("cStateInputVar_Name", "string", true));
//    parameters.insert(GraphMLParameter("cStateInputVar_DataType", "string", true));
//    parameters.insert(GraphMLParameter("cStateInputVar_Complex", "string", true));
//    parameters.insert(GraphMLParameter("cStateInputVar_Width", "string", true));
//    parameters.insert(GraphMLParameter("cStateInputVar_InitialCondition", "string", true));

    return parameters;
}

void ThreadCrossingFIFO::populatePropertiesFromGraphML(std::map<std::string, std::string> dataKeyValueMap,
                                                       std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    if (dialect != GraphMLDialect::VITIS) {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - ThreadCrossing", getSharedPointer()));
    }

    //==== Import important properties ====
    std::string fifoLengthStr;
    std::string initialConditionStr;
    std::string blockSizeStr;

    //Variables will be initialized during emit

//    std::string cStateVarNameStr;
//    std::string cStateVarDataTypeStr;
//    std::string cStateVarComplexStr;
//    std::string cStateVarWidthStr;
//    std::string cStateVarInitValStr;
//    std::string cStateInputVarNameStr;
//    std::string cStateInputVarDataTypeStr;
//    std::string cStateInputVarComplexStr;
//    std::string cStateInputVarWidthStr;
//    std::string cStateInputVarInitValStr;

    //Vitis Names -- DelayLength, InitialCondit
    fifoLengthStr = dataKeyValueMap.at("FIFO_Length");
    initialConditionStr = dataKeyValueMap.at("InitialCondition");
    blockSizeStr = dataKeyValueMap.at("BlockSize");

//    cStateVarNameStr = dataKeyValueMap.at("cStateVar_Name");
//    cStateVarDataTypeStr = dataKeyValueMap.at("cStateVar_DataType");
//    cStateVarComplexStr = dataKeyValueMap.at("cStateVar_Complex");
//    cStateVarWidthStr = dataKeyValueMap.at("cStateVar_Width");
//    cStateVarInitValStr = dataKeyValueMap.at("cStateVar_InitialCondition");
//    cStateInputVarNameStr = dataKeyValueMap.at("cStateInputVar_Name");
//    cStateInputVarDataTypeStr = dataKeyValueMap.at("cStateInputVar_DataType");
//    cStateInputVarComplexStr = dataKeyValueMap.at("cStateInputVar_Complex");
//    cStateInputVarWidthStr = dataKeyValueMap.at("cStateInputVar_Width");
//    cStateInputVarInitValStr = dataKeyValueMap.at("cStateInputVar_InitialCondition");

    fifoLength = std::stoi(fifoLengthStr);
    initConditions = NumericValue::parseXMLString(initialConditionStr);
    blockSize = std::stoi(blockSizeStr);

//    bool cStateVarComplex = GeneralHelper::parseBool(cStateVarComplexStr);
//    int cStateVarWidth = std::stoi(cStateVarWidthStr);
//    std::vector<NumericValue> cStateVarInitConditions = NumericValue::parseXMLString(cStateVarInitValStr);
//    DataType cStateVarDT = DataType(cStateVarDataTypeStr, cStateVarComplex, cStateVarWidth);
//    cStateVar.setName(cStateVarNameStr);
//    cStateVar.setDataType(cStateVarDT);
//    cStateVar.setInitValue(cStateVarInitConditions);
//
//    bool cStateInputVarComplex = GeneralHelper::parseBool(cStateInputVarComplexStr);
//    int cStateInputVarWidth = std::stoi(cStateInputVarWidthStr);
//    std::vector<NumericValue> cStateInputVarInitConditions = NumericValue::parseXMLString(cStateInputVarInitValStr);
//    DataType cStateInputVarDT = DataType(cStateInputVarDataTypeStr, cStateInputVarComplex, cStateInputVarWidth);
//    cStateInputVar.setName(cStateVarNameStr);
//    cStateInputVar.setDataType(cStateInputVarDT);
//    cStateInputVar.setInitValue(cStateInputVarInitConditions);
}

void ThreadCrossingFIFO::emitPropertiesToGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode) {

    GraphMLHelper::addDataNode(doc, graphNode, "FIFO_Length", GeneralHelper::to_string(fifoLength));
    GraphMLHelper::addDataNode(doc, graphNode, "InitialCondition", NumericValue::toString(initConditions));
    GraphMLHelper::addDataNode(doc, graphNode, "BlockSize", GeneralHelper::to_string(blockSize));

    //Variables will be initialized during emit
//    GraphMLHelper::addDataNode(doc, graphNode, "cStateVar_Name", cStateVar.getName());
//    GraphMLHelper::addDataNode(doc, graphNode, "cStateVar_DataType", cStateVar.getDataType().toString(DataType::StringStyle::SIMULINK));
//    GraphMLHelper::addDataNode(doc, graphNode, "cStateVar_Complex", cStateVar.getDataType().isComplex() ? "true" : "false");
//    GraphMLHelper::addDataNode(doc, graphNode, "cStateVar_Width", GeneralHelper::to_string(cStateVar.getDataType().getWidth()));
//    GraphMLHelper::addDataNode(doc, graphNode, "cStateVar_InitialCondition", NumericValue::toString(cStateVar.getInitValue()));
//
//    GraphMLHelper::addDataNode(doc, graphNode, "cStateInputVar_Name", cStateInputVar.getName());
//    GraphMLHelper::addDataNode(doc, graphNode, "cStateInputVar_DataType", cStateInputVar.getDataType().toString(DataType::StringStyle::SIMULINK));
//    GraphMLHelper::addDataNode(doc, graphNode, "cStateInputVar_Complex", cStateInputVar.getDataType().isComplex() ? "true" : "false");
//    GraphMLHelper::addDataNode(doc, graphNode, "cStateInputVar_Width", GeneralHelper::to_string(cStateInputVar.getDataType().getWidth()));
//    GraphMLHelper::addDataNode(doc, graphNode, "cStateInputVar_InitialCondition", NumericValue::toString(cStateInputVar.getInitValue()));
}

std::string ThreadCrossingFIFO::labelStr() {
    std::string label = Node::labelStr();

    return label + "\nFIFO_Length:" + GeneralHelper::to_string(fifoLength) +
           "\nInitialCondition: " + NumericValue::toString(initConditions) +
           "\nBlockSize: " + GeneralHelper::to_string(blockSize);
}

void ThreadCrossingFIFO::validate() {
    Node::validate();

    //Should have 1 input ports and 1 output port
    if(inputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ThreadCrossingFIFO - Should Have Exactly 1 Input Port", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ThreadCrossingFIFO - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    //The src arcs should all be in one partition and the dst arcs should all be in (a different) single partition
    std::set<std::shared_ptr<Arc>> inArcs = getInputPort(0)->getArcs();
    std::set<std::shared_ptr<Arc>> outArcs = getOutputPort(0)->getArcs();

    if(inArcs.empty()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ThreadCrossingFIFO - Should have a driver Arc", getSharedPointer()));
    }

    if(outArcs.empty()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ThreadCrossingFIFO - Should at least 1 output Arc", getSharedPointer()));
    }

    int partitionIn = (*inArcs.begin())->getSrcPort()->getParent()->getPartitionNum();

    bool firstOut = true;
    int partitionOut;
    for(auto outArc = outArcs.begin(); outArc != outArcs.end(); outArc++){
        if(firstOut){
            firstOut = false;
            partitionOut = (*outArc)->getDstPort()->getParent()->getPartitionNum();
        }else{
            if(partitionOut != (*outArc)->getDstPort()->getParent()->getPartitionNum()){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ThreadCrossingFIFO - Outputs Arcs to Different Partitions Detected", getSharedPointer()));
            }
        }
    }

    if(partitionIn == partitionOut){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ThreadCrossingFIFO - Input and Output Partitions are the Same", getSharedPointer()));
    }

        if(initConditions.size() > (fifoLength*blockSize - blockSize)){ // - blockSize because we need to be able to write 1 value into the FIFO to ensure deadlock cannot occure
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ThreadCrossingFIFO - The number of initial conditions cannot be larger than the FIFO - 1 block", getSharedPointer()));
    }
}

bool ThreadCrossingFIFO::hasState() {
    return true;
}

bool ThreadCrossingFIFO::hasCombinationalPath() {
    return false;
}

std::vector<Variable> ThreadCrossingFIFO::getCStateVars() {
    //Do not return anything by default.  Handled durring thread creation
    return Node::getCStateVars();
}

CExpr ThreadCrossingFIFO::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                    int outputPortNum, bool imag) {
    //getCStateVar() will initialize cStateVar if it is not already

    std::string expr;
    if(blockSize > 1){
        int width = getCStateVar().getDataType().getWidth();
        if(width > 1){
            expr = "(" + getCStateVar().getCVarName(imag) + "+" + GeneralHelper::to_string(width) + "*" + cBlockIndexVarName + ")";
        }else{
            expr = "(" +  getCStateVar().getCVarName(imag) + "[" + cBlockIndexVarName + "])";
        }
    }else{
        expr = getCStateVar().getCVarName(imag);
    }

    return CExpr(expr, true);;
}

void ThreadCrossingFIFO::emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType) {
    //This does not do anything.  The read/write operations are handled by the core schedulers
}

void
ThreadCrossingFIFO::emitCExprNextState(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType) {
    DataType inputDataType = getInputPort(0)->getDataType();
    std::shared_ptr<OutputPort> srcPort = getInputPort(0)->getSrcOutputPort();
    int srcOutPortNum = srcPort->getPortNum();
    std::shared_ptr<Node> srcNode = srcPort->getParent();

    //Emit the upstream
    std::string inputExprRe = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, false);
    std::string inputExprIm;

    if(inputDataType.isComplex()){
        inputExprIm = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, true);
    }

    //TODO: Implement Vector Support (need to loop over input variable indexes (will be stored as a variable due to default behavior of internal fanout

    std::string stateInputDeclAssignRe = getCStateInputVar().getCVarDecl(false, false, false, false) + " = " + inputExprRe + ";";
    cStatementQueue.push_back(stateInputDeclAssignRe);
    if(inputDataType.isComplex()){
        std::string stateInputDeclAssignIm = getCStateInputVar().getCVarDecl(true, false, false, false) + " = " + inputExprIm + ";";
        cStatementQueue.push_back(stateInputDeclAssignIm);
    }
}

bool ThreadCrossingFIFO::createStateUpdateNode(std::vector<std::shared_ptr<Node>> &new_nodes,
                                               std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                               std::vector<std::shared_ptr<Arc>> &new_arcs,
                                               std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                               bool includeContext) {
    return false;
}

bool ThreadCrossingFIFO::canExpand() {
    return false;
}

void ThreadCrossingFIFO::initializeVarIfNotAlready(std::shared_ptr<Node> node, Variable &var, bool &init, std::string suffix){
    //If not yet initialized, initialize
    if(!init){
        std::string varName = node->getName()+"_n"+GeneralHelper::to_string(node->getId())+"_" +suffix;
        var.setName(varName);

        init = true;
    }
}

std::string ThreadCrossingFIFO::getCBlockIndexVarName() const {
    return cBlockIndexVarName;
}

void ThreadCrossingFIFO::setCBlockIndexVarName(const std::string &cBlockIndexVarName) {
    ThreadCrossingFIFO::cBlockIndexVarName = cBlockIndexVarName;
}

std::string ThreadCrossingFIFO::getFIFOStructTypeName(){
    if(cStateVar.getDataType().isComplex()){
        return cStateVar.getName() + "_t";
    }
    return "";
}

std::string ThreadCrossingFIFO::createFIFOStruct(){
    if(cStateVar.getDataType().isComplex()) {
        std::string typeName = getFIFOStructTypeName();
        DataType stateDT = cStateVar.getDataType();
        int elementWidth = stateDT.getWidth();

        //TODO: Check 2D case
        std::string structStr = "typedef struct {\n";
        structStr += stateDT.toString(DataType::StringStyle::C) + " real" + (blockSize>1 ? "[" + GeneralHelper::to_string(blockSize) + "]" : "") + (elementWidth>1 ? "[" + GeneralHelper::to_string(elementWidth) + "]" : "");

        if(stateDT.isComplex()) {
            structStr += ",\n" + stateDT.toString(DataType::StringStyle::C) + " imag" + (blockSize>1 ? "[" + GeneralHelper::to_string(blockSize) + "]" : "") + (elementWidth>1 ? "[" + GeneralHelper::to_string(elementWidth) + "]" : "") + "\n";
        }
        structStr += "} " + typeName + ";";
        return structStr;
    }

    return "";
}