//
// Created by Christopher Yarp on 2019-03-15.
//

#include <General/GeneralHelper.h>
#include "TappedDelay.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"

#define CIRC_BUFF_TYPE CircularBufferType::DOUBLE_LEN

TappedDelay::TappedDelay() : Delay(){
    //Override the circular buffer type
    setCircularBufferType(CIRC_BUFF_TYPE);
}

TappedDelay::TappedDelay(std::shared_ptr<SubSystem> parent) : Delay(parent) {
    //Override the circular buffer type
    setCircularBufferType(CIRC_BUFF_TYPE);
}

TappedDelay::TappedDelay(std::shared_ptr<SubSystem> parent, TappedDelay *orig) : Delay(parent, orig) {

}

std::shared_ptr<TappedDelay>
TappedDelay::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                               std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<TappedDelay> newNode = NodeFactory::createNode<TappedDelay>(parent);

    //Import Delay base properties
    if(dataKeyValueMap.at("block_function") == "VectorTappedDelay" || dataKeyValueMap.at("block_function") == "TappedDelayWithReset") {
        populatePropertiesFromGraphML(newNode, "Numeric.Depth",
                                      "Numeric.InitCond", id, name,
                                      dataKeyValueMap, parent, dialect);
    }else{
        populatePropertiesFromGraphML(newNode, "Numeric.NumDelays",
                                      "Numeric.vinit", id, name,
                                      dataKeyValueMap, parent, dialect);
    }

    //Override the TappedDelay specific properties (earliestFirst, allocateExtraSpace)
    if (dialect == GraphMLDialect::VITIS) {
        std::string earliestFirstStr = dataKeyValueMap.at("EarliestFirst");
        newNode->earliestFirst = GeneralHelper::parseBool(earliestFirstStr);
        std::string allocateExtraSpaceStr = dataKeyValueMap.at("IncludeCurrent");
        newNode->allocateExtraSpace = GeneralHelper::parseBool(allocateExtraSpaceStr);
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        if(dataKeyValueMap.at("block_function") == "VectorTappedDelay" || dataKeyValueMap.at("block_function") == "TappedDelayWithReset"){
            newNode->earliestFirst = false;
            newNode->allocateExtraSpace = true;
        }else {
            std::string orderStr = dataKeyValueMap.at("DelayOrder");
            if (orderStr == "Newest") {
                newNode->earliestFirst = true;
            } else if (orderStr == "Oldest") {
                newNode->earliestFirst = false;
            } else {
                throw std::runtime_error(
                        ErrorHelpers::genErrorStr("Unknown DelayOrder: " + orderStr + " - TappedDelay", newNode));
            }

            std::string includeCurrentStr = dataKeyValueMap.at("includeCurrent");
            if(includeCurrentStr == "on"){
                newNode->allocateExtraSpace = true;
            }else if(includeCurrentStr == "off"){
                newNode->allocateExtraSpace = false;
            }else{
                throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown includeCurrent: " + orderStr + " - TappedDelay", newNode));
            }
        }
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - TappedDelay", newNode));
    }

    return newNode;
}

bool TappedDelay::hasCombinationalPath() {
    return allocateExtraSpace;
}

std::set<GraphMLParameter> TappedDelay::graphMLParameters() {
    std::set<GraphMLParameter> parameters = Delay::graphMLParameters();

    parameters.insert(GraphMLParameter("EarliestFirst", "boolean", true));
    parameters.insert(GraphMLParameter("IncludeCurrent", "boolean", true));

    return parameters;
}

std::string TappedDelay::typeNameStr() {
    return "TappedDelay";
}

std::string TappedDelay::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() +
             "\nDelayLength: " + GeneralHelper::to_string(delayValue) +
             "\nInitialCondition: " + NumericValue::toString(initCondition) +
             "\nEarliestFirst: " + GeneralHelper::to_string(earliestFirst) +
             "\nIncludeCurrent: " + GeneralHelper::to_string(allocateExtraSpace);

    return label;
}

std::shared_ptr<Node> TappedDelay::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<TappedDelay>(parent, this);
}

xercesc::DOMElement *
TappedDelay::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "TappedDelay");

    //This emits the core Delay parameters
    emitGraphMLDelayParams(doc, thisNode);

    //Emit the TappedDelay Specific Parameters
    GraphMLHelper::addDataNode(doc, thisNode, "EarliestFirst", GeneralHelper::to_string(earliestFirst));

    GraphMLHelper::addDataNode(doc, thisNode, "IncludeCurrent", GeneralHelper::to_string(allocateExtraSpace));

    return thisNode;
}

void TappedDelay::validate() {
    Node::validate();

    //Should have 1 input ports and 1 output port
    if(inputPorts.size() >2){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - TappedDelay - Should Have 1 Input Port and, Optionally, 1 Reset Port", getSharedPointer()));
    }

    if(inputPorts.size() == 2 && !allocateExtraSpace){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - TappedDelay - Async Reset Ports are only supported with Tapped Delays which Pass Through The Current Value", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - TappedDelay - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    if(delayValue <1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - TappedDelay - Requires a Delay of at least 1", getSharedPointer()));
    }

    //Check that input port and the output port have the same type
    DataType outType = getOutputPort(0)->getDataType();
    DataType inType = getInputPort(0)->getDataType();

    DataType outTypeDim1 = outType;
    outTypeDim1.setDimensions({1});
    DataType inTypeDim1 = inType;
    inTypeDim1.setDimensions({1});

    if(inputPorts.size() >1){
        DataType rstType = getInputPort(1)->getDataType();
        if(!rstType.isScalar()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - TappedDelay - Async Reset Ports Should be Scalar", getSharedPointer()));
        }

        if(!rstType.isBool()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - TappedDelay - Async Reset Ports Should be Bool Type", getSharedPointer()));
        }

        if(rstType.isComplex()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - TappedDelay - Async Reset Ports Should be A Real Type", getSharedPointer()));
        }
    }

    if(outTypeDim1 != inTypeDim1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - TappedDelay - DataType of Input Port Does not Match Output Port", getSharedPointer()));
    }

    int delayLen = delayValue;
    if(allocateExtraSpace){
        delayLen++;
    }
    if (delayLen != outType.numberOfElements()/inType.numberOfElements()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - TappedDelay - Output must be the length of the delay (+1 if current value passed)", getSharedPointer()));
    }

    if(usesCircularBuffer()){
        int expectedInitCondSize = getBufferAllocatedLen()*inType.numberOfElements();
        int initConditionSize = initCondition.size();
        if (initConditionSize != expectedInitCondSize) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "Validation Failed - TappedDelay - Number of initial conditions does not match the allocated size of the circular buffer",
                    getSharedPointer()));
        }
    }else {
        int expectedInitCondSize = outType.numberOfElements();
        int initConditionSize = initCondition.size();
        if (initConditionSize != expectedInitCondSize) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "Validation Failed - TappedDelay - Number of initial conditions must match the number of elements in the output when operating in shift register mode",
                    getSharedPointer()));
        }
    }
}

CExpr
TappedDelay::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                       bool imag) {

    if(usesCircularBuffer()){
        //Need to pass the current value through if requested.  It is inserted into the extra slot
        //Instead of inserting from the next state variable, we are inserting from the input (see below)
        if(allocateExtraSpace) {
            std::shared_ptr<OutputPort> srcPort = getInputPort(0)->getSrcOutputPort();
            int srcOutPortNum = srcPort->getPortNum();
            std::shared_ptr<Node> srcNode = srcPort->getParent();

            CExpr inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, imag);

            //Reset buffer here if reset pulled
            //Also reset pointer locations so that initial state is properly aligned
            bool hasRst = inputPorts.size()>1;
            if(hasRst){
                std::shared_ptr<OutputPort> rstPort = getInputPort(1)->getSrcOutputPort();
                int rstOutPortNum = rstPort->getPortNum();
                std::shared_ptr<Node> rstNode = rstPort->getParent();
                CExpr rstExpr = rstNode->emitC(cStatementQueue, schedType, rstOutPortNum, false);

                //Emit the reset check
                cStatementQueue.push_back("if(" + rstExpr.getExpr() + "){");
                //Reset the state vars
                std::vector<std::string> bufferRst = cStateVar.genReset();
                cStatementQueue.insert(cStatementQueue.end(), bufferRst.begin(), bufferRst.end());
                std::vector<std::string> offsetRst = circularBufferOffsetVar.genReset();
                cStatementQueue.insert(cStatementQueue.end(), offsetRst.begin(), offsetRst.end());
                cStatementQueue.push_back("}else{");
                //Else, perform the standard emit where the current input is pulled into the buffer
            }

            //For both the earliest first and oldest first versions of tapped delay, the circular buffer is the location to write into
            std::string insertPosition = circularBufferOffsetVar.getCVarName(false);

            //The function should handle the case where where circular buffers with extra elements are in use
            assignInputToBuffer(inputExpr, insertPosition, imag, cStatementQueue);

            if(hasRst){
                //Close the reset scope
                cStatementQueue.push_back("}");
            }
        }

        //Return the variable
        int fifoLen = delayValue;
        if(allocateExtraSpace){
            fifoLen++;
        }

        if(circularBufferType == CircularBufferType::NO_EXTRA_LEN) {
            if(earliestFirst) {
                //The most recent value is at the circular buffer cursor position and that is what should be presented first
                return CExpr(cStateVar.getCVarName(imag), getBufferLength(),
                             circularBufferOffsetVar.getCVarName(false));
            }else{
                //The most revent value is in the circular buffer cursor position.  However, we want to present the oldest value first
                //Even though this says there is no extra length allocated, the allocated size can be rounded up to a power of 2.
                //We want to return the current cursor position - (fifo len -1).  However, we want to avoid wraparound.
                //Note: 0%3=0, (2^8-1)%3 = 255%3 = 0.  So, unless the mod is by a power of 2, we cannot simply live with the wrap
                //We, however, can add the buffer length again.

                //Get an unsigned type that can work with 2x the max index
                int numBitsReq = GeneralHelper::numIntegerBits(getBufferLength()*2-1, false);
                DataType offsetDt = circularBufferOffsetVar.getDataType();
                offsetDt.setTotalBits(numBitsReq);
                offsetDt = offsetDt.getCPUStorageType();

                return CExpr(cStateVar.getCVarName(imag), getBufferLength(),
                             "((" + offsetDt.toString(DataType::StringStyle::C, false, false) + ")"
                             + circularBufferOffsetVar.getCVarName(false) + "+" + GeneralHelper::to_string(getBufferLength()-(fifoLen-1)) +
                             ")");
            }
        }else{
            if(earliestFirst){
                //The returned array is simply the allocated array starting at the current offset of the circular buffer
                return CExpr("(" + cStateVar.getCVarName(imag) + "+" + circularBufferOffsetVar.getCVarName(false) + ")", CExpr::ExprType::ARRAY);
            }else{
                return CExpr("(" + cStateVar.getCVarName(imag) + "+" +
                                        circularBufferOffsetVar.getCVarName(false) + "-" +
                                        GeneralHelper::to_string(fifoLen-1) + ")", CExpr::ExprType::ARRAY);
            }
        }
    }else {
        //We need to pass the current value through if requested
        if (allocateExtraSpace) {
            std::shared_ptr<OutputPort> srcPort = getInputPort(0)->getSrcOutputPort();
            int srcOutPortNum = srcPort->getPortNum();
            std::shared_ptr<Node> srcNode = srcPort->getParent();

            CExpr inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, imag);

            //Reset buffer here if reset pulled
            bool hasRst = inputPorts.size()>1;
            if(hasRst){
                std::shared_ptr<OutputPort> rstPort = getInputPort(1)->getSrcOutputPort();
                int rstOutPortNum = rstPort->getPortNum();
                std::shared_ptr<Node> rstNode = rstPort->getParent();
                CExpr rstExpr = rstNode->emitC(cStatementQueue, schedType, rstOutPortNum, false);

                //Emit the reset check
                cStatementQueue.push_back("if(" + rstExpr.getExpr() + "){");
                //Reset the state vars
                std::vector<std::string> bufferRst = cStateVar.genReset();
                cStatementQueue.insert(cStatementQueue.end(), bufferRst.begin(), bufferRst.end());
                cStatementQueue.push_back("}else{");
                //Else, perform the standard emit where the current input is pulled into the buffer
            }

            //It is possible for the input to be a vector, emit a for loop if nessasary
            DataType inputDT = getInputPort(0)->getDataType();

            //=== Open for Loop ===
            std::vector<std::string> forLoopIndexVars;
            std::vector<std::string> forLoopClose;
            if (!inputDT.isScalar()) {
                std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                        EmitterHelpers::generateVectorMatrixForLoops(inputDT.getDimensions());

                std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
                forLoopIndexVars = std::get<1>(forLoopStrs);
                forLoopClose = std::get<2>(forLoopStrs);

                cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());
            }

            //Because there is a minimum delay of 1, the state variable has at least 2 elements (and can be dereferenced)
            std::string assignTo = cStateVar.getCVarName(imag) +
                                   (earliestFirst ? "[0]" : "[" + GeneralHelper::to_string(delayValue) +
                                                            "]"); //Note not delayValue-1 because the array was extended by 1

            std::string assignToDeref =
                    assignTo + (inputDT.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars));
            std::vector<std::string> emptyArr;
            std::string assignFromDeref = inputExpr.getExprIndexed(inputDT.isScalar() ? emptyArr : forLoopIndexVars,
                                                                   true);

            cStatementQueue.push_back(assignToDeref + " = " + assignFromDeref + ";");

            //=== Close for Loop ===
            if (!inputDT.isScalar()) {
                cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
            }

            if(hasRst){
                //Close the reset scope
                cStatementQueue.push_back("}");
            }
        }

        //Return the variable without any dereferencing
        return CExpr(cStateVar.getCVarName(imag), CExpr::ExprType::ARRAY);
    }
}

bool TappedDelay::hasInternalFanout(int inputPort, bool imag) {
    //The input is used more than once by this node, once for computing the input for the next state,
    //and once for computing the passthough value
    //TODO: Change this so that it is not nessisary to use the value more than once.
    if(allocateExtraSpace && inputPort == 0){
        return true;
    }
    return Node::hasInternalFanout(inputPort, imag);
}

std::vector<NumericValue> TappedDelay::getExportableInitConds() {
    //Do not reverse initial conditions in TappedDelay
    return getExportableInitCondsHelper();
}

void TappedDelay::propagateProperties() {
    //Do not reverse initial conditions in TappedDelay
    propagatePropertiesHelper();
}

void TappedDelay::emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                   std::shared_ptr<StateUpdate> stateUpdateSrc) {

    //Emit check for reset.  If reset, do not do the state update.  The state was reset in emitCExpr
    bool hasRst = inputPorts.size()>1;
    if(hasRst){
        std::shared_ptr<OutputPort> rstPort = getInputPort(1)->getSrcOutputPort();
        int rstOutPortNum = rstPort->getPortNum();
        std::shared_ptr<Node> rstNode = rstPort->getParent();
        CExpr rstExpr = rstNode->emitC(cStatementQueue, schedType, rstOutPortNum, false);

        //Emit the reset check
        cStatementQueue.push_back("if(!(" + rstExpr.getExpr() + ")){");
        //Perform the state update if
    }

    if(usesCircularBuffer() && !earliestFirst){
        if(requiresStandaloneCExprNextState()) { //Includes check for allocateExtraSpace
            assignInputToBuffer(circularBufferOffsetVar.getCVarName(false), cStatementQueue);
        }else{
            cStatementQueue.push_back("//Explicit next state assignment for " + getFullyQualifiedName() +
                                      " [ID: " + GeneralHelper::to_string(id) + ", Type:" + typeNameStr() +
                                      "] is not needed because a combinational path exists through the node and the input has already been captured in the Delay buffer");
        }
        incrementAndWrapCircularBufferOffset(cStatementQueue);
    }else{
        Delay::emitCStateUpdate(cStatementQueue, schedType, stateUpdateSrc);
    }

    if(hasRst){
        //Close the reset scope
        cStatementQueue.push_back("}");
    }
}

void TappedDelay::incrementAndWrapCircularBufferOffset(std::vector<std::string> &cStatementQueue) {
    if(usesCircularBuffer() && !earliestFirst) {
        if(circularBufferType == CircularBufferType::DOUBLE_LEN) {
            //Use mod method
            cStatementQueue.push_back(circularBufferOffsetVar.getCVarName(false) +
                                      "=((" + circularBufferOffsetVar.getCVarName(false) + "+1)%" +
                                      GeneralHelper::to_string(getBufferLength()) + ")" + "+" +
                                      GeneralHelper::to_string(getBufferLength()) + ";");
        }else if(circularBufferType == CircularBufferType::PLUS_DELAY_LEN_M1){
            //Do not use the mod method
            cStatementQueue.push_back("if(" + circularBufferOffsetVar.getCVarName(false) + ">= " + GeneralHelper::to_string(getBufferAllocatedLen()-1) + "){");
            // Let delay len (with extra element) = 3.  We will allocate 3-1=2 extra slots before
            // extra extra orig orig orig
            // 0     1     2    3    4
            // The pointer should be set to the first orig, which is the delay (plus 1 if passthrough) - 1
            cStatementQueue.push_back("\t" + circularBufferOffsetVar.getCVarName(false) + "=" + GeneralHelper::to_string(getBufferLength()-1) + ";");
            cStatementQueue.push_back("}else{");
            cStatementQueue.push_back("\t" + circularBufferOffsetVar.getCVarName(false) + "++" + ";");
            cStatementQueue.push_back("}");
        }else if(circularBufferType == CircularBufferType::NO_EXTRA_LEN){
            //Use the standard method for delay
            Delay::incrementAndWrapCircularBufferOffset(cStatementQueue);
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown Circular Buffer Type", getSharedPointer()));
        }
    }else{
        Delay::incrementAndWrapCircularBufferOffset(cStatementQueue);
    }
}

int TappedDelay::getCircBufferInitialIdx() {
    if(usesCircularBuffer() && !earliestFirst) {
        //For the versions with extra length, we initialize the circular buffer cursor to be in the second segment of the array
        if(circularBufferType == CircularBufferType::DOUBLE_LEN) {
            return getBufferLength();
        }else if(circularBufferType == CircularBufferType::PLUS_DELAY_LEN_M1){
            return getBufferLength()-1;
        }else if(circularBufferType == CircularBufferType::NO_EXTRA_LEN){
            return Delay::getCircBufferInitialIdx();
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown Circular Buffer Type", getSharedPointer()));
        }
    }else{
        return Delay::getCircBufferInitialIdx();
    }

}
