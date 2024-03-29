//
// Created by Christopher Yarp on 4/29/20.
//

#include "DownsampleClockDomain.h"
#include "RateChange.h"
#include "General/GeneralHelper.h"
#include "General/ErrorHelpers.h"
#include "MultiRateHelpers.h"
#include "GraphMLTools/GraphMLHelper.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/ContextHelper.h"
#include "PrimitiveNodes/WrappingCounter.h"
#include "RepeatOutput.h"

DownsampleClockDomain::DownsampleClockDomain() : contextDriver(nullptr) {

}

DownsampleClockDomain::DownsampleClockDomain(std::shared_ptr<SubSystem> parent) : ClockDomain(parent), ContextRoot(), contextDriver(nullptr) {

}

DownsampleClockDomain::DownsampleClockDomain(std::shared_ptr<SubSystem> parent, DownsampleClockDomain *orig) : ClockDomain(parent, orig), contextDriver(nullptr) {

}

std::shared_ptr<Arc> DownsampleClockDomain::getContextDriver() const {
    return contextDriver;
}

void DownsampleClockDomain::setContextDriver(const std::shared_ptr<Arc> &contextDriver) {
    DownsampleClockDomain::contextDriver = contextDriver;
}

void DownsampleClockDomain::validate() {
    ClockDomain::validate();

    std::shared_ptr<DownsampleClockDomain> thisCast = std::static_pointer_cast<DownsampleClockDomain>(getSharedPointer());
    MultiRateHelpers::validateSpecialiedClockDomain(thisCast);
}

std::string DownsampleClockDomain::typeNameStr() {
    return "DownsampleClockDomain";
}

xercesc::DOMElement *DownsampleClockDomain::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode,
                                                        bool include_block_node_type) {
    //Performs the same function as the Subsystem emit GraphML except that the block type is changed

    //Emit the basic info for this node
    xercesc::DOMElement *thisNode = emitGraphMLBasics(doc, graphNode);

    if (include_block_node_type){
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "DownsampleClockDomain");
    }

    emitGramphMLSubgraphAndChildren(doc, thisNode);

    return thisNode;
}

std::shared_ptr<Node> DownsampleClockDomain::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<DownsampleClockDomain>(parent, this);
}

void DownsampleClockDomain::shallowCloneWithChildren(std::shared_ptr<SubSystem> parent,
                                                     std::vector<std::shared_ptr<Node>> &nodeCopies,
                                                     std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode,
                                                     std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode) {
    //Copy this node
    std::shared_ptr<DownsampleClockDomain> clonedNode = std::dynamic_pointer_cast<DownsampleClockDomain>(shallowClone(parent));

    ClockDomain::shallowCloneWithChildrenWork(clonedNode, nodeCopies, origToCopyNode, copyToOrigNode);
}

std::vector<std::shared_ptr<Node>> DownsampleClockDomain::discoverAndMarkContexts(std::vector<Context> contextStack) {
    //Return all nodes in context (including ones from recursion)
    std::shared_ptr<DownsampleClockDomain> thisAsDownsampleDomain = std::static_pointer_cast<DownsampleClockDomain>(getSharedPointer());

    return ContextHelper::discoverAndMarkContexts_SubsystemContextRoots(contextStack, thisAsDownsampleDomain);
}

bool DownsampleClockDomain::isSpecialized() {
    return true;
}

void DownsampleClockDomain::orderConstrainZeroInputNodes(std::vector<std::shared_ptr<Node>> predecessorNodes,
                                                         std::vector<std::shared_ptr<Node>> &new_nodes,
                                                         std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                         std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                         std::vector<std::shared_ptr<Arc>> &deleted_arcs) {

    throw std::runtime_error(ErrorHelpers::genErrorStr("orderConstrainZeroInputNodes has been depricated and is not supported by ClockDomains", getSharedPointer()));

    //This function has been depricated in favor of a function which order constrains nodes with zero input that are in contexts
    //This new function is aware of partitions and is Design::orderConstrainZeroInputNodes
}

bool DownsampleClockDomain::shouldReplicateContextDriver() {
    if(useVectorSamplingMode){
        return false;
    }else {
        return true;
    }
}

int DownsampleClockDomain::getNumSubContexts() const {
    return 2;
}

void DownsampleClockDomain::createSupportNodes(std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                               std::vector<std::shared_ptr<Node>> &nodesToRemove,
                                               std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                               std::vector<std::shared_ptr<Arc>> &arcToRemove,
                                               bool includeContext, bool includeOutputBridgeNodes) {
    //Create a WrappingCounter as the context driver of the ClockDomain
    std::shared_ptr<WrappingCounter> driverNode = NodeFactory::createNode<WrappingCounter>(parent);
    driverNode->setName(name+"_Counter");
    driverNode->setPartitionNum(partitionNum);
    //TODO: When allowing clock domains to be split into different sub-blocking lengths, potentially change this (or replicate)
    driverNode->setBaseSubBlockingLen(baseSubBlockingLen);
    nodesToAdd.push_back(driverNode);
    driverNode->setCountTo(downsampleRatio);
    driverNode->setInitCondition(0);

    if(includeContext) {
        //Copy Context of the Downsample Domain (not of nodes inside the Downsample Domain as the driver should not be part of the domain it is driving)
        std::vector<Context> context = driverNode->getContext();
        driverNode->setContext(context);

        if (context.size() > 0) {
            //Add the driver node as a lower level contextRoot
            std::shared_ptr<ContextRoot> contextRoot = context[context.size() - 1].getContextRoot();
            int subcontext = context[context.size() - 1].getSubContext();
            contextRoot->addSubContextNode(subcontext, driverNode);
        }
    }

    //Create arc
    DataType driverArcDataType = DataType(false, false, false, 1, 0, {1});
    driverArcDataType = driverArcDataType.getCPUStorageType();

    //TODO: Fix Arc Rate, currently unused
    std::shared_ptr<Arc> driverArc = Arc::connectNodes(driverNode->getOutputPortCreateIfNot(0), getOrderConstraintInputPortCreateIfNot(), driverArcDataType);
    arcsToAdd.push_back(driverArc);
    contextDriver = driverArc;

    //Create nodes for output I/O arcs.  For the downsample subsystems, we will create RepeatOutputs since there is
    //no reason to force the output to be 0 between activations of the downsample subsystem.  Note that RepeatOutput
    //is basically the same as an EnableOutput in that it acts as a latch.  These nodes will have the partition
    //of the node they are connected to

    std::shared_ptr<DownsampleClockDomain> thisAsDownsampleClockDomain = std::static_pointer_cast<DownsampleClockDomain>(getSharedPointer());
    std::vector<Context> context;

    if(includeContext) {
        context = getContext();
        //Add the downsample clock domain to the context to be used for RepeatOutputs for I/O outputs
        context.emplace_back(thisAsDownsampleClockDomain, 0);
    }

    if(includeOutputBridgeNodes) {
        //Runs through the list of IO output ports.  Note, each port should only have 1 input arc which should be the
        //driver arc
        for(auto ioPort = ioOutput.begin(); ioPort != ioOutput.end(); ioPort++){
            std::set<std::shared_ptr<Arc>> ioArcs = (*ioPort)->getArcs();

            //TODO: Remove check
            if(ioArcs.size() != 1){
                throw std::runtime_error(ErrorHelpers::genErrorStr("While creating intermediate nodes for clock domain I/O outputs, found a flagged output that does not have exactly 1 input arc, has " + GeneralHelper::to_string(ioArcs.size()), getSharedPointer()));
            }
            std::shared_ptr<Arc> ioArc = (*ioArcs.begin());
            std::shared_ptr<Node> srcNode = ioArc->getSrcPort()->getParent();
            std::shared_ptr<RateChange> srcNodeAsRateChange = GeneralHelper::isType<Node, RateChange>(srcNode);

            bool insertBridge = false;
            if(srcNodeAsRateChange){
                //Check if the RateChange is an output and it's clock domain is this node.  If it is, then no bridge is required.  Otherwise a bridging node is requires
                if(!(MultiRateHelpers::findClockDomain(srcNode) == thisAsDownsampleClockDomain && !srcNodeAsRateChange->isInput())){
                    insertBridge = true;
                }
            }else{
                insertBridge = true;
            }

            if(insertBridge){
                //Create repeat node with this node as the parent
                std::shared_ptr<RepeatOutput> intermediateNode = NodeFactory::createNode<RepeatOutput>(thisAsDownsampleClockDomain);
                intermediateNode->setName((*ioPort)->getName() + "_ClockDomainBridge");
                intermediateNode->setPartitionNum(srcNode->getPartitionNum());
                //TODO: When allowing clock domains to be split into different sub-blocking lengths, potentially change this (or replicate)
                intermediateNode->setBaseSubBlockingLen(srcNode->getBaseSubBlockingLen());
                nodesToAdd.push_back(intermediateNode);

                //Set context if includeContext, add this node to the context stack as ContextRoots do not include themselves in their context stack
                if(includeContext){
                    intermediateNode->setContext(context);
                }

                //Rewire origional arc to repeat
                ioArc->setDstPortUpdateNewUpdatePrev(intermediateNode->getInputPortCreateIfNot(0));

                //Create new arc to ioPort
                //TODO: properly set arc rate.  Currently unused
                std::shared_ptr<Arc> newArc = Arc::connectNodes(intermediateNode->getOutputPortCreateIfNot(0), *ioPort, ioArc->getDataType());
                arcsToAdd.push_back(newArc);
            }
        }
    }
}

std::vector<std::shared_ptr<Arc>> DownsampleClockDomain::getContextDecisionDriver() {
    if(contextDriver){
        return {contextDriver};
    }
    return {};
}

std::vector<Variable> DownsampleClockDomain::getCContextVars() {
    //When operating in vector mode, repeat and upsample do not need to declare state variables (unless phase != 0 is later supported)
    //However, they do need their outputs declared outside of the clock domain
    std::vector<Variable> rateChangeExternVars;
    for(const std::shared_ptr<RateChange> &rateChange : rateChangeIn){
        std::vector<Variable> rateChangeVars = rateChange->getVariablesToDeclareOutsideClockDomain();
        rateChangeExternVars.insert(rateChangeExternVars.end(), rateChangeVars.begin(), rateChangeVars.end());
    }
    for(const std::shared_ptr<RateChange> &rateChange : rateChangeOut){
        std::vector<Variable> rateChangeVars = rateChange->getVariablesToDeclareOutsideClockDomain();
        rateChangeExternVars.insert(rateChangeExternVars.end(), rateChangeVars.begin(), rateChangeVars.end());
    }

    for(const std::shared_ptr<BlockingOutput> &ioBlockingOutput : ioBlockingOutput){
        Variable blockingVar = ioBlockingOutput->getOutputVar();
        rateChangeExternVars.insert(rateChangeExternVars.end(), blockingVar);
    }

    return rateChangeExternVars;
}


Variable DownsampleClockDomain::getCContextVar(int contextVarIndex) {
    //Do not need any additional context vars

    //Even though getCContextVars is traversing sets of the input and output rate change nodes,
    //The order should be preserved during emit time (pointers should not change)
    return getCContextVars()[contextVarIndex];
}

bool DownsampleClockDomain::requiresContiguousContextEmits() {
    //TODO: Downsample clock domains do not necessarily need to be emitted in one block
    //      However, it allows us to insert the increment of the execution count in the
    //      context close.  We could make this an explicit node but it would require
    //      replicating the node across contexts.  Refactor this at some point
    //
    //      This will not have much impact with the current scheduling scheme for
    //      multithreading as that schedules clock domains together anyway.
    if(requiresDeclaringExecutionCount()) {
        return true;
    }else{
        return false;
    }
}


void DownsampleClockDomain::emitCContextOpenFirst(std::vector<std::string> &cStatementQueue,
                                                  SchedParams::SchedType schedType, int subContextNumber,
                                                  int partitionNum) {
    //Only output of this partition's clock domain logic is not being suppressed (and being handled by adjusting the compute outer loop)
    if(!useVectorSamplingMode && suppressClockDomainLogicForPartitions.find(partitionNum) == suppressClockDomainLogicForPartitions.end()) {
        //For single threaded operation, this is simply enableDriverPort = getEnableSrc().  However, for multi-threaded emit, the driver arc may come from a FIFO (which can be different depending on the partition)
        std::vector<std::shared_ptr<Arc>> contextDrivers = getContextDriversForPartition(partitionNum);
        if (contextDrivers.empty()) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "C Emit Error - No context drivers available for the given partition: " +
                    GeneralHelper::to_string(partitionNum), getSharedPointer()));
        }

        //There are probably multiple driver arcs (since they were copied for each enabled input and output.  Just grab the first one
        std::shared_ptr<OutputPort> enableDriverPort = contextDrivers[0]->getSrcPort();
        CExpr enableDriverExpr = enableDriverPort->getParent()->emitC(cStatementQueue, schedType,
                                                                      enableDriverPort->getPortNum());
        if (enableDriverExpr.isArrayOrBuffer()) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "The driver to the downsample clock domain is expected to be a scalar expression or variable",
                    getSharedPointer()));
        }

        std::string cExpr;
        //There are actually 2 contexts for DownsampleClockDomain, 0 which is the actual clock domain, and 1 which includes StateUpdate nodes for UpsampleOutput
        if (subContextNumber == 0) {
            cExpr = "if(" + enableDriverExpr.getExpr() + "){";
        } else if (subContextNumber == 1) {
            cExpr = "if(!" + enableDriverExpr.getExpr() + "){";
        } else {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "C Emit Error - Unexpected sub-context number: " + GeneralHelper::to_string(subContextNumber),
                    getSharedPointer()));
        }

        cStatementQueue.push_back(cExpr);
    }else if(useVectorSamplingMode){
        //Will open a scope for the clock domain
        std::string cExpr = "{";
        cStatementQueue.push_back(cExpr);
    }
}

void
DownsampleClockDomain::emitCContextOpenMid(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                           int subContextNumber, int partitionNum) {
    //Only output of this partition's clock domain logic is not being suppressed (and being handled by adjusting the compute outer loop)
    if(!useVectorSamplingMode && suppressClockDomainLogicForPartitions.find(partitionNum) == suppressClockDomainLogicForPartitions.end()) {
        //For single threaded operation, this is simply enableDriverPort = getEnableSrc().  However, for multi-threaded emit, the driver arc may come from a FIFO (which can be different depending on the partition)
        std::vector<std::shared_ptr<Arc>> contextDrivers = getContextDriversForPartition(partitionNum);
        if (contextDrivers.empty()) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "C Emit Error - No context drivers available for the given partition: " +
                    GeneralHelper::to_string(partitionNum), getSharedPointer()));
        }

        //There are probably multiple driver arcs (since they were copied for each enabled input and output.  Just grab the first one
        std::shared_ptr<OutputPort> enableDriverPort = contextDrivers[0]->getSrcPort();
        CExpr enableDriverExpr = enableDriverPort->getParent()->emitC(cStatementQueue, schedType,
                                                                      enableDriverPort->getPortNum());

        if (enableDriverExpr.isArrayOrBuffer()) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "The driver to the downsample clock domain is expected to be a scalar expression or variable",
                    getSharedPointer()));
        }

        std::string cExpr;
        //There are actually 2 contexts for DownsampleClockDomain, 0 which is the actual clock domain, and 1 which includes StateUpdate nodes for UpsampleOutput
        if (subContextNumber == 0) {
            cExpr = "else if(" + enableDriverExpr.getExpr() + "){";
        } else if (subContextNumber == 1) {
            cExpr = "else if(!" + enableDriverExpr.getExpr() + "){";
        } else {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "C Emit Error - Unexpected sub-context number: " + GeneralHelper::to_string(subContextNumber),
                    getSharedPointer()));
        }

        cStatementQueue.push_back(cExpr);
    }else if(useVectorSamplingMode){
        //Will open a scope for the clock domain
        std::string cExpr = "{";
        cStatementQueue.push_back(cExpr);
    }
}

void
DownsampleClockDomain::emitCContextOpenLast(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                            int subContextNumber, int partitionNum) {
    //Only output of this partition's clock domain logic is not being suppressed (and being handled by adjusting the compute outer loop)
    if(!useVectorSamplingMode && suppressClockDomainLogicForPartitions.find(partitionNum) == suppressClockDomainLogicForPartitions.end()) {
        //For single threaded operation, this is simply enableDriverPort = getEnableSrc().  However, for multi-threaded emit, the driver arc may come from a FIFO (which can be different depending on the partition)
        std::vector<std::shared_ptr<Arc>> contextDrivers = getContextDriversForPartition(partitionNum);
        if (contextDrivers.empty()) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "C Emit Error - No context drivers available for the given partition: " +
                    GeneralHelper::to_string(partitionNum), getSharedPointer()));
        }

        std::string cExpr = "else{";
        //There are actually 2 contexts for DownsampleClockDomain, 0 which is the actual clock domain, and 1 which includes StateUpdate nodes for UpsampleOutput
        if (subContextNumber != 0 && subContextNumber != 1) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "C Emit Error - Unexpected sub-context number: " + GeneralHelper::to_string(subContextNumber),
                    getSharedPointer()));
        }

        cStatementQueue.push_back(cExpr);
    }else if(useVectorSamplingMode){
        //Will open a scope for the clock domain
        std::string cExpr = "{";
        cStatementQueue.push_back(cExpr);
    }
}

void DownsampleClockDomain::emitCContextCloseFirst(std::vector<std::string> &cStatementQueue,
                                                   SchedParams::SchedType schedType, int subContextNumber,
                                                   int partitionNum) {
    //Only output of this partition's clock domain logic is not being suppressed (and being handled by adjusting the compute outer loop)
    if(!useVectorSamplingMode && suppressClockDomainLogicForPartitions.find(partitionNum) == suppressClockDomainLogicForPartitions.end()) {
        if (subContextNumber != 0 && subContextNumber != 1) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "Tried to close unexpected context " + GeneralHelper::to_string(subContextNumber),
                    getSharedPointer()));
        }

        //Increment the counter before closing the context
        //Only done for subcontext 0 (clock domain executed).  Subcontext 1 is when Upsample sets 0.
        if(subContextNumber == 0) {
            std::string counterVar = getExecutionCountVariableName();
            cStatementQueue.push_back(counterVar + "++;");
        }

        cStatementQueue.push_back("}");
    }else if(useVectorSamplingMode){
        //Will open a scope for the clock domain
        std::string cExpr = "}";
        cStatementQueue.push_back(cExpr);
    }
}

void
DownsampleClockDomain::emitCContextCloseMid(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                            int subContextNumber, int partitionNum) {
    DownsampleClockDomain::emitCContextCloseFirst(cStatementQueue, schedType, subContextNumber, partitionNum);
}

void DownsampleClockDomain::emitCContextCloseLast(std::vector<std::string> &cStatementQueue,
                                                  SchedParams::SchedType schedType, int subContextNumber,
                                                  int partitionNum) {
    DownsampleClockDomain::emitCContextCloseFirst(cStatementQueue, schedType, subContextNumber, partitionNum);
}

bool DownsampleClockDomain::allowFIFOAbsorption() {
    return true;
}

void DownsampleClockDomain::setClockDomainDriver(std::shared_ptr<Arc> newDriver) {
    contextDriver = newDriver;
}

std::shared_ptr<Arc> DownsampleClockDomain::getClockDomainDriver(){
    return contextDriver;
}
