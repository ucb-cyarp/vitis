//
// Created by Christopher Yarp on 9/3/19.
//

#include <General/GeneralHelper.h>
#include "ThreadCrossingFIFO.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"
#include "Blocking/BlockingHelpers.h"

int ThreadCrossingFIFO::getFifoLength() const {
    return fifoLength;
}

void ThreadCrossingFIFO::setFifoLength(int fifoLength) {
    ThreadCrossingFIFO::fifoLength = fifoLength;
}

std::vector<std::vector<NumericValue>> ThreadCrossingFIFO::getInitConditions() const {
    return initConditions;
}

void ThreadCrossingFIFO::setInitConditions(const std::vector<std::vector<NumericValue>> &initConditions) {
    ThreadCrossingFIFO::initConditions = initConditions;
}

std::vector<NumericValue> ThreadCrossingFIFO::getInitConditionsCreateIfNot(int port) {
    while(port >= initConditions.size()){
        initConditions.push_back(std::vector<NumericValue>());
    }
    return initConditions[port];
}

void ThreadCrossingFIFO::setInitConditionsCreateIfNot(int port, const std::vector<NumericValue> &initConditions) {
    while(port >= ThreadCrossingFIFO::initConditions.size()){
        ThreadCrossingFIFO::initConditions.push_back(std::vector<NumericValue>());
    }
    ThreadCrossingFIFO::initConditions[port] = initConditions;
}

Variable ThreadCrossingFIFO::getCStateVar(int port) {
    while(port >= cStateVars.size()){
        cStateVars.push_back(Variable());
        cStateVarsInitialized.push_back(false);
    }

    ThreadCrossingFIFO::initializeVarIfNotAlready(getSharedPointer(), cStateVars, cStateVarsInitialized, port, "src");

    return cStateVars[port];
}

void ThreadCrossingFIFO::setCStateVar(int port, const Variable &cStateVar) {
    while(port >= cStateVars.size()){
        cStateVars.push_back(Variable());
        cStateVarsInitialized.push_back(false);
    }

    cStateVars[port] = cStateVar;
    cStateVarsInitialized[port] = true;
}

Variable ThreadCrossingFIFO::getCStateVarExpandedForBlockSize(int port){
    //After correctly setting the base type of the variable (in initializeVarIfNotAlready), scale by
    //the block size

    Variable var = getCStateVar(port);

    //Expand the variable based on the block size of the FIFO
    //Note that this does not propagate outside of this function
    if(getBlockSizeCreateIfNot(port)>1){
        DataType dt = var.getDataType();
        dt = dt.expandForBlock(getBlockSizeCreateIfNot(port));
        var.setDataType(dt);
    }

    return var;
}

Variable ThreadCrossingFIFO::getCStateInputVar(int port) {
    while(port >= cStateInputVars.size()){
        cStateInputVars.push_back(Variable());
        cStateInputVarsInitialized.push_back(false);
    }

    ThreadCrossingFIFO::initializeVarIfNotAlready(getSharedPointer(), cStateInputVars, cStateInputVarsInitialized, port, "dst");

    return cStateInputVars[port];
}

Variable ThreadCrossingFIFO::getCStateInputVarExpandedForBlockSize(int port){
    //After correctly setting the base type of the variable (in initializeVarIfNotAlready), scale by
    //the block size

    Variable var = getCStateInputVar(port);

    //Expand the variable based on the block size of the FIFO
    //Note that this does not propagate outside of this function

    //For FIFOs outside any blocking domain, including the global blocking domain (ie. I/O FIFO at base rate=), do not
    //expand the size of the FIFO.  In this case, their block size will be set to 1 and any indexing is handled
    //by the BlockingDomain input and output nodes.

    if(getBlockSizeCreateIfNot(port)>1){
        DataType dt = var.getDataType();
        dt = dt.expandForBlock(getBlockSizeCreateIfNot(port));
        var.setDataType(dt);
    }

    return var;
}

void ThreadCrossingFIFO::setCStateInputVar(int port, const Variable &cStateInputVar) {
    while(port >= cStateInputVars.size()){
        cStateInputVars.push_back(Variable());
        cStateInputVarsInitialized.push_back(false);
    }

    cStateInputVars[port] = cStateInputVar;
    cStateInputVarsInitialized[port] = true;
}

std::vector<int> ThreadCrossingFIFO::getBlockSizes() const {
    return blockSizes;
}

void ThreadCrossingFIFO::setBlockSizes(const std::vector<int> &blockSizes){
    ThreadCrossingFIFO::blockSizes = blockSizes;
}

void ThreadCrossingFIFO::setBlockSize(int portNum, int blockSize){
    unsigned long portLen = blockSizes.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        blockSizes.push_back(0);
    }

    blockSizes[portNum] = blockSize;
}

int ThreadCrossingFIFO::getBlockSizeCreateIfNot(int portNum){
    unsigned long portLen = blockSizes.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        blockSizes.push_back(0);
    }

    return blockSizes[portNum];
}

std::vector<int> ThreadCrossingFIFO::getSubBlockSizesIn() const {
    return subBlockSizesIn;
}

int ThreadCrossingFIFO::getSubBlockSizeInCreateIfNot(int portNum){
    unsigned long portLen = subBlockSizesIn.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        subBlockSizesIn.push_back(0);
    }

    return subBlockSizesIn[portNum];
}

void ThreadCrossingFIFO::setSubBlockSizesIn(const std::vector<int> &subBlockSizes){
    ThreadCrossingFIFO::subBlockSizesIn = subBlockSizes;
}

void ThreadCrossingFIFO::setSubBlockSizeIn(int portNum, int subBlockSize){
    unsigned long portLen = subBlockSizesIn.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        subBlockSizesIn.push_back(0);
    }

    subBlockSizesIn[portNum] = subBlockSize;
}

std::vector<int> ThreadCrossingFIFO::getSubBlockSizesOut() const {
    return subBlockSizesOut;
}

int ThreadCrossingFIFO::getSubBlockSizeOutCreateIfNot(int portNum){
    unsigned long portLen = subBlockSizesOut.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        subBlockSizesOut.push_back(0);
    }

    return subBlockSizesOut[portNum];
}

void ThreadCrossingFIFO::setSubBlockSizesOut(const std::vector<int> &subBlockSizes){
    ThreadCrossingFIFO::subBlockSizesOut = subBlockSizes;
}

void ThreadCrossingFIFO::setSubBlockSizeOut(int portNum, int subBlockSize){
    unsigned long portLen = subBlockSizesOut.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        subBlockSizesOut.push_back(0);
    }

    subBlockSizesOut[portNum] = subBlockSize;
}

//
std::vector<int> ThreadCrossingFIFO::getBaseSubBlockSizesIn() const {
    return baseSubBlockSizesIn;
}

int ThreadCrossingFIFO::getBaseSubBlockSizeInCreateIfNot(int portNum){
    unsigned long portLen = baseSubBlockSizesIn.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        baseSubBlockSizesIn.push_back(0);
    }

    return baseSubBlockSizesIn[portNum];
}

std::vector<int> ThreadCrossingFIFO::getBaseSubBlockSizesOut() const {
    return baseSubBlockSizesOut;
}

int ThreadCrossingFIFO::getBaseSubBlockSizeOutCreateIfNot(int portNum){
    unsigned long portLen = baseSubBlockSizesOut.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        baseSubBlockSizesOut.push_back(0);
    }

    return baseSubBlockSizesOut[portNum];
}

void ThreadCrossingFIFO::setBaseSubBlockSizesIn(const std::vector<int> &subBlockSizes){
    ThreadCrossingFIFO::baseSubBlockSizesIn = subBlockSizes;
}

void ThreadCrossingFIFO::setBaseSubBlockSizeIn(int portNum, int subBlockSize){
    unsigned long portLen = baseSubBlockSizesIn.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        baseSubBlockSizesIn.push_back(0);
    }

    baseSubBlockSizesIn[portNum] = subBlockSize;
}

void ThreadCrossingFIFO::setBaseSubBlockSizesOut(const std::vector<int> &subBlockSizes){
    ThreadCrossingFIFO::baseSubBlockSizesOut = subBlockSizes;
}

void ThreadCrossingFIFO::setBaseSubBlockSizeOut(int portNum, int subBlockSize){
    unsigned long portLen = baseSubBlockSizesOut.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        baseSubBlockSizesOut.push_back(0);
    }

    baseSubBlockSizesOut[portNum] = subBlockSize;
}

ThreadCrossingFIFO::ThreadCrossingFIFO() : fifoLength(8), copyMode(ThreadCrossingFIFOParameters::CopyMode::CLANG_MEMCPY_INLINED){}

ThreadCrossingFIFO::ThreadCrossingFIFO(std::shared_ptr<SubSystem> parent) : Node(parent), fifoLength(8), copyMode(ThreadCrossingFIFOParameters::CopyMode::CLANG_MEMCPY_INLINED){}

ThreadCrossingFIFO::ThreadCrossingFIFO(std::shared_ptr<SubSystem> parent, ThreadCrossingFIFO *orig) : Node(parent, orig),
                                       fifoLength(orig->fifoLength), initConditions(orig->initConditions),
                                       cStateVars(orig->cStateVars), cStateInputVars(orig->cStateInputVars),
                                       cBlockIndexExprsInput(orig->cBlockIndexExprsInput),
                                       cBlockIndexExprsOutput(orig->cBlockIndexExprsOutput),
                                       blockSizes(orig->blockSizes), subBlockSizesIn(orig->subBlockSizesIn),
                                       subBlockSizesOut(orig->subBlockSizesOut),
                                       cStateVarsInitialized(orig->cStateVarsInitialized),
                                       cStateInputVarsInitialized(orig->cStateInputVarsInitialized),
                                       copyMode(orig->copyMode){}

std::set<GraphMLParameter> ThreadCrossingFIFO::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("FIFO_Length", "int", true));
    //There is a seperate initial condition parameter for each port pair
    for(int i = 0; i<initConditions.size(); i++) {
        parameters.insert(GraphMLParameter("InitialCondition_Port" + GeneralHelper::to_string(i), "string", true));
    }
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

    //There is a seperate parameter for the initial condition of each port
    bool done = false;
    int portNum = 0;
    while(!done) {
        auto blockSizeIter = dataKeyValueMap.find("BlockSize_Port" + GeneralHelper::to_string(portNum));
        auto initialConditionIter = dataKeyValueMap.find("InitialCondition_Port" + GeneralHelper::to_string(portNum));

        if(blockSizeIter != dataKeyValueMap.end()){
            //Read block size for port
            int blockSizePortN = std::stoi(blockSizeIter->second);
            setBlockSize(portNum, blockSizePortN);

            //Read Initial Conditions for Port
            std::string initialConditionStr = initialConditionIter->second;
            std::vector<NumericValue> portInitConditions = NumericValue::parseXMLString(initialConditionStr);
            setInitConditionsCreateIfNot(portNum, portInitConditions);
            portNum++;
        }else{
            done = true;
        }
    }

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

    //There is a separate block size & initial condition entry for each port pair
    for(int i = 0; i<blockSizes.size(); i++){
        GraphMLHelper::addDataNode(doc, graphNode, "BlockSize_Port" + GeneralHelper::to_string(i), GeneralHelper::to_string(blockSizes[i]));
        GraphMLHelper::addDataNode(doc, graphNode, "InitialCondition_Port" + GeneralHelper::to_string(i), NumericValue::toString(initConditions[i]));
    }

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

std::string ThreadCrossingFIFO::typeNameStr(){
    return "ThreadCrossingFIFO";
}

std::string ThreadCrossingFIFO::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr();
    label += "\nFIFO_Length: " + GeneralHelper::to_string(fifoLength);

    //There is a separate initial condition entry for each port pair
    for(int i = 0; i<blockSizes.size(); i++){
        label += "\nBlockSize_Port" + GeneralHelper::to_string(i) + ": " + GeneralHelper::to_string(blockSizes[i]);
        label += "\nInitialCondition_Port" + GeneralHelper::to_string(i) + ": " + NumericValue::toString(initConditions[i]);
    }

    return label;
}

void ThreadCrossingFIFO::validate() {
    Node::validate();

    //TODO: Check Initial Conditions against the block size of each port
    //      Check same number of block of initial conditions across all ports (not necessarily the same number of elements)

    //Should have 1 input ports and 1 output port
    if(inputPorts.size() < 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ThreadCrossingFIFO - Should Have at least 1 Input Port", getSharedPointer()));
    }

    if(outputPorts.size() < 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ThreadCrossingFIFO - Should Have at least 1 Output Port", getSharedPointer()));
    }

    if(inputPorts.size() != outputPorts.size()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ThreadCrossingFIFO - Number of input and output ports should match", getSharedPointer()));
    }

    int globalPartitionOut;
    bool globalPartitionOutSet = false;

    //Check each port pair
    for(int portNum = 0; portNum<inputPorts.size(); portNum++) {
        //The src arcs should all be in one partition and the dst arcs should all be in (a different) single partition
        std::set<std::shared_ptr<Arc>> inArcs = getInputPort(portNum)->getArcs();
        std::set<std::shared_ptr<Arc>> outArcs = getOutputPort(portNum)->getArcs();

        if (inArcs.empty()) {
            throw std::runtime_error(
                    ErrorHelpers::genErrorStr("Validation Failed - ThreadCrossingFIFO - Port " + GeneralHelper::to_string(portNum) + " should have a driver Arc",
                                              getSharedPointer()));
        }

        if (outArcs.empty()) {
            throw std::runtime_error(
                    ErrorHelpers::genErrorStr("Validation Failed - ThreadCrossingFIFO - Port " + GeneralHelper::to_string(portNum) + " should at least 1 output Arc",
                                              getSharedPointer()));
        }

        //Already validated all arcs to a port have the same DT, now check that the datatypes of the input/output port pair match
        //The dimensions may differ according to the sub-blocking length
        DataType srcDT = (*inArcs.begin())->getDataType();
        DataType dstDT = (*outArcs.begin())->getDataType();

        DataType srcDTScalar = srcDT;
        srcDTScalar.setDimensions({1});
        DataType dstDTScalar = dstDT;
        dstDTScalar.setDimensions({1});

        if(srcDTScalar != dstDTScalar){
            throw std::runtime_error(
                ErrorHelpers::genErrorStr("Validation Failed - ThreadCrossingFIFO - Port " + GeneralHelper::to_string(portNum) + " input and output datatypes should match",
                                      getSharedPointer()));
        }

        //TODO: Add dimension checks

        int partitionIn = (*inArcs.begin())->getSrcPort()->getParent()->getPartitionNum();

        if(partitionIn != partitionNum){
            throw std::runtime_error(
                    ErrorHelpers::genErrorStr("Validation Failed - ThreadCrossingFIFO - Input Port " + GeneralHelper::to_string(portNum) + " should be in the same partition as the FIFO",
                                              getSharedPointer()));
        }

        int partitionOut;
        for (auto outArc = outArcs.begin(); outArc != outArcs.end(); outArc++) {
            partitionOut = (*outArc)->getDstPort()->getParent()->getPartitionNum();

            if(!globalPartitionOutSet){
                globalPartitionOut = partitionOut;
                globalPartitionOutSet = true;
            }else{
                if (partitionOut != globalPartitionOut) {
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "Validation Failed - ThreadCrossingFIFO - Outputs Ports to Different Partitions Detected",
                            getSharedPointer()));
                }
            }
        }

        if (getInitConditionsCreateIfNot(portNum).size() > (fifoLength-1) * (getBlockSizeCreateIfNot(portNum)*getInputPort(portNum)->getDataType().numberOfElements()/getSubBlockSizeInCreateIfNot(portNum))) { // - blockSize because we need to be able to write 1 value into the FIFO to ensure deadlock cannot occur
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "Validation Failed - ThreadCrossingFIFO - The number of initial conditions cannot be larger than the FIFO - 1 block",
                    getSharedPointer()));
        }

        if (getInitConditionsCreateIfNot(portNum).size() % (getBlockSizeCreateIfNot(portNum)*getInputPort(portNum)->getDataType().numberOfElements()/getSubBlockSizeInCreateIfNot(portNum)) != 0) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "Validation Failed - ThreadCrossingFIFO - Initial Conditions for Port " +
                    GeneralHelper::to_string(portNum) + " (" + GeneralHelper::to_string(getInitConditionsCreateIfNot(portNum).size()) +
                    ") must be a multiple of its block size (" +
                    GeneralHelper::to_string(getBlockSizeCreateIfNot(portNum)*getInputPort(portNum)->getDataType().numberOfElements()/getSubBlockSizeInCreateIfNot(portNum)) + ")",
                    getSharedPointer()));
        }

        if(portNum != 0){
            if(getInitConditionsCreateIfNot(portNum).size()/getBlockSizeCreateIfNot(portNum)/getInputPort(portNum)->getDataType().numberOfElements()/getSubBlockSizeInCreateIfNot(portNum) != getInitConditionsCreateIfNot(0).size()/getBlockSizeCreateIfNot(0)/getInputPort(0)->getDataType().numberOfElements()/getSubBlockSizeInCreateIfNot(portNum)){
                throw std::runtime_error(ErrorHelpers::genErrorStr(
                        "Validation Failed - ThreadCrossingFIFO - All ports must have the same number of initial conditions",
                        getSharedPointer()));
            }
        }

        if(getBlockSizeCreateIfNot(portNum) % getSubBlockSizeInCreateIfNot(portNum) != 0){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Sub-Block Size of Input Must be a Factor of the Block Size", getSharedPointer()));
        }

        if(getBlockSizeCreateIfNot(portNum) % getSubBlockSizeOutCreateIfNot(portNum) != 0){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Sub-Block Size of Output Must be a Factor of the Block Size", getSharedPointer()));
        }
    }

    if(orderConstraintInputPort){
        std::set<std::shared_ptr<Arc>> orderConstraintInputArcs = orderConstraintInputPort->getArcs();

        for(auto it = orderConstraintInputArcs.begin(); it != orderConstraintInputArcs.end(); it++){
            int partitionIn = (*it)->getSrcPort()->getParent()->getPartitionNum();

            if(partitionIn != partitionNum){
                throw std::runtime_error(
                        ErrorHelpers::genErrorStr("Validation Failed - ThreadCrossingFIFO - Order Constraint Port should be in the same partition as the FIFO",
                                                  getSharedPointer()));
            }
        }
    }

    if(orderConstraintOutputPort){
        std::set<std::shared_ptr<Arc>> orderConstraintOutputArcs = orderConstraintOutputPort->getArcs();

        for(auto it = orderConstraintOutputArcs.begin(); it != orderConstraintOutputArcs.end(); it++){
            int partitionOut = (*it)->getDstPort()->getParent()->getPartitionNum();

            if(!globalPartitionOutSet){
                globalPartitionOut = partitionOut;
                globalPartitionOutSet = true;
            }else{
                if (partitionOut != globalPartitionOut) {
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "Validation Failed - ThreadCrossingFIFO - Output Constraint Port to Different Partition Detected",
                            getSharedPointer()));
                }
            }
        }
    }

    //TODO: Verify all nodes at output of a port have the same base sub-blocking rate in addition to having the same clock domain
    //TODO: Modify after https://github.com/ucb-cyarp/vitis/issues/101 resolved
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
    CExpr::ExprType exprType;
    //Note: FIFOs outside of any blocking domain (including the global blocking domain), ie. I/O FIFOs at the base rate
    //will have their indexing handled by the Blocking Domain Input/Output nodes.  They will have their block size set to 1
    if(getBlockSizeCreateIfNot(outputPortNum) > 1){
        //Indexing into the FIFO is required at this point

        //Because of C multidimensional array semantics, and because the added dimension for blocks >1 is prepended to
        //the dimensions, indexing the first dimension will return the correct value.  If the data type is a scalar, it
        //returns a scalar value for the given block.  If the data type is a vector or matrix, this will still return a
        //pointer but a pointer to the correct block.

        //If there is a sub-blocking length >1, however, we need to not "dereference" this just yet as indexing into the sub-block
        //still needs to occur at this outer dimension
        if(getSubBlockSizeOutCreateIfNot(outputPortNum) > 1){
            expr = "(" +  getCStateVar(outputPortNum).getCVarName(imag) + "+(" + getCBlockIndexExprOutputCreateIfNot(outputPortNum) + "))";
            exprType = CExpr::ExprType::ARRAY; //Guaranteed to be an array, regardless of the dimensions of the port
        }else {
            expr = "(" + getCStateVar(outputPortNum).getCVarName(imag) + "[" + getCBlockIndexExprOutputCreateIfNot(outputPortNum) + "])";
            exprType = getInputPort(outputPortNum)->getDataType().isScalar() ? CExpr::ExprType::SCALAR_VAR : CExpr::ExprType::ARRAY;
        }
    }else{
        //The block size is 1, just return the state variable.  No indexing based on the current block is required
        expr = getCStateVar(outputPortNum).getCVarName(imag);
        exprType = getInputPort(outputPortNum)->getDataType().isScalar() ? CExpr::ExprType::SCALAR_VAR : CExpr::ExprType::ARRAY;
    }

    //Will output as a variable even though we index into the block.
    return CExpr(expr, exprType);
}

void ThreadCrossingFIFO::emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, std::shared_ptr<StateUpdate> stateUpdateSrc) {
    //This does not do anything.  The read/write operations are handled by the core schedulers
}

void
ThreadCrossingFIFO::emitCExprNextState(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType) {
    //Do for each input port
    for(int i = 0; i<inputPorts.size(); i++) {
        DataType inputDataType = getInputPort(i)->getDataType();
        std::shared_ptr<OutputPort> srcPort = getInputPort(i)->getSrcOutputPort();
        int srcOutPortNum = srcPort->getPortNum();
        std::shared_ptr<Node> srcNode = srcPort->getParent();

        //Emit the upstream
        CExpr inputExprRe = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, false);
        CExpr inputExprIm;

        if (inputDataType.isComplex()) {
            inputExprIm = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, true);
        }

        std::vector<std::string> forLoopIndexVars;
        std::vector<std::string> forLoopClose;
        //If the output is a vector, construct a for loop which puts the results in a temporary array
        if(!inputDataType.isScalar()){
            //Create nested loops for a given array
            std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                    EmitterHelpers::generateVectorMatrixForLoops(inputDataType.getDimensions());

            std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
            forLoopIndexVars = std::get<1>(forLoopStrs);
            forLoopClose = std::get<2>(forLoopStrs);

            cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());
        }

        int blockSize = getBlockSizeCreateIfNot(i);
        std::string cBlockIndexExpr = getCBlockIndexExprInputCreateIfNot(i);
        //If the sub-blocking length is not 1, the input to the FIFO should have an outer dimension which is equivalent to the
        //sub blocking length.  Copying the full sub-block should occur automatically like any non-scalar copy.  Indexing into
        //the sub-block should not be included in getCBlockIndexExprOutputCreateIfNot.

        //Check I/O Types Vs. Buffer Type
        std::vector<int> expectedDTDims = inputDataType.getDimensions();
        if(blockSize != 1) { //If Block Size is 1, the getCStateVarExpandedForBlockSize should match the I/O type as is
            if(getSubBlockSizeInCreateIfNot(i) > 1){
                int numSubBlocks = getBlockSizeCreateIfNot(i)/getSubBlockSizeInCreateIfNot(i);
                expectedDTDims[0] *= numSubBlocks; //The outer dimension should be the sub-block size.  Scale it by the number of sub-blocks
            }else{
                if(inputDataType.isScalar()){ //Another dimension is not added, it is just scaled to the block size
                    expectedDTDims[0] = blockSize;
                }else {
                    expectedDTDims.insert(expectedDTDims.begin(), getBlockSizeCreateIfNot(i));
                }
            }
        }
        DataType expectedDT = inputDataType;
        expectedDT.setDimensions(expectedDTDims);
        DataType cStateVarExpandedForBlockSizeDT = getCStateVarExpandedForBlockSize(i).getDataType();

        if(expectedDT != cStateVarExpandedForBlockSizeDT){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unexpected Input/Output Type for FIFO compared to Input Var Type", getSharedPointer()));
        }

        std::vector<std::string> emptyArr;
        std::string stateInputDeclAssignRe;
        if(blockSize == 1 && inputDataType.isScalar()){
            //No indexing required.  The FIFO variable is a scalar.  Dereference the state variable and assign
            stateInputDeclAssignRe += "*" + getCStateInputVar(i).getCVarName(false);
        }else{
            //There are arrays being handled.  Either the block size is > 1, the input is non-scalar, or both
            if(blockSize>1){
                //The FIFO has a block size > 1.  Perform the indexing to get to the sub-block
                if(getSubBlockSizeInCreateIfNot(i) > 1){
                    //The outer index will be into the sub-blocks.  Do not dereference this first index
                    stateInputDeclAssignRe += "(" + getCStateInputVar(i).getCVarName(false) + "+(" + cBlockIndexExpr + "))";
                }else {
                    //Sub blocking of size 1, can dereference outer dimension
                    stateInputDeclAssignRe += "(" + getCStateInputVar(i).getCVarName(false) + "[" + cBlockIndexExpr + "]" + ")";
                }
            }else{
                //Has a block size of 1, just pass the variable without any indexing
                stateInputDeclAssignRe += getCStateInputVar(i).getCVarName(false);
            }
        }
        //Perform the assignment
        if(inputDataType.isScalar()){
            stateInputDeclAssignRe += "=" + inputExprRe.getExpr();
        }else{
            stateInputDeclAssignRe += EmitterHelpers::generateIndexOperation(forLoopIndexVars) + "=" + inputExprRe.getExprIndexed(forLoopIndexVars, true);
        }
        stateInputDeclAssignRe += ";";
        cStatementQueue.push_back(stateInputDeclAssignRe);

        if (inputDataType.isComplex()) {
            std::string stateInputDeclAssignIm;
            if(blockSize == 1 && inputDataType.isScalar()){
                //No indexing required.  The FIFO variable is a scalar.  Dereference the state variable and assign
                stateInputDeclAssignIm += "*" + getCStateInputVar(i).getCVarName(true);
            }else{
                //There are arrays being handled.  Either the block size is > 1, the input is non-scalar, or both
                if(blockSize>1){
                    //The FIFO has a block size > 1.  Perform the indexing to get to the sub-block
                    if(getSubBlockSizeInCreateIfNot(i) > 1){
                        //The outer index will be into the sub-blocks.  Do not dereference this first index
                        stateInputDeclAssignIm +=  "(" + getCStateInputVar(i).getCVarName(true) + "+(" + cBlockIndexExpr + "))";
                    }else {
                        //Sub blocking of size 1, can dereference outer dimension
                        stateInputDeclAssignIm +=  "(" + getCStateInputVar(i).getCVarName(true) + "[" + cBlockIndexExpr + "]" + ")";
                    }
                }else{
                    //Has a block size of 1, just pass the variable without any indexing
                    stateInputDeclAssignIm += getCStateInputVar(i).getCVarName(true);
                }
            }
            //Perform the assignment
            if(inputDataType.isScalar()){
                stateInputDeclAssignIm += "=" + inputExprIm.getExpr();
            }else{
                stateInputDeclAssignIm += EmitterHelpers::generateIndexOperation(forLoopIndexVars) + "=" + inputExprIm.getExprIndexed(forLoopIndexVars, true);
            }
            stateInputDeclAssignIm += ";";
            cStatementQueue.push_back(stateInputDeclAssignIm);
        }

        //Close for loop
        if(!inputDataType.isScalar()){
            cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
        }
    }
}

bool ThreadCrossingFIFO::createStateUpdateNode(std::vector<std::shared_ptr<Node>> &new_nodes,
                                               std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                               std::vector<std::shared_ptr<Arc>> &new_arcs,
                                               std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                               bool includeContext) {
    //Do not check Node::passesThroughInputs since this does not create StateUpdate elements

    return false;
}

bool ThreadCrossingFIFO::canExpand() {
    return false;
}

void ThreadCrossingFIFO::initializeVarIfNotAlready(std::shared_ptr<Node> node, std::vector<Variable> &vars, std::vector<bool> &inits, int port, std::string suffix){
    //If not yet initialized, initialize
    if(!inits[port]){
        //Set name
        std::string varName = node->getName()+"_n"+GeneralHelper::to_string(node->getId())+ "_p" + GeneralHelper::to_string(port) +"_" +suffix;
        vars[port].setName(varName);

        //Set datatype
        std::set<std::shared_ptr<Arc>> arcSet = node->getInputPortCreateIfNot(port)->getArcs();
        if(arcSet.empty()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Error when initializing FIFO variable.  Unable to get datatype since port " + GeneralHelper::to_string(port) + " not connected", node));
        }

        //FIFO can exist in a domain which does not have a sub-blocking length of 1.  The I/O is scaled by the sub-blocking length
        //We need the base type without the sub-blocking
        DataType dt = (*arcSet.begin())->getDataType();
        std::vector<int> dims = dt.getDimensions();
        std::shared_ptr<ThreadCrossingFIFO> asThreadCrossingFIFO = std::dynamic_pointer_cast<ThreadCrossingFIFO>(node);
        //TODO: Refactor
        if(asThreadCrossingFIFO == nullptr){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Expected ThreadCrossingFIFO when initializing FIFO Variable", node));
        }
        std::vector<int> baseDims = dims; //Can get base type from either input or output, will get from input
        int subBlockSize = asThreadCrossingFIFO->getSubBlockSizeInCreateIfNot(port);
        if(subBlockSize > 1) { //Only do this reduction if the sub-block size is not 1.  Otherwise, the base type is the I/O type
            baseDims = BlockingHelpers::blockingDomainDimensionReduce(dims, subBlockSize, 1);
        }
        dt.setDimensions(baseDims);

        //Do not scale for block size.  This is done at the boarder of the compute function (ie. in the structure definition)
        vars[port].setDataType(dt);

        inits[port] = true;
    }
}

void ThreadCrossingFIFO::initializeVarIfNotAlready(std::shared_ptr<Node> node, Variable &var, bool &init, std::string suffix){
    //If not yet initialized, initialize
    if(!init){
        //Set name
        std::string varName = node->getName() + "_n" + GeneralHelper::to_string(node->getId()) + "_" +suffix;
        var.setName(varName);

        //Do not scale for block size.  This is done at the boarder of the compute function (ie. in the structure definition)

        init = true;
    }
}

std::vector<std::string> ThreadCrossingFIFO::getCBlockIndexExprsInput() const {
    return cBlockIndexExprsInput;
}

void ThreadCrossingFIFO::setCBlockIndexExprsInput(const std::vector<std::string> &cBlockIndexExprsInput) {
    ThreadCrossingFIFO::cBlockIndexExprsInput = cBlockIndexExprsInput;
}

std::string ThreadCrossingFIFO::getCBlockIndexExprInputCreateIfNot(int portNum){
    unsigned long portLen = cBlockIndexExprsInput.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        cBlockIndexExprsInput.push_back("");
    }

    return cBlockIndexExprsInput[portNum];
}

void ThreadCrossingFIFO::setCBlockIndexExprInput(int portNum, const std::string &cBlockIndexExprInput){
    unsigned long portLen = cBlockIndexExprsInput.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        cBlockIndexExprsInput.push_back("");
    }

    cBlockIndexExprsInput[portNum] = cBlockIndexExprInput;
}

std::vector<std::string> ThreadCrossingFIFO::getCBlockIndexExprsOutput() const {
    return cBlockIndexExprsOutput;
}

void ThreadCrossingFIFO::setCBlockIndexExprsOutput(const std::vector<std::string> &cBlockIndexExprsOutput) {
    ThreadCrossingFIFO::cBlockIndexExprsOutput = cBlockIndexExprsOutput;
}

std::string ThreadCrossingFIFO::getCBlockIndexExprOutputCreateIfNot(int portNum){
    unsigned long portLen = cBlockIndexExprsOutput.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        cBlockIndexExprsOutput.push_back("");
    }

    return cBlockIndexExprsOutput[portNum];
}

void ThreadCrossingFIFO::setCBlockIndexExprOutput(int portNum, const std::string &cBlockIndexExprOutput){
    unsigned long portLen = cBlockIndexExprsOutput.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        cBlockIndexExprsOutput.push_back("");
    }

    cBlockIndexExprsOutput[portNum] = cBlockIndexExprOutput;
}

std::string ThreadCrossingFIFO::getFIFOStructTypeName(){
    return name+"_n"+GeneralHelper::to_string(id) + "_t";
}

std::string ThreadCrossingFIFO::createFIFOStruct(){
    std::string typeName = getFIFOStructTypeName();

    //TODO: Check 2D case
    std::string structStr = "typedef struct {\n";
    for(int i = 0; i<inputPorts.size(); i++) {
        DataType stateDT = getCStateVar(i).getDataType();

        //Expand the data type for the block size.  Note that this does not propagate outside of this function
        int blockSize = getBlockSizeCreateIfNot(i);
        DataType blockStateDT = stateDT.expandForBlock(blockSize);

        //Create a comment for the src and dst
        std::shared_ptr<OutputPort> src = (*inputPorts[i]->getArcs().begin())->getSrcPort();
        structStr += "//From Orig: " + src->getParent()->getFullyQualifiedOrigName() + " [Port " + GeneralHelper::to_string(src->getPortNum()) + "]\n";
        structStr += "//From: " + src->getParent()->getFullyQualifiedName() + " [Port " + GeneralHelper::to_string(src->getPortNum()) + "]\n";

        //There are possibly 2 entries per port
        structStr += stateDT.toString(DataType::StringStyle::C) + " port" + GeneralHelper::to_string(i) + "_real" +
                     blockStateDT.dimensionsToString(true) + ";\n";

        if (stateDT.isComplex()) {
            structStr += stateDT.toString(DataType::StringStyle::C) + " port" + GeneralHelper::to_string(i) + "_imag" +
                         blockStateDT.dimensionsToString(true) + ";\n";
        }
    }
    structStr += "} " + typeName + ";";
    return structStr;
}

std::vector<std::shared_ptr<ClockDomain>> ThreadCrossingFIFO::getClockDomains() const {
    return clockDomains;
}

void ThreadCrossingFIFO::setClockDomains(const std::vector<std::shared_ptr<ClockDomain>> &clockDomains) {
    ThreadCrossingFIFO::clockDomains = clockDomains;
}

std::shared_ptr<ClockDomain> ThreadCrossingFIFO::getClockDomainCreateIfNot(int portNum){
    unsigned long portLen = clockDomains.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        clockDomains.push_back(nullptr);
    }

    return clockDomains[portNum];
}
void ThreadCrossingFIFO::setClockDomain(int portNum, std::shared_ptr<ClockDomain> clockDomain){
    unsigned long portLen = clockDomains.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        clockDomains.push_back(nullptr);
    }

    clockDomains[portNum] = clockDomain;
}

int ThreadCrossingFIFO::getTotalBlockSizeAllPorts() {
    int elements = 0;
    for(int blkSize : blockSizes){
        elements += blkSize;
    }

    return elements;
}

ThreadCrossingFIFOParameters::CopyMode ThreadCrossingFIFO::getCopyMode() const {
    return copyMode;
}

void ThreadCrossingFIFO::setCopyMode(ThreadCrossingFIFOParameters::CopyMode copyMode) {
    ThreadCrossingFIFO::copyMode = copyMode;
}
