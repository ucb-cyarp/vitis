//
// Created by Christopher Yarp on 9/2/21.
//

#include "DomainPasses.h"

#include "MultiRate/MultiRateHelpers.h"
#include "GraphCore/Node.h"
#include "MasterNodes/MasterInput.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterUnconnected.h"
#include "MultiThread/ThreadCrossingFIFO.h"
#include "General/GraphAlgs.h"
#include "General/ErrorHelpers.h"
#include "Blocking/BlockingHelpers.h"
#include "Blocking/BlockingDomain.h"
#include "Blocking/BlockingInput.h"
#include "Blocking/BlockingOutput.h"
#include "GraphCore/ContextRoot.h"
#include "Passes/ContextPasses.h"
#include "PrimitiveNodes/TappedDelay.h"
#include "GraphCore/DummyReplica.h"
#include "PrimitiveNodes/Constant.h"

#include <iostream>

std::vector<std::shared_ptr<ClockDomain>> DomainPasses::specializeClockDomains(Design &design, std::vector<std::shared_ptr<ClockDomain>> clockDomains){
    std::vector<std::shared_ptr<ClockDomain>> specializedDomains;

    for(unsigned long i = 0; i<clockDomains.size(); i++){
        if(!clockDomains[i]->isSpecialized()){
            std::vector<std::shared_ptr<Node>> nodesToAdd, nodesToRemove;
            std::vector<std::shared_ptr<Arc>> arcsToAdd, arcsToRemove;
            std::shared_ptr<ClockDomain> specializedDomain = clockDomains[i]->specializeClockDomain(nodesToAdd, nodesToRemove, arcsToAdd, arcsToRemove);
            specializedDomains.push_back(specializedDomain);
            design.addRemoveNodesAndArcs(nodesToAdd, nodesToRemove, arcsToAdd, arcsToRemove);
        }else{
            specializedDomains.push_back(clockDomains[i]);
        }
    }

    return specializedDomains;
}

std::vector<std::shared_ptr<ClockDomain>> DomainPasses::findClockDomains(Design &design){
    std::vector<std::shared_ptr<ClockDomain>> clockDomains;

    {
        std::vector<std::shared_ptr<Node>> nodes = design.getNodes();
        for (unsigned long i = 0; i < nodes.size(); i++) {
            std::shared_ptr<ClockDomain> asClkDomain = GeneralHelper::isType<Node, ClockDomain>(nodes[i]);

            if (asClkDomain) {
                clockDomains.push_back(asClkDomain);
            }
        }
    }

    return clockDomains;
}

void DomainPasses::createClockDomainSupportNodes(Design &design, std::vector<std::shared_ptr<ClockDomain>> clockDomains, bool includeContext, bool includeOutputBridgingNodes) {
    for(unsigned long i = 0; i<clockDomains.size(); i++){
        std::vector<std::shared_ptr<Node>> nodesToAdd, nodesToRemove;
        std::vector<std::shared_ptr<Arc>> arcsToAdd, arcsToRemove;
        clockDomains[i]->createSupportNodes(nodesToAdd, nodesToRemove, arcsToAdd, arcsToRemove, includeContext, includeOutputBridgingNodes);
        design.addRemoveNodesAndArcs(nodesToAdd, nodesToRemove, arcsToAdd, arcsToRemove);
    }
}

void DomainPasses::resetMasterNodeClockDomainLinks(Design &design) {
    design.getInputMaster()->resetIoClockDomains();
    design.getOutputMaster()->resetIoClockDomains();
    design.getVisMaster()->resetIoClockDomains();
    design.getUnconnectedMaster()->resetIoClockDomains();
    design.getTerminatorMaster()->resetIoClockDomains();
}

std::map<int, std::set<std::pair<int, int>>> DomainPasses::findPartitionClockDomainRates(Design &design) {
    std::map<int, std::set<std::pair<int, int>>> clockDomains;

    //Find clock domains in IO
    //Only looking at Input, Output, and Visualization.  Unconnected and Terminated are unnessasary
    std::set<std::pair<int, int>> ioRates;
    std::vector<std::shared_ptr<MasterNode>> mastersToInclude = {design.getInputMaster(), design.getOutputMaster(), design.getVisMaster()};
    for(int i = 0; i<mastersToInclude.size(); i++) {
        std::shared_ptr<MasterNode> master = mastersToInclude[i];
        std::set<std::shared_ptr<ClockDomain>> inputClockDomains = master->getClockDomains();
        for (auto clkDomain = inputClockDomains.begin(); clkDomain != inputClockDomains.end(); clkDomain++) {
            if (*clkDomain == nullptr) {
                ioRates.emplace(1, 1);
            } else {
                ioRates.insert((*clkDomain)->getRateRelativeToBase());
            }
        }
    }
    clockDomains[IO_PARTITION_NUM] = ioRates;

    {
        std::vector<std::shared_ptr<Node>> nodes = design.getNodes();
        for (unsigned long i = 0; i < nodes.size(); i++) {
            int partition = nodes[i]->getPartitionNum();

            //Do not include clock domains themselves in this
            if (GeneralHelper::isType<Node, ClockDomain>(nodes[i]) == nullptr) {
                //Note that FIFOs report their clock domain differently in order to handle the case when they are connected directly to the InputMaster
                //Each input/output port pair can also have a different clock domain
                if (GeneralHelper::isType<Node, ThreadCrossingFIFO>(nodes[i])) {
                    std::shared_ptr<ThreadCrossingFIFO> fifo = std::dynamic_pointer_cast<ThreadCrossingFIFO>(nodes[i]);
                    for (int portNum = 0; portNum < fifo->getInputPorts().size(); portNum++) {
                        std::shared_ptr<ClockDomain> clkDomain = fifo->getClockDomainCreateIfNot(portNum);

                        if (clkDomain == nullptr) {
                            clockDomains[partition].emplace(1, 1);
                        } else {
                            std::pair<int, int> rate = clkDomain->getRateRelativeToBase();
                            clockDomains[partition].insert(rate);
                        }
                    }
                } else {
                    std::shared_ptr<ClockDomain> clkDomain = MultiRateHelpers::findClockDomain(nodes[i]);

                    if (clkDomain == nullptr) {
                        clockDomains[partition].emplace(1, 1);
                    } else {
                        std::pair<int, int> rate = clkDomain->getRateRelativeToBase();
                        clockDomains[partition].insert(rate);
                    }
                }
            }
        }
    }

    return clockDomains;
}

std::pair<bool, int> DomainPasses::findEffectiveSubBlockingLengthForNode(std::shared_ptr<Node> node, int baseSubBlockingLength){
    std::shared_ptr<ClockDomain> clockDomain = MultiRateHelpers::findClockDomain(node);

    return findEffectiveSubBlockingLengthForNodesUnderClockDomain(clockDomain, baseSubBlockingLength);
}

std::pair<bool, int> DomainPasses::findEffectiveSubBlockingLengthForNodesUnderClockDomain(std::shared_ptr<ClockDomain> clockDomain, int baseSubBlockingLength){
    std::pair<int, int> rateRelToBase;
    if(clockDomain) {
        rateRelToBase = clockDomain->getRateRelativeToBase();
    }else{
        //The base clock rate
        rateRelToBase = std::pair<int, int>(1, 1);
    };
    int effectiveSubBlockingRate = baseSubBlockingLength*rateRelToBase.first;
    bool isIntegerSubBlockingLen = effectiveSubBlockingRate % rateRelToBase.second == 0;
    effectiveSubBlockingRate /= rateRelToBase.second;

    return {isIntegerSubBlockingLen, effectiveSubBlockingRate};
}

std::set<std::shared_ptr<Node>> DomainPasses::discoverNodesThatCanBreakDependenciesWhenSubBlocking(Design &design, int baseSubBlockingLength){
    std::set<std::shared_ptr<Node>> nodesThatCanBreakDependency;

    std::vector<std::shared_ptr<Node>> nodes = design.getNodes();
    for(const std::shared_ptr<Node> &node : nodes){
        std::pair<bool, int> effectiveSubBlk = findEffectiveSubBlockingLengthForNode(node, baseSubBlockingLength);
        if(effectiveSubBlk.first){
            if(node->canBreakBlockingDependency(effectiveSubBlk.second)){
                nodesThatCanBreakDependency.insert(node);
            }
        }
    }

    return nodesThatCanBreakDependency;
}

void DomainPasses::mergeBlockingGroups(std::shared_ptr<std::set<std::shared_ptr<Node>>> mergeFrom,
                                       std::shared_ptr<std::set<std::shared_ptr<Node>>> mergeInto,
                                       std::set<std::shared_ptr<std::set<std::shared_ptr<Node>>>> &blockingGroups,
                                       std::map<std::shared_ptr<Node>, std::shared_ptr<std::set<std::shared_ptr<Node>>>> &nodeToBlockingGroup,
                                       std::map<std::shared_ptr<std::set<std::shared_ptr<Node>>>,
                                                std::shared_ptr<std::set<std::shared_ptr<Node>>>> &blockingEnvelopGroups){
    if(mergeFrom != mergeInto){
        //Move the nodes from "merge from" into "merge into"
        for (const std::shared_ptr<Node> &node: *mergeFrom) {
            mergeInto->insert(node);
            //Change the mapping of nodes
            nodeToBlockingGroup[node] = mergeInto;
        }

        //Move the nodes from "merge from" blockingMoveGroup to "merge into" blockingMoveGroup
        std::shared_ptr<std::set<std::shared_ptr<Node>>> mergeFromBlockingMoveGroup = blockingEnvelopGroups[mergeFrom];
        std::shared_ptr<std::set<std::shared_ptr<Node>>> mergeIntoBlockingMoveGroup = blockingEnvelopGroups[mergeInto];

        for (const std::shared_ptr<Node> &node: *mergeFromBlockingMoveGroup) {
            mergeIntoBlockingMoveGroup->insert(node);
        }

        //Remove the "merge from" blockingMoveGroup
        blockingEnvelopGroups.erase(mergeFrom);

        //Remove the "merge from" from set from the blockingGroups
        blockingGroups.erase(mergeFrom);

    }
}

void DomainPasses::blockingNodeSetDiscoveryContextNeedsEncapsulation(std::shared_ptr<ContextRoot> contextRoot,
                                                                     std::set<std::shared_ptr<std::set<std::shared_ptr<Node>>>> &blockingGroups,
                                                                     std::map<std::shared_ptr<Node>, std::shared_ptr<std::set<std::shared_ptr<Node>>>> &nodeToBlockingGroup,
                                                                     std::map<std::shared_ptr<std::set<std::shared_ptr<Node>>>,
                                                                              std::shared_ptr<std::set<std::shared_ptr<Node>>>> &blockingEnvelopGroups){
    // * On encountering a context, assuming it is required for all nodes in the context to be in the same blocking domain
    //   - Create the union of the connected components of nodes contained within it (including nodes in nested contexts) and place the context root in the set
    //      -- Remove the original strongly connected component sets and replace them with the union.
    //      -- Update the node to strongly connected component set mapping
    //   - Create a duplicate of the union set which we will call the moving set
    //      - remove the nodes under the context which do not need to explicitly be moved (ie. nodes that are under the subsystem if the context is defined by the subsystem)
    //         - Nodes in mux contexts are not removed from the list because they need to be moved
    //        These are the nodes which will explicitly be moved under the
    //   - Repeat for other contexts at this level

    std::shared_ptr<Node> asNode = GeneralHelper::isType<ContextRoot, Node>(contextRoot);
    //TODO: Remove check
    if(asNode == nullptr){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Context root that is not a node was discovered.  This should be impossible"));
    }

    std::shared_ptr<SubSystem> asSubsys = GeneralHelper::isType<ContextRoot, SubSystem>(contextRoot);
    if(asSubsys){
        //This context root takes the form of a subsystem, will hierarchically get all children and merge their blocking groups
        //Will not recurse on nested context because this outer context needs to be in a single blocking group, which forces all descendant
        //contexts to be in the same

        std::shared_ptr<std::set<std::shared_ptr<Node>>> groupToMergeInto = nodeToBlockingGroup[asSubsys];

        std::set<std::shared_ptr<Node>> descendants = asSubsys->getDescendants();
        for(const std::shared_ptr<Node> &descendant : descendants){
            mergeBlockingGroups(nodeToBlockingGroup[descendant], groupToMergeInto, blockingGroups, nodeToBlockingGroup, blockingEnvelopGroups);
        }

        //Only the root of this tree needs to be moved during block node creation, not its descendants, remove them from the blocking envelop group
        //NOTE: we are removing the nodes from the envelop group instead of explicitly placing just the context root in an empty envelop group
        //      because not all nodes in the blocking group need to be from this context.  Some may be outside of it and we would need to check
        //      for these nodes at some point to determine that they would also need to be enveloped.  Instead, we start out with all nodes
        //      in the blocking group in the envelop group, then we remove the ones which are covered by moving the outer context root subsystem
        std::shared_ptr<std::set<std::shared_ptr<Node>>> blockingEnvelopGroup = blockingEnvelopGroups[groupToMergeInto];
        for(const std::shared_ptr<Node> &descendant : descendants){
            blockingEnvelopGroup->erase(descendant);
        }

        //TODO Refactor?
        //If the context driver is going to be (or has been) replicated (ex. Clock Domain), it needs to be pulled into the blocking group as well
        if(contextRoot->shouldReplicateContextDriver()){
            std::vector<std::shared_ptr<Arc>> drivers = contextRoot->getContextDecisionDriver();
            for(const std::shared_ptr<Arc> &driver : drivers){
                std::shared_ptr<Node> driverNode = driver->getSrcPort()->getParent();
                //TODO: Remove Check?
                if(!driverNode->getInputArcs().empty()){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Expected Context Driver to be Standalone", asSubsys));
                }
                mergeBlockingGroups(nodeToBlockingGroup[driverNode], groupToMergeInto, blockingGroups, nodeToBlockingGroup, blockingEnvelopGroups);
            }

            //Check if replication has already occurred
            std::map<int, std::shared_ptr<DummyReplica>> dummyReplicas = contextRoot->getDummyReplicas();
            for(const auto &dummyReplica : dummyReplicas){
                //Get the driver connected to the dummy replica
                std::set<std::shared_ptr<Arc>> dummyReplicaDrivers = dummyReplica.second->getInputArcs();
                if(dummyReplicaDrivers.size() != 1){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Expected a single driver for DummyReplica while Blocking", dummyReplica.second));
                }
                std::shared_ptr<Arc> dummyReplicaDriverArc = *(dummyReplicaDrivers.begin());
                std::shared_ptr<Node> dummyReplicaDriverNode = dummyReplicaDriverArc->getSrcPort()->getParent();
                //TODO: Remove checks?
                if(!dummyReplicaDriverNode->getInputArcs().empty()){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Expected the driver for DummyReplica (" + dummyReplicaDriverNode->getFullyQualifiedName() + ") to have no inputs", dummyReplica.second));
                }
                if(dummyReplicaDriverNode->getOutputArcs().size() != 1){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Expected the driver for DummyReplica (" + dummyReplicaDriverNode->getFullyQualifiedName() + ") to have single output arc", dummyReplica.second));
                }

                mergeBlockingGroups(nodeToBlockingGroup[dummyReplicaDriverNode], groupToMergeInto, blockingGroups, nodeToBlockingGroup, blockingEnvelopGroups);
                mergeBlockingGroups(nodeToBlockingGroup[dummyReplica.second], groupToMergeInto, blockingGroups, nodeToBlockingGroup, blockingEnvelopGroups);
            }
        }
    }else{
        //This is a mux like context root (does not
        //Will use the context nodes list when merging
        std::shared_ptr<std::set<std::shared_ptr<Node>>> groupToMergeInto = nodeToBlockingGroup[asNode];

        for(int i = 0; i<contextRoot->getNumSubContexts(); i++) {
            std::vector<std::shared_ptr<Node>> nodesInContext = contextRoot->getSubContextNodes(i);
            for (const std::shared_ptr<Node> &nodeInContext : nodesInContext){
                mergeBlockingGroups(nodeToBlockingGroup[nodeInContext], groupToMergeInto, blockingGroups, nodeToBlockingGroup, blockingEnvelopGroups);

                //It is possible to have a nested context root in a mux context, namely another mux.  If we find one, recurse on it
                std::shared_ptr<ContextRoot> asContextRoot = GeneralHelper::isType<Node, ContextRoot>(nodeInContext);
                if(asContextRoot){
                    blockingNodeSetDiscoveryContextNeedsEncapsulation(asContextRoot,blockingGroups,nodeToBlockingGroup,blockingEnvelopGroups);
                }
            }

            //Do not remove nodes from the blocking envelop group for the mux like context.  The mux type context does not provide a guaranteed hierarchy point to move all the nodes from under itself
        }
    }
}

void DomainPasses::blockingNodeSetDiscoveryTraverse(std::set<std::shared_ptr<Node>> nodesAtLevel,
                                                    int baseSubBlockingLength,
                                                    std::set<std::shared_ptr<std::set<std::shared_ptr<Node>>>> &blockingGroups,
                                                    std::map<std::shared_ptr<Node>, std::shared_ptr<std::set<std::shared_ptr<Node>>>> &nodeToBlockingGroup,
                                                    std::map<std::shared_ptr<std::set<std::shared_ptr<Node>>>,
                                                             std::shared_ptr<std::set<std::shared_ptr<Node>>>> &blockingEnvelopGroups,
                                                    std::set<std::shared_ptr<ClockDomain>> &clockDomainsOutsideSubBlocking){ //Clock domains which should be converted to be vector subsampling/supersampling based
    // * On encountering a context, assuming it is required for all nodes in the context to be in the same blocking domain
    //   - Create the union of the connected components of nodes contained within it (including nodes in nested contexts) and place the context root in the set
    //      -- Remove the origional strongly connected component sets and replace them with the union.
    //      -- Update the node to strongly connected component set mapping
    //   - Create a duplicate of the union set which we will call the moving set
    //      - remove the nodes under the context which do not need to explicitally be moved (ie. nodes that are under the subsystem if the context is defined by the subsystem)
    //         - Nodes in mux contexts are not removed from the list because they need to be moved
    //        These are the nodes which will explicitally be moved under the
    //   - Repeat for other contexts at this level
    //
    // * On encountering a clock domain
    //   - if its timing is incompatible with sub-blocking, follow the same logic as a typical context
    //   - If is compatible with sub-blocking,
    //     -- Check if any of the strongly connected component set of nodes within it (including in nested contexts) contain nodes from outside of the clock domain
    //        -- If so, enforce the whole clock domain being placed in a single blocking domain -> execute the same logic as a typical context
    //        -- If not, recurse down to contexts within this clock domain.

    for(const std::shared_ptr<Node> &node : nodesAtLevel){
        if(GeneralHelper::isType<Node, ClockDomain>(node)){
            //Check if this clock domain is compatible with the sub-blocking length
            std::shared_ptr<ClockDomain> asClkDomain = std::static_pointer_cast<ClockDomain>(node);
            std::pair<bool, int> effectiveSubBlockingLength = findEffectiveSubBlockingLengthForNodesUnderClockDomain(asClkDomain, baseSubBlockingLength);
            if(effectiveSubBlockingLength.first){
                //The clock domain's rate does not require it be encapsulated in a blocking domain (compatible with sub-blocking under it)

                //Check if any of the nodes in the blocking sets of child nodes are outside of the clock domain.
                std::set<std::shared_ptr<Node>> descendents = asClkDomain->getDescendants();
                std::set<std::shared_ptr<std::set<std::shared_ptr<Node>>>> setsAlreadyChecked;
                bool foundNodeOutsideClkDomain = false;
                for(const std::shared_ptr<Node> &descendent : descendents){
                    std::shared_ptr<std::set<std::shared_ptr<Node>>> descendentBlockingGroup = nodeToBlockingGroup[descendent];
                    if(!GeneralHelper::contains(descendentBlockingGroup, setsAlreadyChecked)){
                        setsAlreadyChecked.insert(descendentBlockingGroup);

                        for(const std::shared_ptr<Node> &blockingGroupNode : *descendentBlockingGroup ){
                            std::shared_ptr<ClockDomain> blockingGroupNodeClkDomain = MultiRateHelpers::findClockDomain(blockingGroupNode);
                            if(MultiRateHelpers::isOutsideClkDomain(blockingGroupNodeClkDomain, asClkDomain)){
                                foundNodeOutsideClkDomain = true;
                                break; //No need to check the other nodes in the blocking group
                            }
                        }
                    }

                    if(foundNodeOutsideClkDomain){
                        break; //No need to keep checking descendants
                    }
                }

                if(foundNodeOutsideClkDomain){
                    //This clock domain needs to be encapsulated like any other context
                    std::shared_ptr<ContextRoot> asContextRoot = GeneralHelper::isType<ClockDomain, ContextRoot>(asClkDomain);
                    if(asContextRoot) {
                        blockingNodeSetDiscoveryContextNeedsEncapsulation(asContextRoot, blockingGroups, nodeToBlockingGroup, blockingEnvelopGroups);
                    }else{
                        throw std::runtime_error(ErrorHelpers::genErrorStr("Unspecialized Clock domain discovered when blocking", asClkDomain));
                    }

                }else {
                    //If not, blocking can proceed under this clock domain!  Add it to the list of clockDomainsOutsideSubBlocking
                    //And recurse on its children
                    clockDomainsOutsideSubBlocking.insert(asClkDomain);

                    //Need to get all descendants which are context roots (including ones under subsystems)
                    //Includes nested clock domains
                    std::set<std::shared_ptr<ContextRoot>> clkDomainNestedContextRoots = GraphAlgs::findContextRootsUnderSubsystem(asClkDomain);
                    std::set<std::shared_ptr<Node>> clkDomainNestedContextRootsAsNodes;
                    //TODO: Fix Diamond Inheritance Issues
                    for(const std::shared_ptr<ContextRoot> &nestedContextRoot : clkDomainNestedContextRoots){
                        std::shared_ptr<Node> nestedContextRootAsNode = std::dynamic_pointer_cast<Node>(nestedContextRoot);
                        if(nestedContextRootAsNode == nullptr){
                            throw std::runtime_error(ErrorHelpers::genErrorStr("Context Root Encountered which is Not a Node"));
                        }
                        clkDomainNestedContextRootsAsNodes.insert(nestedContextRootAsNode);
                    }

                    blockingNodeSetDiscoveryTraverse(clkDomainNestedContextRootsAsNodes,
                            baseSubBlockingLength,
                            blockingGroups,
                            nodeToBlockingGroup,
                            blockingEnvelopGroups,
                            clockDomainsOutsideSubBlocking);
                }
            }else{
                //The clock domain's rate requires it be encapsulated in a blocking domain
                std::shared_ptr<ContextRoot> asContextRoot = GeneralHelper::isType<ClockDomain, ContextRoot>(asClkDomain);
                if(asContextRoot) {
                    blockingNodeSetDiscoveryContextNeedsEncapsulation(asContextRoot, blockingGroups, nodeToBlockingGroup, blockingEnvelopGroups);
                }else{
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Unspecialized Clock domain discovered when blocking", asClkDomain));
                }
            }
        }else if(GeneralHelper::isType<Node, ContextRoot>(node)){
            std::shared_ptr<ContextRoot> asContextRoot = std::dynamic_pointer_cast<ContextRoot>(node);

            //This is another type of context, its entire context needs to be placed in an encapsulated domain
            blockingNodeSetDiscoveryContextNeedsEncapsulation(asContextRoot, blockingGroups, nodeToBlockingGroup, blockingEnvelopGroups);
        }
    }
}

void DomainPasses::setMasterBlockSizesBasedOnPortClockDomain(Design &design, int baseBlockingLength){
    design.getInputMaster()->setPortBlockSizesBasedOnClockDomain(baseBlockingLength);
    design.getOutputMaster()->setPortBlockSizesBasedOnClockDomain(baseBlockingLength);
    design.getVisMaster()->setPortBlockSizesBasedOnClockDomain(baseBlockingLength);
}

void DomainPasses::setMasterBlockOrigDataTypes(Design &design){
    design.getInputMaster()->setPortOriginalDataTypesBasedOnCurrentTypes();
    design.getOutputMaster()->setPortOriginalDataTypesBasedOnCurrentTypes();
    design.getVisMaster()->setPortOriginalDataTypesBasedOnCurrentTypes();
}

void DomainPasses::createBlockingInputNodesForIONotAtBaseDomain(std::shared_ptr<MasterInput> masterInput,
                                                  std::set<std::shared_ptr<Node>> &nodesToAdd,
                                                  std::set<std::shared_ptr<Arc>> &arcsToAdd,
                                                  std::map<std::shared_ptr<Arc>, int> &arcsWithDeferredBlockingExpansion,
                                                  int baseBlockLength){
    std::vector<std::shared_ptr<OutputPort>> inputMasterPorts = masterInput->getOutputPorts();

    for(const std::shared_ptr<OutputPort> &masterPort : inputMasterPorts) {
        if (!masterPort->getArcs().empty()) {
            DataType origDataType = masterInput->getPortOrigDataType(masterPort);
            std::shared_ptr<ClockDomain> dstClockDomain = masterInput->getPortClkDomain(masterPort);
            std::pair<int, int> dstClockDomainRateRelToBase(1, 1);
            if (dstClockDomain) {
                dstClockDomainRateRelToBase = dstClockDomain->getRateRelativeToBase();
            }
            std::set<std::shared_ptr<Arc>> inArcs = masterPort->getArcs();

            int scaledBaseBlockLength =
                    baseBlockLength * dstClockDomainRateRelToBase.first / dstClockDomainRateRelToBase.second;

            //TODO: Remove Check once https://github.com/ucb-cyarp/vitis/issues/101 resolved
            for (const std::shared_ptr<Arc> &arc: inArcs) {
                std::shared_ptr<ClockDomain> dstClkDomain = MultiRateHelpers::findClockDomain(
                        arc->getDstPort()->getParent());
                if (dstClkDomain != dstClockDomain) {
                    //Even if they are at the same rate, the indexing logic will be different for each arc if they are in different blocking domains
                    throw std::runtime_error(
                            "Expect All Arcs from MasterInput port to be destined for same clock domain.  See https://github.com/ucb-cyarp/vitis/issues/101");
                }
            }

            //Blocking Inputs should already be inserted for arcs in the base clocking domain
            if (dstClockDomain != nullptr && dstClockDomain->isUsingVectorSamplingMode()) {
                //Need to insert & rewire Blocking inputs as needed
                masterInput->setPortClockDomainLogicHandledByBlockingBoundary(masterPort);

                //Can share blocking inputs among arcs
                //However, only share among destinations in the same partition
                std::map<std::pair<std::shared_ptr<BlockingDomain>, int>, std::shared_ptr<BlockingInput>> existingBlockingInputs;

                for (const std::shared_ptr<Arc> &arc: inArcs) {
                    std::shared_ptr<OutputPort> srcPort = masterPort;
                    std::vector<std::shared_ptr<BlockingDomain>> blockingDomainStack = BlockingHelpers::findBlockingDomainStack(
                            arc->getDstPort()->getParent());
                    int dstPartition = arc->getDstPort()->getParent()->getPartitionNum();

                    int localBlockLength = scaledBaseBlockLength;

                    //Traverse down the blocking domain stack
                    for (const std::shared_ptr<BlockingDomain> &blockingDomain: blockingDomainStack) {
                        int numSubBlocks = blockingDomain->getBlockingLen() / blockingDomain->getSubBlockingLen();
                        if (localBlockLength % numSubBlocks != 0) {
                            throw std::runtime_error(ErrorHelpers::genErrorStr(
                                    "Unexpected Sub-Block Length While Handling Master Input Arc"));
                        }
                        int localSubBlockLength = localBlockLength / numSubBlocks;

                        std::shared_ptr<BlockingInput> blockingInput;
                        if (GeneralHelper::contains({blockingDomain, dstPartition}, existingBlockingInputs)) {
                            //A blocking input already exists in this blocking domain, no need to replicate
                            blockingInput = existingBlockingInputs[{blockingDomain, dstPartition}];
                        } else {
                            //Need to make a Blocking Input

                            blockingInput = NodeFactory::createNode<BlockingInput>(blockingDomain);
                            nodesToAdd.insert(blockingInput);
                            existingBlockingInputs[{blockingDomain, dstPartition}] = blockingInput;
                            blockingDomain->addBlockInputRateAdjusted(blockingInput);
                            blockingInput->setBlockingLen(localBlockLength);
                            blockingInput->setSubBlockingLen(localSubBlockLength);
                            blockingInput->setPartitionNum(dstPartition);

                            blockingInput->setName("BlockingDomainForMasterInput_" + masterPort->getName());

                            std::shared_ptr<ClockDomain> blockInputClkDomain = MultiRateHelpers::findClockDomain(blockingInput);
                            if(blockInputClkDomain){
                                blockInputClkDomain->addIOBlockingInput(blockingInput);
                            }

                            std::shared_ptr<Arc> blockingInputConnection = Arc::connectNodes(srcPort,
                                                                                             blockingInput->getInputPortCreateIfNot(
                                                                                                     0),
                                                                                             origDataType,
                                                                                             arc->getSampleTime());
                            arcsWithDeferredBlockingExpansion[blockingInputConnection] = localBlockLength; //Need to expand the arc going into the blocking input by the block size
                            arcsToAdd.insert(blockingInputConnection);
                        }

                        //Assign the given arc to the proper BlockingInput
                        //Will be re-assigned if there is another
                        arc->setSrcPortUpdateNewUpdatePrev(blockingInput->getOutputPortCreateIfNot(0));
                        arcsWithDeferredBlockingExpansion[arc] = localSubBlockLength;

                        //Set for the next level down the domain stack (if one exists)
                        localBlockLength = localSubBlockLength;
                        srcPort = blockingInput->getOutputPortCreateIfNot(0);
                    }
                }
            }
        }
    }
}

void DomainPasses::createBlockingOutputNodesForIONotAtBaseDomain(std::shared_ptr<MasterOutput> masterOutput,
                                                                 std::set<std::shared_ptr<Node>> &nodesToAdd,
                                                                 std::set<std::shared_ptr<Arc>> &arcsToAdd,
                                                                 std::map<std::shared_ptr<Arc>, int> &arcsWithDeferredBlockingExpansion,
                                                                 int baseBlockLength){
    std::vector<std::shared_ptr<InputPort>> outputMasterPorts = masterOutput->getInputPorts();

    for(const std::shared_ptr<InputPort> &masterPort : outputMasterPorts) {
        if (!masterPort->getArcs().empty()) {
            DataType origDataType = masterOutput->getPortOrigDataType(masterPort);
            std::shared_ptr<ClockDomain> srcClockDomain = masterOutput->getPortClkDomain(masterPort);
            std::pair<int, int> srcClockDomainRateRelToBase(1, 1);
            if (srcClockDomain) {
                srcClockDomainRateRelToBase = srcClockDomain->getRateRelativeToBase();
            }
            std::set<std::shared_ptr<Arc>> outArcs = masterPort->getArcs();

            int scaledBaseBlockLength =
                    baseBlockLength * srcClockDomainRateRelToBase.first / srcClockDomainRateRelToBase.second;

            //There should only be 1 input arc per port
            if (outArcs.size() != 1) {
                throw std::runtime_error(
                        ErrorHelpers::genErrorStr("Master Output should have 1 port connected per port", masterOutput));
            }

            std::shared_ptr<Arc> arc = *(outArcs.begin());

            //TODO: Remove Check once https://github.com/ucb-cyarp/vitis/issues/101 resolved
            {
                std::shared_ptr<ClockDomain> srcClkDomainFromNode = MultiRateHelpers::findClockDomain(
                        arc->getSrcPort()->getParent());
                if (srcClkDomainFromNode != srcClockDomain) {
                    throw std::runtime_error("Expected Master Arc ClockDomain to match setting in Master Output");
                }
            }

            //Blocking Inputs should already be inserted for arcs in the base clocking domain
            if (srcClockDomain != nullptr && srcClockDomain->isUsingVectorSamplingMode()) {
                //Need to insert & rewire Blocking inputs as needed
                masterOutput->setPortClockDomainLogicHandledByBlockingBoundary(masterPort);

                //Do not expect outright duplication of arcs to I/O
                //Will not try to share block output nodes
                //To do this, need to search other arcs connected to the source node and find a Blocking Output
                //Then need to trace from it, forward, checking for the proper blocking factors.
                //TODO: If this becomes an issue, can add this feature

                std::shared_ptr<InputPort> dstPort = masterPort;
                std::vector<std::shared_ptr<BlockingDomain>> blockingDomainStack = BlockingHelpers::findBlockingDomainStack(
                        arc->getSrcPort()->getParent());
                std::shared_ptr<OutputPort> srcPort = arc->getSrcPort();
                int srcPartition = srcPort->getParent()->getPartitionNum();

                int localBlockLength = scaledBaseBlockLength;

                //Traverse down the blocking domain stack
                for (const std::shared_ptr<BlockingDomain> &blockingDomain: blockingDomainStack) {
                    int numSubBlocks = blockingDomain->getBlockingLen() / blockingDomain->getSubBlockingLen();
                    if (localBlockLength % numSubBlocks != 0) {
                        throw std::runtime_error(ErrorHelpers::genErrorStr(
                                "Unexpected Sub-Block Length While Handling Master Output Arc"));
                    }
                    int localSubBlockLength = localBlockLength / numSubBlocks;

                    std::shared_ptr<BlockingOutput> blockingOutput = NodeFactory::createNode<BlockingOutput>(
                            blockingDomain);
                    nodesToAdd.insert(blockingOutput);
                    blockingDomain->addBlockOutputRateAdjusted(blockingOutput);
                    blockingOutput->setBlockingLen(localBlockLength);
                    blockingOutput->setSubBlockingLen(localSubBlockLength);
                    blockingOutput->setPartitionNum(srcPartition);

                    blockingOutput->setName("BlockingDomainForMasterOutput_" + masterPort->getName());

                    std::shared_ptr<ClockDomain> blockOutputClkDomain = MultiRateHelpers::findClockDomain(blockingOutput);
                    if(blockOutputClkDomain){
                        blockOutputClkDomain->addIOBlockingOutput(blockingOutput);
                    }

                    std::shared_ptr<Arc> blockingOutputConnection = Arc::connectNodes(
                            blockingOutput->getOutputPortCreateIfNot(0),
                            dstPort,
                            origDataType,
                            arc->getSampleTime());
                    arcsWithDeferredBlockingExpansion[blockingOutputConnection] = localBlockLength; //Need to expand the arc going into the blocking input by the block size
                    arcsToAdd.insert(blockingOutputConnection);

                    //Assign the given arc to the proper BlockingOutput
                    //Will be re-assigned if there is another
                    arc->setDstPortUpdateNewUpdatePrev(blockingOutput->getInputPortCreateIfNot(0));
                    arcsWithDeferredBlockingExpansion[arc] = localSubBlockLength;

                    //Set for the next level down the domain stack (if one exists)
                    localBlockLength = localSubBlockLength;
                    dstPort = blockingOutput->getInputPortCreateIfNot(0);
                }
            }
        }
    }
}

//TODO: Need to extend to do both inputs and outputs.  Same problem can occure
void DomainPasses::createBlockingNodesForIONotAtBaseDomain(Design &design, std::map<std::shared_ptr<Arc>, int> &arcsWithDeferredBlockingExpansion, int baseBlockLength){
    std::set<std::shared_ptr<Node>> nodesToAdd;
    std::set<std::shared_ptr<Arc>> arcsToAdd;

    createBlockingInputNodesForIONotAtBaseDomain(design.getInputMaster(),
            nodesToAdd,
            arcsToAdd,
            arcsWithDeferredBlockingExpansion,
            baseBlockLength);

    createBlockingOutputNodesForIONotAtBaseDomain(design.getOutputMaster(),
                                                 nodesToAdd,
                                                 arcsToAdd,
                                                 arcsWithDeferredBlockingExpansion,
                                                 baseBlockLength);

    createBlockingOutputNodesForIONotAtBaseDomain(design.getVisMaster(),
                                                 nodesToAdd,
                                                 arcsToAdd,
                                                 arcsWithDeferredBlockingExpansion,
                                                 baseBlockLength);

    std::set<std::shared_ptr<Node>> blankNodesSet;
    std::set<std::shared_ptr<Arc>> blankArcsSet;
    design.addRemoveNodesAndArcs(nodesToAdd, blankNodesSet, arcsToAdd, blankArcsSet);
    design.assignNodeIDs();
    design.assignArcIDs();
}

void DomainPasses::blockAndSubBlockDesign(Design &design, int baseBlockingLength, int baseSubBlockingLength){
    if(baseBlockingLength == 1){
        //Do not insert the global blocking domain, do not insert sub-blocking domains
        return;
    }

    //TODO: Before conducting the sub-blocking (if applicable), discover the clock domains associated with the different master
    //I/O ports.  This information will be used to set the block sizes of the blocking domain

    std::map<std::shared_ptr<Arc>, int> arcsWithDeferredBlockingExpansion;

    if(baseSubBlockingLength > 1) {
        //Create Sub-blocking domains

        //Create a clone of the Design
        std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> origToCopyNode;
        std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> copyToOrigNode;
        std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> origToCopyArc;
        std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> copyToOrigArc;
        Design designClone = design.copyGraph(origToCopyNode, copyToOrigNode, origToCopyArc, copyToOrigArc);

        //Walk through designs and disconnect delays with length > sub-blocking factor (local sub-blocking factor taking into account any clock domains)
        //  Note these delays as they still need to be handled during sub-block insertion
        std::set<std::shared_ptr<Node>> dependencyBreakNodes = discoverNodesThatCanBreakDependenciesWhenSubBlocking(
                designClone, baseSubBlockingLength);
        for (const std::shared_ptr<Node> &dependencyBreakNode: dependencyBreakNodes) {
            std::set<std::shared_ptr<Arc>> arcsToDisconnect = dependencyBreakNode->disconnectOutputs();
            std::set<std::shared_ptr<Arc>> emptyArcSet;
            std::set<std::shared_ptr<Node>> emptyNodeSet;
            designClone.addRemoveNodesAndArcs(emptyNodeSet, emptyNodeSet, emptyArcSet, arcsToDisconnect);
        }

        //Find the strongly connected components in the design
        std::set<std::shared_ptr<Node>> cloneMasterNodes = designClone.getMasterNodes();
        std::set<std::set<std::shared_ptr<Node>>> cloneStronglyConnectedComponents = GraphAlgs::findStronglyConnectedComponents(
                designClone.getNodes(), cloneMasterNodes);

        //Translate back to the original design:
        std::set<std::set<std::shared_ptr<Node>>> stronglyConnectedComponents;

        for (const std::set<std::shared_ptr<Node>> &cloneStronglyConnectedComponent: cloneStronglyConnectedComponents) {
            std::set<std::shared_ptr<Node>> stronglyConnectedComponent;

            for (const std::shared_ptr<Node> &cloneNode: cloneStronglyConnectedComponent) {
                stronglyConnectedComponent.insert(copyToOrigNode[cloneNode]);
            }
            stronglyConnectedComponents.insert(stronglyConnectedComponent);
        }

        //Now, we need to use the context hierarchy along with the discovered strongly connected components to
        //determine where blocking domains should be placed.  There are a few constraints that need to be considered
        //
        // 1. Nodes in a strongly connected component need to be placed together under the blocking domain
        // 2. Contexts (except clock domains), need to be placed in a single connected component
        //    2.a. Contexts may exist in strongly connected components with other nodes outside of the context.
        //         All of these nodes need to be placed in the same blocking domain
        //    2.b. There may be nested contexts, so care must be taken when moving nodes and contexts to make sure the
        //         nodes are not moved out of contexts.  Probably the best way to handle this is to perform a hierarchical
        //         move where contexts that are also subsystems are moved by changing the parent of the subsystem
        // 3. Clock domains may need to be placed inside of a blocking domain (if their rate is incompatible with the blocking)
        //    If their rate is compatible with sub-blocking, sub-blocking domains can exist under them like a standard subsystem
        //    3.a. If a clock domain with compatible rate has nodes that are participating in a connected component with nodes
        //         outside of the clock domain, the whole clock domain should
        // 4. Context and context stacks need to be updated.  It may actually just be easier to reset and redo context
        //    discovery.  We needed context discovery before so that Mux contexts were properly grouped under block domains
        //
        // My initial thought was to simply merge strongly connected components by itterating over
        // We also need to consider that finding the sets to be blocked is only part of the battle, we also need to be careful
        // when creating the blocking contexts.
        // My thought now is that a hierarchical approach is appropriate, with different conditions to handle the differerent constraints
        // shown above.

        //NOTE: IMPORTANT: We only need to consider one level of context hierarchy (with the exception of clock domains)

        std::set<std::shared_ptr<std::set<std::shared_ptr<Node>>>> blockingGroups;
        std::map<std::shared_ptr<Node>, std::shared_ptr<std::set<std::shared_ptr<Node>>>> nodeToBlockingGroup;

        for (const std::set<std::shared_ptr<Node>> &stronglyConnectedComponent: stronglyConnectedComponents) {
            std::shared_ptr<std::set<std::shared_ptr<Node>>> blockingGroup = std::make_shared<std::set<std::shared_ptr<Node>>>();

            for (const std::shared_ptr<Node> &node: stronglyConnectedComponent) {
                blockingGroup->insert(node);
                nodeToBlockingGroup[node] = blockingGroup;
            }

            blockingGroups.insert(blockingGroup);
        }

        //Create copies of the blocking group as blockingEnvelopGroups
        std::map<std::shared_ptr<std::set<std::shared_ptr<Node>>>, std::shared_ptr<std::set<std::shared_ptr<Node>>>> blockingEnvelopGroups;
        for (const std::shared_ptr<std::set<std::shared_ptr<Node>>> &blockingGroup: blockingGroups) {
            std::shared_ptr<std::set<std::shared_ptr<Node>>> blockingEnvelopeGroup = std::make_shared<std::set<std::shared_ptr<Node>>>(
                    *blockingGroup);
            blockingEnvelopGroups[blockingGroup] = blockingEnvelopeGroup;
        }

        //When traversing hierarchically, it makes sense to follow a similar behavior that context marking and discovery took
        // * First, create a map of nodes to their strongly connected component set -> requires changing the strongly connected component datastructure
        //   to use smart pointers to sets
        // * On encountering a context, assuming it is required for all nodes in the context to be in the same blocking domain
        //   - Create the union of the connected components of nodes contained within it (including nodes in nested contexts) and place the context root in the set
        //      -- Remove the original strongly connected component sets and replace them with the union.
        //      -- Update the node to strongly connected component set mapping
        //   - Create a duplicate of the union set which we will call the moving set
        //      - remove the nodes under the context which do not need to explicitly be moved (ie. nodes that are under the subsystem if the context is defined by the subsystem)
        //         - Nodes in mux contexts are not removed from the list because they need to be moved
        //        These are the nodes which will explicitly be moved under the
        //   - Repeat for other contexts at this level
        //
        // * On encountering a clock domain
        //   - if its timing is incompatible with sub-blocking, follow the same logic as a typical context
        //   - If is compatible with sub-blocking,
        //     -- Check if any of the strongly connected component set of nodes within it (including in nested contexts) contain nodes from outside of the clock domain
        //        -- If so, enforce the whole clock domain being placed in a single blocking domain -> execute the same logic as a typical context
        //        -- If not, recurse down to contexts within this clock domain.

        std::set<std::shared_ptr<ClockDomain>> clockDomainsOutsideSubBlocking;

        //Need to run blockingNodeSetDisovery on Top Level Context Roots (can be nested in subsystems
        std::set<std::shared_ptr<Node>> topLvlContextRoots;
        std::vector<std::shared_ptr<Node>> topLvlNodesInDesign = design.getTopLevelNodes();

        for(const std::shared_ptr<Node> &topLvlNode : topLvlNodesInDesign) {
            if(std::dynamic_pointer_cast<ContextRoot>(topLvlNode) != nullptr){
                topLvlContextRoots.insert(topLvlNode);
            }else {
                std::shared_ptr<SubSystem> asSubsystem = GeneralHelper::isType<Node, SubSystem>(topLvlNode);
                if(asSubsystem) {
                    std::set <std::shared_ptr<ContextRoot>> nestedContextRoots = GraphAlgs::findContextRootsUnderSubsystem(asSubsystem);
                    //TODO: Fix Diamond Inheritance Issues
                    for (const std::shared_ptr <ContextRoot> &nestedContextRoot: nestedContextRoots) {
                        std::shared_ptr <Node> nestedContextRootAsNode = std::dynamic_pointer_cast<Node>(
                                nestedContextRoot);
                        if (nestedContextRootAsNode == nullptr) {
                            throw std::runtime_error(
                                    ErrorHelpers::genErrorStr("Context Root Encountered which is Not a Node"));
                        }
                        topLvlContextRoots.insert(nestedContextRootAsNode);
                    }
                }
            }
        }

        blockingNodeSetDiscoveryTraverse(topLvlContextRoots, baseSubBlockingLength,
                                         blockingGroups,
                                         nodeToBlockingGroup,
                                         blockingEnvelopGroups,
                                         clockDomainsOutsideSubBlocking);


        //Set clockDomainsOutsideSubBlocking to use vector subsampling/supersampling mode
        //Do this before blocking so that the rate change nodes that operate on vectors are not put in blocking domains
        //Also remove any driving nodes/arcs that will no longer be needed
        std::set<std::shared_ptr<Node>> driverNodesToRemove;
        std::set<std::shared_ptr<Arc>> driverArcsToRemove;
        for(const std::shared_ptr<ClockDomain> &clkDomain : clockDomainsOutsideSubBlocking){
            clkDomain->setUseVectorSamplingModeAndPropagateToRateChangeNodes(true, driverNodesToRemove, driverArcsToRemove);
        }
        std::set<std::shared_ptr<Node>> emptyNodeSet;
        std::set<std::shared_ptr<Arc>> emptyArcSet;
        design.addRemoveNodesAndArcs(emptyNodeSet, driverNodesToRemove, emptyArcSet, driverArcsToRemove);
        for(const std::shared_ptr<Node> &driverNodeToRemove : driverNodesToRemove){
            //Remove from blocking groups
            std::shared_ptr<std::set<std::shared_ptr<Node>>> blockingGroup = nodeToBlockingGroup[driverNodeToRemove];
            std::shared_ptr<std::set<std::shared_ptr<Node>>> blockingEvelopeGroup = blockingEnvelopGroups[blockingGroup];
            blockingGroup->erase(driverNodeToRemove);
            blockingEvelopeGroup->erase(driverNodeToRemove);
            nodeToBlockingGroup.erase(driverNodeToRemove);

            if(blockingGroup->empty()){
                if(!blockingEvelopeGroup->empty()){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Expected Blocking Env Group to be Empty if Blocking Group is Empty"));
                }
                blockingGroups.erase(blockingGroup);
                blockingEnvelopGroups.erase(blockingGroup);
            }
        }

        //Move and/or Replicate Lone Constant Nodes
        {
            std::set<std::shared_ptr<Node>> nodesToAdd;

            moveAndOrCopyLoneConstantNodes(blockingGroups,
                                           nodeToBlockingGroup,
                                           blockingEnvelopGroups,
                                           nodesToAdd);

            std::set<std::shared_ptr<Node>> emptyNodeSet;
            std::set<std::shared_ptr<Arc>> emptyArcSet;

            design.addRemoveNodesAndArcs(nodesToAdd, emptyNodeSet, emptyArcSet, emptyArcSet);
            design.assignNodeIDs();
        }

        //** Insert the sub-blocking domains

        // * For moving sets, find the most common subsystem containing the nodes in the set.  Also find the effective
        //   sub-blocking length for nodes in the design.  They should all be the same if the blocking node set discovery worked correctly
        //   - create the blocking domain here
        //   - move the nodes in the moving set under the blocking domain.  Can use moveNodePreserveHierarchy
        for (const std::shared_ptr<std::set<std::shared_ptr<Node>>> &blockingGroup: blockingGroups) {
            if (blockingGroup->size() == 1) {
                //This is a blocking group of 1 node.  It does not need to be combined with other nodes
                //It may have a specialization
                std::pair<bool, int> effectiveSubBlockingLength = findEffectiveSubBlockingLengthForNode(
                        *(blockingGroup->begin()), baseSubBlockingLength);
                if (!effectiveSubBlockingLength.first) {
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "Unexpected incompatible sub-blocking length when sub-blocking node",
                            *(blockingGroup->begin())));
                }
                int localSubBlockingLength = effectiveSubBlockingLength.second;

                std::vector<std::shared_ptr<Node>> nodesToAdd;
                std::vector<std::shared_ptr<Node>> nodesToRemove;
                std::vector<std::shared_ptr<Arc>> arcsToAdd;
                std::vector<std::shared_ptr<Arc>> arcsToRemove;
                std::vector<std::shared_ptr<Node>> nodesToRemoveFromTopLevel;

                std::shared_ptr<Node> nodeToSpecialize = *blockingGroup->begin();
                if(GeneralHelper::isType<Node, Delay>(nodeToSpecialize) != nullptr &&
                        GeneralHelper::isType<Node, TappedDelay>(nodeToSpecialize) == nullptr){
                    //Delays, (but not tapped delays) need to have their specialization deferred to allow FIFO delay absorption
                    std::shared_ptr<Delay> asDelay = std::dynamic_pointer_cast<Delay>(nodeToSpecialize);
                    asDelay->specializeForBlockingDeferDelayReconfigReshape(localSubBlockingLength, 1, nodesToAdd,
                                                                            nodesToRemove,
                                                                            arcsToAdd, arcsToRemove, nodesToRemoveFromTopLevel,
                                                                            arcsWithDeferredBlockingExpansion);
                }else {
                    //When subblocking, the blocking length of the sub-blocking domain is the local sub-blocking length.  The sub-blocking length is 1
                    (*blockingGroup->begin())->specializeForBlocking(localSubBlockingLength, 1, nodesToAdd,
                                                                     nodesToRemove,
                                                                     arcsToAdd, arcsToRemove, nodesToRemoveFromTopLevel,
                                                                     arcsWithDeferredBlockingExpansion);
                }

                design.addRemoveNodesAndArcs(nodesToAdd, nodesToRemove, arcsToAdd, arcsToRemove);

                for (const std::shared_ptr<Node> nodeToRemoveFromTop: nodesToRemoveFromTopLevel) {
                    design.removeTopLevelNode(nodeToRemoveFromTop);

                    std::shared_ptr<ContextRoot> asContextRoot = GeneralHelper::isType<Node, ContextRoot>(
                            nodeToRemoveFromTop);
                    if (asContextRoot) {
                        design.removeTopLevelContextRoot(asContextRoot);
                    }
                }

                design.assignNodeIDs();
                design.assignArcIDs();
            } else {
                //Need to wrap this group in a blocking domain
                createSubBlockingDomain(design,
                                        *(blockingEnvelopGroups[blockingGroup]),
                                        *blockingGroup,
                                        baseSubBlockingLength,
                                        arcsWithDeferredBlockingExpansion);
            }
        }
    }

    //Create outer blocking domain
    //To insert the outer blocking domain, the set is the top level nodes.
    createGlobalBlockingDomain(design, baseBlockingLength, baseSubBlockingLength, arcsWithDeferredBlockingExpansion);

    //Create Blocking Input Nodes for Master Input Arcs Not Operating in The Base Clock Domain
    //This resolves an issue where Input is directly connected to a node with blocking specialization
    //The one exception is for arcs to Clock Domains not operating in vector mode.
    DomainPasses::createBlockingNodesForIONotAtBaseDomain(design, arcsWithDeferredBlockingExpansion, baseBlockingLength);

    for(const auto &arcDeferred : arcsWithDeferredBlockingExpansion){
        DataType dt = arcDeferred.first->getDataType();
        dt = dt.expandForBlock(arcDeferred.second);
        arcDeferred.first->setDataType(dt);
    }

    //** Reset and Re-discover contexts
    // * Clear context stack and context root context info.  Clear topLevelContext list
    // * Re-discover contexts with block domains treated similarly to clock domains or enabled subsystems
    //TODO: Alternatively avoid re-discovering contexts and do in place.  Should speed things up but requires more complex code

    design.clearContextDiscoveryInfo();
    ContextPasses::discoverAndMarkContexts(design);
}

void DomainPasses::createSubBlockingDomain(Design &design,
                                           std::set<std::shared_ptr<Node>> nodesToMove,
                                           std::set<std::shared_ptr<Node>> nodesInSubBlockingDomain,
                                           int baseSubBlockingLength,
                                           std::map<std::shared_ptr<Arc>, int> &arcsWithDeferredBlockingExpansion){

    if(nodesToMove.empty()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Tried to create sub-blocking domain with no nodes"));
    }

    //Discover the most common ancestors of the nodes to be moved (where the blocking domain will be created)
    std::shared_ptr<SubSystem> mostSpecificCommonAncestor = nullptr;
    {
        bool firstNode = true;
        for (const std::shared_ptr<Node> &node: nodesToMove) {
            if (firstNode) {
                mostSpecificCommonAncestor = node->getParent();
                firstNode = false;
            } else {
                mostSpecificCommonAncestor = GraphAlgs::findMostSpecificCommonAncestorParent(mostSpecificCommonAncestor,node);
            }
        }
    }

    //Need to identify arcs that move between nodes in the domain and nodes out of the domain
    //Blocking Input/Output nodes need to be inserted at these arcs
    std::set<std::shared_ptr<Arc>> arcsIntoDomain;
    std::set<std::shared_ptr<Arc>> arcsOutOfDomain;

    for(const std::shared_ptr<Node> &nodeInDomain : nodesInSubBlockingDomain){
        std::set<std::shared_ptr<Arc>> directArcsIn = nodeInDomain->getDirectInputArcs();
        std::set<std::shared_ptr<Arc>> directArcsOut = nodeInDomain->getDirectOutputArcs();

//        if(!nodeInDomain->getOrderConstraintInputArcs().empty() || !nodeInDomain->getOrderConstraintOutputArcs().empty()){
//            throw std::runtime_error(ErrorHelpers::genErrorStr("Expected blocking to occur before order constraint arcs", nodeInDomain));
//        }

        for(const std::shared_ptr<Arc> &arcIn : directArcsIn){
            std::shared_ptr<Node> srcNode = arcIn->getSrcPort()->getParent();
            if(!GeneralHelper::contains(srcNode, nodesInSubBlockingDomain)){
                arcsIntoDomain.insert(arcIn);
            }
        }

        for(const std::shared_ptr<Arc> &arcOut : directArcsOut){
            std::shared_ptr<Node> dstNode = arcOut->getDstPort()->getParent();
            if(!GeneralHelper::contains(dstNode, nodesInSubBlockingDomain)){
                arcsOutOfDomain.insert(arcOut);
            }
        }
    }

    //Find the local blocking and sub-blocking length
    //We do this by inspecting the nodes in the set to move since these should not contain nodes in multiple
    //clock domains (if there are nested clock domains, they should be under a single node in the move set)
    int localSubBlockingLength;
    auto nodeToMoveIt = nodesToMove.begin();
    {
        std::pair<bool, int> effectiveSubBlockingLength = findEffectiveSubBlockingLengthForNode(*nodeToMoveIt,
                                                                                                baseSubBlockingLength);
        if (!effectiveSubBlockingLength.first) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "Expected node to have valid sub-blocking length when creating sub-blocking domain",
                    *nodeToMoveIt));
        }
        localSubBlockingLength = effectiveSubBlockingLength.second;
    }
    nodeToMoveIt++;
    for(; nodeToMoveIt != nodesToMove.end(); nodeToMoveIt++){
        std::pair<bool, int> effectiveSubBlockingLength = findEffectiveSubBlockingLengthForNode(*nodeToMoveIt, baseSubBlockingLength);
        if (!effectiveSubBlockingLength.first) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "Expected node to have valid sub-blocking length when creating sub-blocking domain",
                    *nodeToMoveIt));
        }

        if(localSubBlockingLength != effectiveSubBlockingLength.second){
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "Found node in a blocking move set with a different computed local sub-blocking length",
                    *nodeToMoveIt));
        }
    }

    std::vector<std::shared_ptr<Node>> nodesToAdd;
    std::vector<std::shared_ptr<Arc>> arcsToAdd;
    std::vector<std::shared_ptr<Node>> nodesToRemoveFromTopLevel;
    BlockingHelpers::createBlockingDomainHelper(nodesToMove,
            arcsIntoDomain,
            arcsOutOfDomain,
            mostSpecificCommonAncestor,
            localSubBlockingLength, //For a sub-blocking domain, the sub-blocking length is the blocking length and the sub-blocking length is 1
            1,
            "SubBlockingGroup",
            nodesToAdd,
            arcsToAdd,
            nodesToRemoveFromTopLevel,
            arcsWithDeferredBlockingExpansion);

    std::vector<std::shared_ptr<Node>> emptyNodeVec;
    std::vector<std::shared_ptr<Arc>> emptyArcVec;

    design.addRemoveNodesAndArcs(nodesToAdd, emptyNodeVec, arcsToAdd, emptyArcVec);
    for(const std::shared_ptr<Node> &nodeToRemoveFromTop : nodesToRemoveFromTopLevel){
        design.removeTopLevelNode(nodeToRemoveFromTop);

        std::shared_ptr<ContextRoot> asContextRoot = GeneralHelper::isType<Node, ContextRoot>(nodeToRemoveFromTop);
        if(asContextRoot){
            design.removeTopLevelContextRoot(asContextRoot);
        }
    }

    design.assignNodeIDs();
    design.assignArcIDs();
}

void DomainPasses::createGlobalBlockingDomain(Design &design,
                                int baseBlockingLength,
                                int baseSubBlockingLength,
                                std::map<std::shared_ptr<Arc>, int> &arcsWithDeferredBlockingExpansion){

    //For the global blocking domain, we move all nodes at the top level into the created blocking domain
    std::vector<std::shared_ptr<Node>> topLevelNodes = design.getTopLevelNodes();
    std::set<std::shared_ptr<Node>> nodesToMove;
    nodesToMove.insert(topLevelNodes.begin(), topLevelNodes.end());

    //The arcs into/out of the domain come from/to the master nodes
    std::set<std::shared_ptr<Arc>> arcsIntoDomain = design.getInputMaster()->getDirectOutputArcs();
    std::set<std::shared_ptr<Arc>> arcsOutOfDomain = design.getOutputMaster()->getDirectInputArcs();
    std::set<std::shared_ptr<Arc>> visOutArcs = design.getVisMaster()->getDirectInputArcs();
    arcsOutOfDomain.insert(visOutArcs.begin(), visOutArcs.end());

    std::vector<std::shared_ptr<Node>> nodesToAdd;
    std::vector<std::shared_ptr<Arc>> arcsToAdd;
    std::vector<std::shared_ptr<Node>> nodesToRemoveFromTopLevel;
    BlockingHelpers::createBlockingDomainHelper(nodesToMove,
                                                arcsIntoDomain,
                                                arcsOutOfDomain,
                                                nullptr,
                                                 baseBlockingLength,
                                                 baseSubBlockingLength,
                                                "GlobalBlockingGroup",
                                                nodesToAdd,
                                                arcsToAdd,
                                                nodesToRemoveFromTopLevel,
                                                arcsWithDeferredBlockingExpansion);

    std::vector<std::shared_ptr<Node>> emptyNodeVector;
    std::vector<std::shared_ptr<Arc>> emptyArcVector;
    design.addRemoveNodesAndArcs(nodesToAdd, emptyNodeVector, arcsToAdd, emptyArcVector);

    //This should be all of the nodes that were moved (ie. the nodes at the top level)
    for(const std::shared_ptr<Node> &nodeToRemoveFromTopLevel : nodesToRemoveFromTopLevel){
        design.removeTopLevelNode(nodeToRemoveFromTopLevel);

        std::shared_ptr<ContextRoot> asContextRoot = GeneralHelper::isType<Node, ContextRoot>(nodeToRemoveFromTopLevel);
        if(asContextRoot){
            design.removeTopLevelContextRoot(asContextRoot);
        }
    }

    design.assignNodeIDs();
    design.assignArcIDs();

    //need to expand the dimensions of the I/O arcs by the blocking factor
    //Note, the arcs may have changed after adding BlockingInput and BlockingOutput nodes
    //Need to do this after blocking insertion because changing the datatype of the arc before adding the blocking
    //domain can result in an improperly expanded datatype being used inside of the domain

    //This is now done outside of this function via arcsWithDeferredBlockingExpansion

}

void DomainPasses::specializeDeferredDelays(Design &design){
    std::vector<std::shared_ptr<Node>> nodes = design.getNodes();

    std::vector<std::shared_ptr<Node>> nodesToAdd;
    std::vector<std::shared_ptr<Node>> nodesToRemove;
    std::vector<std::shared_ptr<Arc>> arcsToAdd;
    std::vector<std::shared_ptr<Arc>> arcsToRemove;
    std::vector<std::shared_ptr<Node>> nodesToRemoveFromTopLevel;
    std::map<std::shared_ptr<Arc>, int> arcsWithDeferredBlockingExpansion;

    for(const std::shared_ptr<Node> &node : nodes){
        if(GeneralHelper::isType<Node, Delay>(node) != nullptr &&
           GeneralHelper::isType<Node, TappedDelay>(node) == nullptr){
            std::shared_ptr<Delay> asDelay = std::dynamic_pointer_cast<Delay>(node);

            if(asDelay->isBlockingSpecializationDeferred()){
                asDelay->specializeForBlocking(0, //These block values are not used, the deferred values are
                        0,
                        nodesToAdd,
                        nodesToRemove,
                        arcsToAdd,
                        arcsToRemove,
                        nodesToRemoveFromTopLevel,
                        arcsWithDeferredBlockingExpansion);
            }
        }
    }

    if(!arcsWithDeferredBlockingExpansion.empty()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Expected no arcs to be processed when specializing deferred delays"));
    }

    design.addRemoveNodesAndArcs(nodesToAdd, nodesToRemove, arcsToAdd, arcsToRemove);

    for (const std::shared_ptr<Node> nodeToRemoveFromTop: nodesToRemoveFromTopLevel) {
        design.removeTopLevelNode(nodeToRemoveFromTop);

        std::shared_ptr<ContextRoot> asContextRoot = GeneralHelper::isType<Node, ContextRoot>(
                nodeToRemoveFromTop);
        if (asContextRoot) {
            design.removeTopLevelContextRoot(asContextRoot);
        }
    }
}

void DomainPasses::moveAndOrCopyLoneConstantNodes(std::set<std::shared_ptr<std::set<std::shared_ptr<Node>>>> &blockingGroups,
                                                  std::map<std::shared_ptr<Node>, std::shared_ptr<std::set<std::shared_ptr<Node>>>> &nodeToBlockingGroup,
                                                  std::map<std::shared_ptr<std::set<std::shared_ptr<Node>>>,
                                                    std::shared_ptr<std::set<std::shared_ptr<Node>>>> &blockingEnvelopGroups,
                                                  std::set<std::shared_ptr<Node>> &nodesToAdd){

    std::set<std::pair<std::shared_ptr<std::set<std::shared_ptr<Node>>>,
             std::shared_ptr<std::set<std::shared_ptr<Node>>>>> blockingGroupsToMerge;

    for(const std::shared_ptr<std::set<std::shared_ptr<Node>>> &blockingGroup : blockingGroups){
        if(blockingGroup != nullptr && blockingGroup->size() == 1){
            //Check if the only node in this blocking group is a constant
            std::shared_ptr<Node> node = *(blockingGroup->begin());
            if(GeneralHelper::isType<Node, Constant>(node)){
                std::set<std::shared_ptr<Arc>> constantOutArcs = node->getOutputArcs();
                if(constantOutArcs != node->getDirectOutputArcs() && !node->getInputArcs().empty()){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Expected No Order Constraint Nodes out of Constant While Blocking", node));
                }

                int arcCount = 0;
                bool discoveredIneligibleArc = false;
                for(const std::shared_ptr<Arc> &constOutArc : constantOutArcs){
                    std::shared_ptr<Node> dstNode = constOutArc->getDstPort()->getParent();
                    std::shared_ptr<std::set<std::shared_ptr<Node>>> dstBlockingGroup = nodeToBlockingGroup[dstNode];
                    bool eligibleForMoveOrCopy = true;
                    if(dstBlockingGroup->size() == 1){
                        if(dstNode->specializesForBlocking()){
                            eligibleForMoveOrCopy = false;
                            discoveredIneligibleArc = true;
                        }
                    }

                    if(eligibleForMoveOrCopy) {
                        if (arcCount >= constantOutArcs.size() - 1 && !discoveredIneligibleArc) {
                            //Can move the constant into the blocking domain of the destination (ie. merge blocking domains)
                            //Do this after looking at all of the blocking groups since this will modify the blockingGroup set
                            blockingGroupsToMerge.emplace(blockingGroup, dstBlockingGroup);
                        } else {
                            //Create a copy of the constant and assign that to the blocking domain of the destination
                            //Will keep its parent the same as the origional constant to avoid too much confusion

                            std::shared_ptr<Node> constantCopy = node->shallowClone(node->getParent());
                            constantCopy->setName(constantCopy->getName()+"_BlockingReplica");
                            nodesToAdd.insert(constantCopy);
                            dstBlockingGroup->insert(constantCopy);
                            blockingEnvelopGroups[dstBlockingGroup]->insert(constantCopy);
                            //Reassign arc
                            std::shared_ptr<OutputPort> newSrcPort = constantCopy->getOutputPortCreateIfNot(constOutArc->getSrcPort()->getPortNum());
                            constOutArc->setSrcPortUpdateNewUpdatePrev(newSrcPort);
                        }
                    }

                    arcCount++;
                }
            }
        }
    }

    for(auto blockingGroupPairToMerge: blockingGroupsToMerge){
        mergeBlockingGroups(blockingGroupPairToMerge.first, blockingGroupPairToMerge.second, blockingGroups, nodeToBlockingGroup, blockingEnvelopGroups);
    }
}