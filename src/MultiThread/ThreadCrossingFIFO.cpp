//
// Created by Christopher Yarp on 9/3/19.
//

#include <General/GeneralHelper.h>
#include "ThreadCrossingFIFO.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"

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

Variable ThreadCrossingFIFO::getCStateInputVar(int port) {
    while(port >= cStateInputVars.size()){
        cStateInputVars.push_back(Variable());
        cStateInputVarsInitialized.push_back(false);
    }

    ThreadCrossingFIFO::initializeVarIfNotAlready(getSharedPointer(), cStateInputVars, cStateInputVarsInitialized, port, "dst");

    return cStateInputVars[port];
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

ThreadCrossingFIFO::ThreadCrossingFIFO() : fifoLength(8){}

ThreadCrossingFIFO::ThreadCrossingFIFO(std::shared_ptr<SubSystem> parent) : Node(parent), fifoLength(8){}

ThreadCrossingFIFO::ThreadCrossingFIFO(std::shared_ptr<SubSystem> parent, ThreadCrossingFIFO *orig) : Node(parent, orig),
                                       fifoLength(orig->fifoLength), initConditions(orig->initConditions),
                                       cStateVars(orig->cStateVars), cStateInputVars(orig->cStateInputVars),
                                       blockSizes(orig->blockSizes), cStateVarsInitialized(orig->cStateVarsInitialized),
                                       cStateInputVarsInitialized(orig->cStateInputVarsInitialized){}

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
        DataType srcDT = (*inArcs.begin())->getDataType();
        DataType dstDT = (*outArcs.begin())->getDataType();

        if(srcDT != dstDT){
            throw std::runtime_error(
                ErrorHelpers::genErrorStr("Validation Failed - ThreadCrossingFIFO - Port " + GeneralHelper::to_string(portNum) + " input and output datatypes should match",
                                      getSharedPointer()));
        }

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

        if (getInitConditionsCreateIfNot(portNum).size() > (fifoLength * getBlockSizeCreateIfNot(portNum)*getInputPort(portNum)->getDataType().numberOfElements() - getBlockSizeCreateIfNot(portNum)/getInputPort(portNum)->getDataType().numberOfElements())) { // - blockSize because we need to be able to write 1 value into the FIFO to ensure deadlock cannot occur
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "Validation Failed - ThreadCrossingFIFO - The number of initial conditions cannot be larger than the FIFO - 1 block",
                    getSharedPointer()));
        }

        if (getInitConditionsCreateIfNot(portNum).size() % (getBlockSizeCreateIfNot(portNum)*getInputPort(portNum)->getDataType().numberOfElements()) != 0) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "Validation Failed - ThreadCrossingFIFO - Initial Conditions for Port " + GeneralHelper::to_string(portNum) + " must be a multiple of its block size (" + GeneralHelper::to_string(getBlockSizeCreateIfNot(portNum)) + ")",
                    getSharedPointer()));
        }

        if(portNum != 0){
            if(getInitConditionsCreateIfNot(portNum).size()/getBlockSizeCreateIfNot(portNum)/getInputPort(portNum)->getDataType().numberOfElements() != getInitConditionsCreateIfNot(0).size()/getBlockSizeCreateIfNot(0)/getInputPort(0)->getDataType().numberOfElements()){
                throw std::runtime_error(ErrorHelpers::genErrorStr(
                        "Validation Failed - ThreadCrossingFIFO - All ports must have the same number of initial conditions",
                        getSharedPointer()));
            }
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
    if(getBlockSizeCreateIfNot(outputPortNum) > 1){
        //Because of C multidimensional array semantics, and because the added dimension for blocks >1 is prepended to
        //the dimensions, indexing the first dimension will return the correct value.  If the data type is a scalar, it
        //returns a scalar value for the given block.  If the data type is a vector or matrix, this will still return a
        //a pointer but a pointer to the correct block.
        expr = "(" +  getCStateVar(outputPortNum).getCVarName(imag) + "[" + getCBlockIndexVarInputNameCreateIfNot(outputPortNum) + "])";
    }else{
        //The block size is 1, just return the state variable.  No indexing based on the current block is required
        expr = getCStateVar(outputPortNum).getCVarName(imag);
    }

    //Will output as a variable even though we index into the block.  Will assume the indexing is relatively heap.
    return CExpr(expr, getCStateVar(outputPortNum).getDataType().isScalar() ? CExpr::ExprType::SCALAR_VAR : CExpr::ExprType::ARRAY);
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

        //TODO: Implement Vector Support (need to loop over input variable indexes (will be stored as a variable due to default behavior of internal fanout

        //TODO:If type is a vector, emit a for loop
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
        std::string cBlockIndexVarOutputName = getCBlockIndexVarOutputNameCreateIfNot(i);

        std::vector<std::string> emptyArr;
        std::string stateInputDeclAssignRe =
                ((blockSize == 1 && inputDataType.isScalar()) ? "*" : "") + //Need to dereference the state variable if the block size is 1 and the type is a scalar
                getCStateInputVar(i).getCVarName(false) +
                ((blockSize > 1) ? "[" + cBlockIndexVarOutputName + "]" : "") + //Index into the block
                (inputDataType.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars)) + //Index into the vector/matrix
                " = " + inputExprRe.getExprIndexed(inputDataType.isScalar() ? emptyArr : forLoopIndexVars, true) + ";"; //Index into the vector/matrix
        cStatementQueue.push_back(stateInputDeclAssignRe);
        if (inputDataType.isComplex()) {
            std::string stateInputDeclAssignIm =
            ((blockSize == 1 && inputDataType.isScalar()) ? "*" : "") + //Need to dereference the state variable if the block size is 1 and the type is a scalar
            getCStateInputVar(i).getCVarName(true) +
            ((blockSize > 1) ? "[" + cBlockIndexVarOutputName + "]" : "") + //Index into the block
            (inputDataType.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars)) + //Index into the vector/matrix
            " = " + inputExprIm.getExprIndexed(inputDataType.isScalar() ? emptyArr : forLoopIndexVars, true) + ";"; //Index into the vector/matrix
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
            throw std::runtime_error(ErrorHelpers::genErrorStr("Error when initializing FIFO variable.  Unable to get datatype since input port " + GeneralHelper::to_string(port) + " not connected", node));
        }
        DataType dt = (*arcSet.begin())->getDataType();

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

std::vector<std::string> ThreadCrossingFIFO::getCBlockIndexVarInputNames() const {
    return cBlockIndexVarInputNames;
}

void ThreadCrossingFIFO::setCBlockIndexVarInputNames(const std::vector<std::string> &cBlockIndexVarNames) {
    ThreadCrossingFIFO::cBlockIndexVarInputNames = cBlockIndexVarNames;
}

std::string ThreadCrossingFIFO::getCBlockIndexVarInputNameCreateIfNot(int portNum){
    unsigned long portLen = cBlockIndexVarInputNames.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        cBlockIndexVarInputNames.push_back("");
    }

    return cBlockIndexVarInputNames[portNum];
}

void ThreadCrossingFIFO::setCBlockIndexVarInputName(int portNum, const std::string &cBlockIndexVarName){
    unsigned long portLen = cBlockIndexVarInputNames.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        cBlockIndexVarInputNames.push_back("");
    }

    cBlockIndexVarInputNames[portNum] = cBlockIndexVarName;
}

std::vector<std::string> ThreadCrossingFIFO::getCBlockIndexVarOutputNames() const {
    return cBlockIndexVarOutputNames;
}

void ThreadCrossingFIFO::setCBlockIndexVarOutputNames(const std::vector<std::string> &cBlockIndexVarNames) {
    ThreadCrossingFIFO::cBlockIndexVarOutputNames = cBlockIndexVarNames;
}

std::string ThreadCrossingFIFO::getCBlockIndexVarOutputNameCreateIfNot(int portNum){
    unsigned long portLen = cBlockIndexVarOutputNames.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        cBlockIndexVarOutputNames.push_back("");
    }

    return cBlockIndexVarOutputNames[portNum];
}

void ThreadCrossingFIFO::setCBlockIndexVarOutputName(int portNum, const std::string &cBlockIndexVarName){
    unsigned long portLen = cBlockIndexVarOutputNames.size();
    for(unsigned long i = portLen; i <= portNum; i++){
        cBlockIndexVarOutputNames.push_back("");
    }

    cBlockIndexVarOutputNames[portNum] = cBlockIndexVarName;
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
