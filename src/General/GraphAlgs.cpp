//
// Created by Christopher Yarp on 10/16/18.
//

#include <GraphCore/ContextFamilyContainer.h>
#include "GraphAlgs.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/EnabledSubSystem.h"
#include "GraphCore/DummyReplica.h"
#include "PrimitiveNodes/Mux.h"
#include "GraphCore/ContextFamilyContainer.h"
#include "GraphCore/ContextContainer.h"
#include "GraphCore/StateUpdate.h"
#include "General/ErrorHelpers.h"
#include "GraphMLTools/GraphMLExporter.h"
#include "MultiRate/RateChange.h"
#include "MultiRate/DownsampleClockDomain.h"
#include "Blocking/BlockingDomain.h"
#include "Blocking/BlockingBoundary.h"

#include <iostream>
#include <random>

std::set<std::shared_ptr<Node>>
GraphAlgs::scopedTraceBackAndMark(std::shared_ptr<InputPort> traceFrom, std::map<std::shared_ptr<Arc>, bool> &marks, bool stopAtPartitionBoundary, bool checkForContextBarriers) {
    std::set<std::shared_ptr<Node>> contextNodes;

    //Get the set of arcs connected to this input port
    std::set<std::shared_ptr<Arc>> arcs = traceFrom->getArcs();

    //Look at each incoming arc
    for(auto it = arcs.begin(); it != arcs.end(); it++){

        std::shared_ptr<Node> srcNode = (*it)->getSrcPort()->getParent();

        //Check for context expand barrier
        bool contextExpandBarrier = false;
        if(checkForContextBarriers && srcNode->getName().find(VITIS_CONTEXT_ABSORB_BARRIER_NAME) != std::string::npos){
            contextExpandBarrier = true;

            std::cout << ErrorHelpers::genErrorStr("Context Discovery/Expansion Stopped by Context Expand Barrier: " + srcNode->getFullyQualifiedName(), traceFrom->getParent(), VITIS_STD_INFO_PREAMBLE) << std::endl;
        }

        //Check if the src node is a state element or a enable node (input or output), or a RateChange node, or in another partition
        bool blockedByStateOrType = srcNode->hasState() || GeneralHelper::isType<Node, EnableNode>(srcNode) != nullptr || GeneralHelper::isType<Node, RateChange>(srcNode) != nullptr || GeneralHelper::isType<Node, BlockingBoundary>(srcNode) != nullptr;
        bool blockedByPartitionBoundary = false;
        if(stopAtPartitionBoundary){
            blockedByPartitionBoundary = srcNode->getPartitionNum() != traceFrom->getParent()->getPartitionNum();
        }

        if(blockedByPartitionBoundary && !blockedByStateOrType && !contextExpandBarrier){
            std::cout << ErrorHelpers::genErrorStr("Context Discovery/Expansion Stopped by Partition Boundary: [" + GeneralHelper::to_string(srcNode->getPartitionNum()) + "!=" + GeneralHelper::to_string(traceFrom->getParent()->getPartitionNum()) + "] " + srcNode->getFullyQualifiedName(), traceFrom->getParent(), VITIS_STD_INFO_PREAMBLE) << std::endl;
        }

        if(!blockedByStateOrType && !blockedByPartitionBoundary && !contextExpandBarrier){
            //The src node is not a state element and is not an enable node, continue

            //Check if arc is already marked (should never occur -> if it does, we have traversed the same node twice which should be impossible.
            // Even with feedback because feedback would cause an output arc to never be marked rather than visited more than once)
            //Will do this as a sanity check for now
            //TODO: remove "already marked" check
            bool alreadyMarked = false;
            if(marks.find(*it) != marks.end()){//Check if arc is already in the map
                    alreadyMarked = marks[*it];
            }
            if(alreadyMarked){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Traceback marked an arc twice.  Should never occur."));
            }

            //Mark the arc
            marks[*it]=true;

            //Check if the src node has all of its output ports marked
            bool allMarked = true;
            std::vector<std::shared_ptr<OutputPort>> srcOutputPorts = srcNode->getOutputPorts();
            for(unsigned long i = 0; i<srcOutputPorts.size() && allMarked; i++){
                std::set<std::shared_ptr<Arc>> srcOutputArcs = srcOutputPorts[i]->getArcs();

                for(auto srcOutArc = srcOutputArcs.begin(); srcOutArc != srcOutputArcs.end() && allMarked; srcOutArc++){
                    bool outMarked = false;

                    if(marks.find(*srcOutArc) != marks.end()){//Check if arc is already in the map
                        outMarked = marks[*srcOutArc];
                    }

                    allMarked = outMarked; //If the mark was found, keep set to true, otherwise set to false and break out of for loops
                }
            }

            if(allMarked) {
                //Add the src node to the context
                contextNodes.insert(srcNode);

                //Recursivly call on the input ports of this node (call on the inputs and specials but not on the OrderConstraintPort)

                std::vector<std::shared_ptr<InputPort>> inputPorts = srcNode->getInputPortsIncludingSpecial();
                for(unsigned long i = 0; i<inputPorts.size(); i++){
                    std::set<std::shared_ptr<Node>> moreNodesInContext = scopedTraceBackAndMark(inputPorts[i], marks, stopAtPartitionBoundary, checkForContextBarriers);

                    //combine the sets of marked nodes
                    contextNodes.insert(moreNodesInContext.begin(), moreNodesInContext.end());
                }

            }
            //else, this node is either not in the context or there is a longer path to the node from within the context.
            //If there is a longer path to the node within the context, the longest path will eventually trigger the node to be inserted

        }//else, stop traversal.  This is a state element, enable node, or RateChange.  Do not mark and do not recurse
    }

    return contextNodes;
}

std::set<std::shared_ptr<Node>>
GraphAlgs::scopedTraceForwardAndMark(std::shared_ptr<OutputPort> traceFrom, std::map<std::shared_ptr<Arc>, bool> &marks, bool stopAtPartitionBoundary, bool checkForContextBarriers){
    std::set<std::shared_ptr<Node>> contextNodes;

    //Get the set of arcs connected to this output port
    std::set<std::shared_ptr<Arc>> arcs = traceFrom->getArcs();

    //Look at each outgoing arc
    for(auto it = arcs.begin(); it != arcs.end(); it++){

        std::shared_ptr<Node> dstNode = (*it)->getDstPort()->getParent();

        //Check for context expand barrier
        bool contextExpandBarrier = false;
        if(checkForContextBarriers && dstNode->getName().find(VITIS_CONTEXT_ABSORB_BARRIER_NAME) != std::string::npos){
            contextExpandBarrier = true;

            std::cout << ErrorHelpers::genErrorStr("Context Discovery/Expansion Stopped by Context Expand Barrier: " + dstNode->getFullyQualifiedName(), traceFrom->getParent(), VITIS_STD_INFO_PREAMBLE) << std::endl;
        }

        //Check if the src node is a state element or a enable node (input or output), or a RateChange node, or in another partition
        bool blockedByStateOrType = dstNode->hasState() || GeneralHelper::isType<Node, EnableNode>(dstNode) != nullptr || GeneralHelper::isType<Node, RateChange>(dstNode) != nullptr || GeneralHelper::isType<Node, BlockingBoundary>(dstNode) != nullptr;
        bool blockedByPartitionBoundary = false;
        if(stopAtPartitionBoundary){
            blockedByPartitionBoundary = dstNode->getPartitionNum() != traceFrom->getParent()->getPartitionNum();
        }

        if(blockedByPartitionBoundary && !blockedByStateOrType && !contextExpandBarrier){
            std::cout << ErrorHelpers::genErrorStr("Context Discovery/Expansion Stopped by Partition Boundary: [" + GeneralHelper::to_string(dstNode->getPartitionNum()) + "!=" + GeneralHelper::to_string(traceFrom->getParent()->getPartitionNum()) + "] " + dstNode->getFullyQualifiedName(), traceFrom->getParent(), VITIS_STD_INFO_PREAMBLE) << std::endl;
        }

        if(!blockedByStateOrType && !blockedByPartitionBoundary && !contextExpandBarrier){
            //The dst node is not a state element and is not an enable node, continue

            //Check if arc is already marked (should never occur -> if it does, we have traversed the same node twice which should be impossible.
            // Even with feedback because feedback would cause an output arc to never be marked rather than visited more than once)
            //Will do this as a sanity check for now
            //TODO: remove "already marked" check
            bool alreadyMarked = false;
            if(marks.find(*it) != marks.end()){//Check if arc is already in the map
                alreadyMarked = marks[*it];
            }
            if(alreadyMarked){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Traceback marked an arc twice.  Should never occur."));
            }

            //Mark the arc
            marks[*it]=true;

            //Check if the dst node has all of its input ports marked
            bool allMarked = true;
            std::vector<std::shared_ptr<InputPort>> dstInputPorts = dstNode->getInputPortsIncludingSpecial();
            for(unsigned long i = 0; i<dstInputPorts.size() && allMarked; i++){
                std::set<std::shared_ptr<Arc>> dstInputArcs = dstInputPorts[i]->getArcs();

                for(auto dstInArc = dstInputArcs.begin(); dstInArc != dstInputArcs.end() && allMarked; dstInArc++){
                    bool outMarked = false;

                    if(marks.find(*dstInArc) != marks.end()){//Check if arc is already in the map
                        outMarked = marks[*dstInArc];
                    }

                    allMarked = outMarked; //If the mark was found, keep set to true, otherwise set to false and break out of for loops
                }
            }

            if(allMarked) {
                //Add the src node to the context
                contextNodes.insert(dstNode);

                //Recursivly call on the output ports of this node (call on the inputs and specials but not on the OrderConstraintPort)

                std::vector<std::shared_ptr<OutputPort>> outputPorts = dstNode->getOutputPorts();
                for(unsigned long i = 0; i<outputPorts.size(); i++){
                    std::set<std::shared_ptr<Node>> moreNodesInContext = scopedTraceForwardAndMark(outputPorts[i], marks, stopAtPartitionBoundary, checkForContextBarriers);

                    //combine the sets of marked nodes
                    contextNodes.insert(moreNodesInContext.begin(), moreNodesInContext.end());
                }

            }
            //else, this node is either not in the context or there is a longer path to the node from within the context.
            //If there is a longer path to the node within the context, the longest path will eventually trigger the node to be inserted

        }//else, stop traversal.  This is a state element, enable node, or RateChange node.  Do not mark and do not recurse
    }

    return contextNodes;
}

void GraphAlgs::moveNodePreserveHierarchy(std::shared_ptr<Node> nodeToMove, std::shared_ptr<SubSystem> moveUnder,
                                          std::vector<std::shared_ptr<Node>> &newNodes, std::string moveSuffix, int overridePartition) {

    //==== Discover Herarechy ====
    std::shared_ptr<SubSystem> moveUnderParent = moveUnder->getParent();

    std::vector<std::shared_ptr<SubSystem>> subsystemStack;

    std::shared_ptr<SubSystem> cursor = nodeToMove->getParent();
    while(cursor != nullptr && cursor != moveUnder && cursor != moveUnderParent){
        subsystemStack.push_back(cursor);
        cursor = cursor->getParent();
    }

    //Check if we traced to the root (nullptr) and the parent of the moveUnder node is not nullptr or if we traced to the moveUnder
    if((moveUnderParent != nullptr && cursor == nullptr) || (cursor == moveUnder)) {
        subsystemStack.clear();
    }


    //Replicate the hierarcy if it does not exist under moveUnder
    std::shared_ptr<SubSystem> cursorDown = moveUnder;

    for(unsigned long i = 0; i < subsystemStack.size(); i++){
        unsigned long ind = subsystemStack.size()-1-i;

        std::string searchingFor = subsystemStack[ind]->getName();
        std::string searchingForWSuffix = searchingFor+moveSuffix;

        //TODO: Refactor to not use name.  Perhaps keep a map of old subsystems to new ones when this is used
        //search through nodes within the cursor
        std::set<std::shared_ptr<Node>> cursorChildren = cursorDown->getChildren();
        std::shared_ptr<SubSystem> tgtSubsys = nullptr;
        for(auto childIt = cursorChildren.begin(); childIt != cursorChildren.end(); childIt++){
            std::shared_ptr<Node> child = *childIt;
            if(GeneralHelper::isType<Node,SubSystem>(child) != nullptr && (child->getName() == searchingFor || child->getName() == searchingForWSuffix)){
                tgtSubsys = std::dynamic_pointer_cast<SubSystem>(child);
                break;
            }
        }

        //Could not find the desired subsystem, need to create it
        if(tgtSubsys == nullptr){
            std::shared_ptr<SubSystem> newSubsys = NodeFactory::createNode<SubSystem>(cursorDown);
            newSubsys->setName(searchingForWSuffix);
            newSubsys->setPartitionNum(subsystemStack[ind]->getPartitionNum());
            newNodes.push_back(newSubsys);
            tgtSubsys = newSubsys;
        }

        cursorDown = tgtSubsys;
    }

    //Move node under
    std::shared_ptr<SubSystem> prevParent = nodeToMove->getParent();
    if(prevParent != nullptr){
        prevParent->removeChild(nodeToMove);
    }

    nodeToMove->setParent(cursorDown);
    cursorDown->addChild(nodeToMove);
}

void GraphAlgs::discoverAndUpdateContexts(std::vector<std::shared_ptr<Node>> nodesToSearch,
                                          std::vector<Context> contextStack,
                                          std::vector<std::shared_ptr<Mux>> &discoveredMux,
                                          std::vector<std::shared_ptr<EnabledSubSystem>> &discoveredEnabledSubSystems,
                                          std::vector<std::shared_ptr<ClockDomain>> &discoveredClockDomains,
                                          std::vector<std::shared_ptr<BlockingDomain>> &discoveredBlockingDomains,
                                          std::vector<std::shared_ptr<Node>> &discoveredGeneral) {

    for(auto it = nodesToSearch.begin(); it != nodesToSearch.end(); it++){
        //Set the context
        (*it)->setContext(contextStack);

        //Recurse
        if(GeneralHelper::isType<Node, Mux>(*it) != nullptr) {
            discoveredMux.push_back(std::dynamic_pointer_cast<Mux>(*it));
        }else if(GeneralHelper::isType<Node, ClockDomain>(*it) != nullptr){//Check this first because ClockDomains are SubSystems
            std::shared_ptr<ClockDomain> foundClkDomain = std::dynamic_pointer_cast<ClockDomain>(*it);
            if(!foundClkDomain->isSpecialized()){
                throw std::runtime_error(ErrorHelpers::genErrorStr("When discovering contexts, found an unspecialized ClockDomain.  Specialization into UpsampleClockDomain or DownsampleClockDomain should occur before context discovery", foundClkDomain));
            }
            discoveredClockDomains.push_back(foundClkDomain);
        }else if(GeneralHelper::isType<Node, EnabledSubSystem>(*it) != nullptr) {//Check this first because EnabledSubSystems are SubSystems
            discoveredEnabledSubSystems.push_back(std::dynamic_pointer_cast<EnabledSubSystem>(*it));
        }else if(GeneralHelper::isType<Node, BlockingDomain>(*it) != nullptr){
            discoveredBlockingDomains.push_back(std::dynamic_pointer_cast<BlockingDomain>(*it));
        }else if(GeneralHelper::isType<Node, ContextRoot>(*it) != nullptr){
            throw std::runtime_error(ErrorHelpers::genErrorStr("During Context Discovery a ContextRoot was discovered where discovery behavior has not yet been specified", *it));
        }else if(GeneralHelper::isType<Node, SubSystem>(*it) != nullptr){
            std::shared_ptr<SubSystem> subSystem = std::dynamic_pointer_cast<SubSystem>(*it);
            subSystem->discoverAndUpdateContexts(contextStack, discoveredMux, discoveredEnabledSubSystems,
                                                 discoveredClockDomains, discoveredBlockingDomains, discoveredGeneral);
        }else{
            discoveredGeneral.push_back(*it);
        }
    }

}

std::set<std::shared_ptr<ContextRoot>> GraphAlgs::findContextRootsUnderSubsystem(std::shared_ptr<SubSystem> subsystem){
    std::set<std::shared_ptr<ContextRoot>> contextRoots;

    std::set<std::shared_ptr<Node>> children = subsystem->getChildren();
    for(const std::shared_ptr<Node> &child : children){
        std::shared_ptr<ContextRoot> asContextRoot = GeneralHelper::isType<Node, ContextRoot>(child);
        if(asContextRoot != nullptr){
            contextRoots.insert(asContextRoot);
        }else{
            std::shared_ptr<SubSystem> asSubsystem = GeneralHelper::isType<Node, SubSystem>(child);
            if(asSubsystem != nullptr){
                std::set<std::shared_ptr<ContextRoot>> contextRootsInSubsys = findContextRootsUnderSubsystem(asSubsystem);
                contextRoots.insert(contextRootsInSubsys.begin(), contextRootsInSubsys.end());
            }
        }
    }

    return contextRoots;
}

std::vector<std::shared_ptr<Node>> GraphAlgs::findNodesStopAtContextFamilyContainers(std::vector<std::shared_ptr<Node>> nodesToSearch){
    std::vector<std::shared_ptr<Node>> foundNodes;

    for(unsigned long i = 0; i<nodesToSearch.size(); i++){
        if(GeneralHelper::isType<Node, ContextFamilyContainer>(nodesToSearch[i]) != nullptr){//Check this first because ContextFamilyContainer are SubSystems
            foundNodes.push_back(nodesToSearch[i]); //Do not recurse
        }else if(GeneralHelper::isType<Node, EnabledSubSystem>(nodesToSearch[i]) != nullptr){//Check this first because EnabledSubSystems are SubSystems
            foundNodes.push_back(nodesToSearch[i]); //Do not recurse
        }else if(GeneralHelper::isType<Node, SubSystem>(nodesToSearch[i]) != nullptr){
            std::shared_ptr<SubSystem> subSystem = std::static_pointer_cast<SubSystem>(nodesToSearch[i]);
            foundNodes.push_back(subSystem);
            std::set<std::shared_ptr<Node>> childrenSetPtrOrder = subSystem->getChildren();
            std::set<std::shared_ptr<Node>, Node::PtrID_Compare> childrenSet;
            childrenSet.insert(childrenSetPtrOrder.begin(), childrenSetPtrOrder.end());
            std::vector<std::shared_ptr<Node>> childrenVector;
            childrenVector.insert(childrenVector.end(), childrenSet.begin(), childrenSet.end());

            std::vector<std::shared_ptr<Node>> moreFoundNodes = GraphAlgs::findNodesStopAtContextFamilyContainers(childrenVector);
            foundNodes.insert(foundNodes.end(), moreFoundNodes.begin(), moreFoundNodes.end());
        }else{
            foundNodes.push_back(nodesToSearch[i]);
        }
    }

    return foundNodes;
}

std::vector<std::shared_ptr<Node>> GraphAlgs::findNodesStopAtContextFamilyContainers(std::vector<std::shared_ptr<Node>> nodesToSearch, int partitionNum){
    std::vector<std::shared_ptr<Node>> foundNodes;

    for(unsigned long i = 0; i<nodesToSearch.size(); i++){
        if(GeneralHelper::isType<Node, ContextFamilyContainer>(nodesToSearch[i]) != nullptr && nodesToSearch[i]->getPartitionNum() == partitionNum){//Check this first because ContextFamilyContainer are SubSystems
            foundNodes.push_back(nodesToSearch[i]); //Do not recurse
        }else if(GeneralHelper::isType<Node, SubSystem>(nodesToSearch[i]) != nullptr){ //Do not check for partition in subsystem
            std::shared_ptr<SubSystem> subSystem = std::static_pointer_cast<SubSystem>(nodesToSearch[i]);
            //Only add the subsystem if it is in the correct partition
            if(subSystem->getPartitionNum() == partitionNum) {
                foundNodes.push_back(subSystem);
            }
            //We will still search below subsystems of a different partition in case a subsystem is mixed (not guarenteed to be seperate like ContextFamilyContainers)
            std::set<std::shared_ptr<Node>> childrenSetPtrOrder = subSystem->getChildren();
            std::set<std::shared_ptr<Node>, Node::PtrID_Compare> childrenSet;
            childrenSet.insert(childrenSetPtrOrder.begin(), childrenSetPtrOrder.end());
            std::vector<std::shared_ptr<Node>> childrenVector;
            childrenVector.insert(childrenVector.end(), childrenSet.begin(), childrenSet.end());

            std::vector<std::shared_ptr<Node>> moreFoundNodes = GraphAlgs::findNodesStopAtContextFamilyContainers(childrenVector, partitionNum);
            foundNodes.insert(foundNodes.end(), moreFoundNodes.begin(), moreFoundNodes.end());
        }else if(nodesToSearch[i]->getPartitionNum() == partitionNum){
            foundNodes.push_back(nodesToSearch[i]);
        }
    }

    return foundNodes;
}

std::vector<std::shared_ptr<Node>> GraphAlgs::topologicalSortDestructive(TopologicalSortParameters parameters,
                                                                         std::vector<std::shared_ptr<Node>> nodesToSort,
                                                                         bool limitRecursionToPartition,
                                                                         int partition,
                                                                         std::vector<std::shared_ptr<Arc>> &arcsToDelete,
                                                                         std::shared_ptr<MasterOutput> outputMaster,
                                                                         std::shared_ptr<MasterInput> inputMaster,
                                                                         std::shared_ptr<MasterOutput> terminatorMaster,
                                                                         std::shared_ptr<MasterUnconnected> unconnectedMaster,
                                                                         std::shared_ptr<MasterOutput> visMaster) {
    std::vector<std::shared_ptr<Node>> schedule;

    //First, remove incoming arc connections to the Unconnected Master and the Terminator Master.
    //These can create false dependencies, especially for state update and other nodes.
    //* this is for the purpose of scheduling only
    std::set<std::shared_ptr<Arc>> terminatorArcs = terminatorMaster->disconnectNode();
    arcsToDelete.insert(arcsToDelete.end(), terminatorArcs.begin(), terminatorArcs.end());

    std::set<std::shared_ptr<Arc>> unconnectedArcs = unconnectedMaster->disconnectNode();
    arcsToDelete.insert(arcsToDelete.end(), unconnectedArcs.begin(), unconnectedArcs.end());

    //Find nodes with 0 in degree at this context level (and not in nested contexts)
    std::vector<std::shared_ptr<Node>> nodesWithZeroInDeg;
    for(unsigned long i = 0; i<nodesToSort.size(); i++){
        //Do not add general subsystems to the list of zero in degree nodes, they do not need to be scheduled.  The nodes within them do.
        //However, do add ContextFamilyContainers as they are a special case of a subsystem that should be scheduled.
        if(GeneralHelper::isType<Node, SubSystem>(nodesToSort[i]) == nullptr || GeneralHelper::isType<Node, ContextFamilyContainer>(nodesToSort[i]) != nullptr) {
            unsigned long inDeg = nodesToSort[i]->inDegree();
            if (inDeg == 0) {
                nodesWithZeroInDeg.push_back(nodesToSort[i]);
            }
        }
    }

    //Handle the case where the output is only connected to nodes with state (or is unconnected).  When this happens, all the arcs
    //to the output are removed and it is its own connected component.  It will never be considered a candidate.  Add it to the
    //list of nodes with zero in deg if this is the case.  Note that if there is at least 1 arc to a node without state, it will be
    //considered a candidate
    if(std::find(nodesToSort.begin(), nodesToSort.end(), outputMaster) != nodesToSort.end() && outputMaster->inDegree() == 0){
        nodesWithZeroInDeg.push_back(outputMaster);
    }

    //We need to keep track of all the nodes we have discovered so far by looking at connected nodes of scheduled nodes
    //Nodes are removed from this list when they are scheduled.  If this list is not empty and there are no nodes with 0
    //in degree, there was a cycle
    std::set<std::shared_ptr<Node>, Node::PtrID_Compare> discoveredNodes;
    discoveredNodes.insert(nodesWithZeroInDeg.begin(), nodesWithZeroInDeg.end());

    std::default_random_engine rndGen(parameters.getRandSeed());

    std::vector<std::shared_ptr<Node>> nodesWithZeroInDegHolding; //This is used by the DFS blocking heuristic as a temporary store for nodes which have 0 indegree.  This allows N nodes that are independent to be scheduled
    int schedInDFSBlock = 0;

    //Schedule Nodes
    while(!nodesWithZeroInDeg.empty()){
        //Schedule Nodes with Zero In Degree
        unsigned long ind;

        switch(parameters.getHeuristic()) {
            case TopologicalSortParameters::Heuristic::BFS:
                ind = 0;
                break;

            case TopologicalSortParameters::Heuristic::DFS:
                ind = nodesWithZeroInDeg.size() - 1;
                break;

            case TopologicalSortParameters::Heuristic::DFS_BLOCKED:
                ind = nodesWithZeroInDeg.size() - 1;
                break;

            case TopologicalSortParameters::Heuristic::RANDOM: {
                std::uniform_int_distribution<unsigned long> dist(0, nodesWithZeroInDeg.size() - 1); //Pick a number randomly in the range [0, nodesWithZeroInDeg.size()-1]
                ind = dist(rndGen);
                break;
            }

            default:
                throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown Scheduling Heuristic"));
                break;
        }

        std::shared_ptr<Node> to_sched = nodesWithZeroInDeg[ind]; //Pop node to schedule off the top of the stack/queue depending on DFS/BFS traversal

        //Get the candidates from this node (the only nodes that do not currently have indeg 0 that could have indeg 0 after this
        //node is scheduled are its neighbors)
        std::set<std::shared_ptr<Node>> candidateNodesPtrOrdered = to_sched->getConnectedOutputNodes();
        std::set<std::shared_ptr<Node>, Node::PtrID_Compare> candidateNodes;
        candidateNodes.insert(candidateNodesPtrOrdered.begin(), candidateNodesPtrOrdered.end()); //Need to order by ID rather than ptr value to have consistent ordering across runs.

        //Disconnect the node
//        std::cerr << "Sched: " << to_sched->getFullyQualifiedName(true) << " [ID: " << to_sched->getId() << "]" << std::endl;
        std::set<std::shared_ptr<Arc>> arcsToRemove = to_sched->disconnectNode();
        arcsToDelete.insert(arcsToDelete.end(), arcsToRemove.begin(), arcsToRemove.end());

        discoveredNodes.erase(to_sched);
        nodesWithZeroInDeg.erase(nodesWithZeroInDeg.begin()+ind);

        //====Check if the node is a ContextContainerFamily====
        if(GeneralHelper::isType<Node, ContextFamilyContainer>(to_sched) != nullptr){
            std::shared_ptr<ContextFamilyContainer> familyContainer = std::static_pointer_cast<ContextFamilyContainer>(to_sched);

            //The context family container is now explicitally included in the schedule since
            //additional arcs from the context drivers are created to the context family containers as part of
            //multithreaded emit

            schedule.push_back(to_sched);

            //Recursively schedule the nodes in this ContextContainerFamily
            //Schedule the subcontexts first

            std::vector<std::shared_ptr<ContextContainer>> subContextContainers = familyContainer->getSubContextContainers();
            for(unsigned long i = 0; i<subContextContainers.size(); i++){
                std::set<std::shared_ptr<Node>> childrenSetPtrOrdered = subContextContainers[i]->getChildren();
                std::set<std::shared_ptr<Node>, Node::PtrID_Compare> childrenSet;
                childrenSet.insert(childrenSetPtrOrdered.begin(), childrenSetPtrOrdered.end()); //Need to order by ID for consistency between runs
                std::vector<std::shared_ptr<Node>> childrenVector;
                childrenVector.insert(childrenVector.end(), childrenSet.begin(), childrenSet.end());

                std::vector<std::shared_ptr<Node>> nextLvlNodes;
                if(limitRecursionToPartition){
                    nextLvlNodes = GraphAlgs::findNodesStopAtContextFamilyContainers(childrenVector, partition);
                }else {
                    nextLvlNodes = GraphAlgs::findNodesStopAtContextFamilyContainers(childrenVector);
                }

                std::vector<std::shared_ptr<Node>> subSched = GraphAlgs::topologicalSortDestructive(parameters,
                                                                     nextLvlNodes, limitRecursionToPartition, partition,
                                                                     arcsToDelete, outputMaster, inputMaster,
                                                                     terminatorMaster, unconnectedMaster, visMaster);
                //Add to schedule
                schedule.insert(schedule.end(), subSched.begin(), subSched.end());
            }

            //Schedule the contextRoot if in the correct partition
            std::shared_ptr<ContextRoot> contextRoot = familyContainer->getContextRoot();
            bool schedContextRoot = true;

            std::shared_ptr<Node> contextRootAsNode = GeneralHelper::isType<ContextRoot, Node>(contextRoot);
            if(contextRootAsNode) {
                if(limitRecursionToPartition) {
                    schedContextRoot = (contextRootAsNode->getPartitionNum() == partition);
                }
            }else{
                throw std::runtime_error(ErrorHelpers::genErrorStr("Unable to cast a ContextRoot to a Node"));
            }

            if(schedContextRoot) {
                if (contextRoot == nullptr) {
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Tried to schedule a ContextRoot that is null"));
                } else if (GeneralHelper::isType<ContextRoot, Mux>(contextRoot) != nullptr) {
                    //The context root is a mux, we just schedule this
                    schedule.push_back(GeneralHelper::isType<ContextRoot, Mux>(contextRoot)); //Schedule the Mux node (context root)
                    //Should not require finding more candidate nodes since any arc to/from it should be elevated to the ContextFamilyContainer
//                std::set<std::shared_ptr<Node>> moreCandidateNodes = std::dynamic_pointer_cast<Mux>(contextRoot)->getConnectedOutputNodes();
//                candidateNodes.insert(moreCandidateNodes.begin(), moreCandidateNodes.end());
                } else if (GeneralHelper::isType<ContextRoot, EnabledSubSystem>(contextRoot) != nullptr) {
                    //Scheduling this should be redundant as all nodes in the enabled subsystem should be scheduled as part of a context
                } else if (GeneralHelper::isType<ContextRoot, DownsampleClockDomain>(contextRoot) != nullptr) {
                    //Scheduling this should be redundant as all nodes in the downsample clock domain should be scheduled as part of a context
                    //However, the context drivers go directly to the DownsampleClockDomain so it needs to be explicitally scheduled
                    //even though it does not emit any actual c
                    schedule.push_back(GeneralHelper::isType<ContextRoot, DownsampleClockDomain>(contextRoot)); //Schedule the Mux node (context root)
                } else if (GeneralHelper::isType<ContextRoot, BlockingDomain>(contextRoot) != nullptr) {
                    //Scheduling this should be redundant as all nodes in the blocking domain subsystem should be scheduled as part of a context
                    //BlockingDomains behave very similarly to enabled subsystems when it comes to scheduling
                }else {
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "When scheduling, a context root was encountered which is not yet implemented"));
                }
            }

            //Schedule the Dummy Node if it is the correct partition (and it exists)
            std::shared_ptr<DummyReplica> dummyNode = familyContainer->getDummyNode();
            bool schedDummyNode = false;
            if(dummyNode != nullptr) {
                if (limitRecursionToPartition) {
                    schedDummyNode = dummyNode->getPartitionNum() == partition;
                }else{
                    schedDummyNode = true;
                }
            }

            if(schedDummyNode){
                schedule.push_back(dummyNode);
            }

        }else{//----End node is a ContextContainerFamily----
            schedule.push_back(to_sched);
        }

        //If this is the nth scheduling, add the zero degree nodes holding to the list
        //Or, if there are no nodes in the zero degree list, add the nodes
        //Reset the scheduling counter

        //Find discovered nodes from the candidate list (that are in the nodes to be sorted set)
        //Also, find nodes with zero in degree
        for(auto it = candidateNodes.begin(); it != candidateNodes.end(); it++) {
            std::shared_ptr<Node> candidateNode = *it;

            if (candidateNode != unconnectedMaster && candidateNode != terminatorMaster && candidateNode != visMaster &&
                candidateNode != inputMaster) {
                if (std::find(nodesToSort.begin(), nodesToSort.end(), candidateNode) != nodesToSort.end()) {
                    discoveredNodes.insert(candidateNode);

                    if (candidateNode->inDegree() == 0) {
                        if(parameters.getHeuristic() == TopologicalSortParameters::Heuristic::BFS || parameters.getHeuristic() == TopologicalSortParameters::Heuristic::DFS) {
                            nodesWithZeroInDeg.push_back(candidateNode); //Push Back for BFS and DFS.  Dequeuing determines BFS or DFS
                        }else if(parameters.getHeuristic() == TopologicalSortParameters::Heuristic::DFS_BLOCKED){
                            nodesWithZeroInDegHolding.push_back(candidateNode);
                        }else{
                            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown Scheduling Heuristic"));
                        }
                    }
                }
            }
        }

        if(parameters.getHeuristic() == TopologicalSortParameters::Heuristic::DFS_BLOCKED){
            if(nodesWithZeroInDeg.empty() || schedInDFSBlock >= parameters.getRandSeed()){ //RandomSeed is a temporary standin for the dfs block size of number of independent instructions to try to schedule together
                nodesWithZeroInDeg.insert(nodesWithZeroInDeg.end(), nodesWithZeroInDegHolding.begin(), nodesWithZeroInDegHolding.end());
                nodesWithZeroInDegHolding.clear();
                schedInDFSBlock = 0;
            }else{
                schedInDFSBlock++;
            }
        }

    }

    //If there are still viable discovered nodes, there was a cycle.
    if(!discoveredNodes.empty()){
        std::cerr << ErrorHelpers::genErrorStr("Topological Sort: Cycle Encountered.  Candidate Nodes: ") << discoveredNodes.size() << std::endl;
        for(auto it = discoveredNodes.begin(); it != discoveredNodes.end(); it++){
            std::shared_ptr<Node> candidateNode = *it;
            std::cerr << ErrorHelpers::genErrorStr(candidateNode->getFullyQualifiedName(true) + " ID: " + GeneralHelper::to_string(candidateNode->getId()) + " DirectInDeg: " + GeneralHelper::to_string(candidateNode->directInDegree()) + " TotalInDeg: " + GeneralHelper::to_string(candidateNode->inDegree())) <<std::endl;
            std::cerr << ErrorHelpers::genErrorStr("{Orig Location: " + candidateNode->getFullyQualifiedOrigName(true) + "}") << std::endl;
            std::set<std::shared_ptr<Node>> connectedInputNodesPtrOrder = candidateNode->getConnectedInputNodes();
            std::set<std::shared_ptr<Node>> connectedInputNodes; //Order by ID for output
            connectedInputNodes.insert(connectedInputNodesPtrOrder.begin(), connectedInputNodesPtrOrder.end());
            for(auto connectedInputNodeIt = connectedInputNodes.begin(); connectedInputNodeIt != connectedInputNodes.end(); connectedInputNodeIt++){
                std::shared_ptr<Node> connectedInputNode = *connectedInputNodeIt;
                std::cerr << ErrorHelpers::genErrorStr("\tConnected to " + (connectedInputNode)->getFullyQualifiedName(true) + " ID: " + GeneralHelper::to_string(connectedInputNode->getId()) + " DirectInDeg: " + GeneralHelper::to_string(connectedInputNode->directInDegree()) + " InDeg: " + GeneralHelper::to_string(connectedInputNode->inDegree())) << std::endl;
                std::cerr << ErrorHelpers::genErrorStr("\t{Orig Location: " + connectedInputNode->getFullyQualifiedOrigName(true) + "}") << std::endl;
                if(std::find(nodesToSort.begin(), nodesToSort.end(), connectedInputNode) == nodesToSort.end()){
                    std::cerr << "\t\tNot found in nodes to sort list" << std::endl;
                }
            }
        }
        throw std::runtime_error(ErrorHelpers::genErrorStr("Topological Sort: Encountered Cycle, Unable to Sort"));
    }

    return schedule;
}

bool GraphAlgs::createStateUpdateNodeDelayStyle(std::shared_ptr<Node> statefulNode,
                                                std::vector<std::shared_ptr<Node>> &new_nodes,
                                                std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                bool includeContext) {
    //Create a state update node for this delay
    std::shared_ptr<StateUpdate> stateUpdate = NodeFactory::createNode<StateUpdate>(statefulNode->getParent());
    stateUpdate->setName("StateUpdate-For-"+statefulNode->getName());
    stateUpdate->setPartitionNum(statefulNode->getPartitionNum());
    stateUpdate->setPrimaryNode(statefulNode);
    statefulNode->addStateUpdateNode(stateUpdate); //Set the state update node pointer in this node

    if(includeContext) {
        //Set context to be the same as the primary node
        std::vector<Context> primarayContex = statefulNode->getContext();
        stateUpdate->setContext(primarayContex);

        //Add node to the lowest level context (if such a context exists
        if (!primarayContex.empty()) {
            Context specificContext = primarayContex[primarayContex.size() - 1];
            specificContext.getContextRoot()->addSubContextNode(specificContext.getSubContext(), stateUpdate);
        }
    }

    new_nodes.push_back(stateUpdate);

    //Make the node dependent on all the outputs of each node connected via an output arc from the node (prevents update from occuring until all dependent nodes have been emitted)
    std::set<std::shared_ptr<Node>> connectedOutNodes = statefulNode->getConnectedOutputNodes();

    for(auto it = connectedOutNodes.begin(); it != connectedOutNodes.end(); it++){
        std::shared_ptr<Node> connectedOutNode = *it;
        std::string name = connectedOutNode->getFullyQualifiedName();
        if(connectedOutNode == nullptr){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Encountered Arc with Null Dst when Wiring State Update"));
        }
        std::shared_ptr<Arc> orderConstraint = Arc::connectNodesOrderConstraint(connectedOutNode, stateUpdate); //Datatype and sample time are not important, use defaults
        new_arcs.push_back(orderConstraint);
    }

    //make this node dependent on the stateful block (prevents the update from occuring until the new state has been calculated for the stateful node -> this occurs when the stateful node is scheduled)
    //Do this after adding the dependencies on the output nodes to avoid creating a false loop
    std::shared_ptr<Arc> orderConstraint = Arc::connectNodesOrderConstraint(statefulNode, stateUpdate); //Datatype and sample time are not important, use defaults
    new_arcs.push_back(orderConstraint);

    return true;
}

std::shared_ptr<SubSystem> GraphAlgs::findMostSpecificCommonAncestor(std::shared_ptr<Node> a, std::shared_ptr<Node> b){
    //Create Hierarchy Vectors

    std::vector<std::shared_ptr<SubSystem>> hierarchyA, hierarchyB;

    for(std::shared_ptr<SubSystem> ptr = a->getParent(); ptr != nullptr; ptr = ptr->getParent()){
        hierarchyA.insert(hierarchyA.begin(), ptr);
    }

    for(std::shared_ptr<SubSystem> ptr = b->getParent(); ptr != nullptr; ptr = ptr->getParent()){
        hierarchyB.insert(hierarchyB.begin(), ptr);
    }

    int maxDepth = std::min(hierarchyA.size(), hierarchyB.size());
    int common = -1;

    for(int i = 0; i<maxDepth; i++){
        if(hierarchyA[i] == hierarchyB[i]){
            common = i;
        }else{
            break;
        }
    }

    if(common<0){
        return nullptr;
    }else{
        return hierarchyA[common];
    }
}

std::shared_ptr<SubSystem> GraphAlgs::findMostSpecificCommonAncestorParent(std::shared_ptr<SubSystem> parent, std::shared_ptr<Node> b){
    //Create Hierarchy Vectors

    std::vector<std::shared_ptr<SubSystem>> hierarchyA, hierarchyB;

    for(std::shared_ptr<SubSystem> ptr = parent; ptr != nullptr; ptr = ptr->getParent()){
        hierarchyA.insert(hierarchyA.begin(), ptr);
    }

    for(std::shared_ptr<SubSystem> ptr = b->getParent(); ptr != nullptr; ptr = ptr->getParent()){
        hierarchyB.insert(hierarchyB.begin(), ptr);
    }

    int maxDepth = std::min(hierarchyA.size(), hierarchyB.size());
    int common = -1;

    for(int i = 0; i<maxDepth; i++){
        if(hierarchyA[i] == hierarchyB[i]){
            common = i;
        }else{
            break;
        }
    }

    if(common<0){
        return nullptr;
    }else{
        return hierarchyA[common];
    }
}

std::set<std::set<std::shared_ptr<Node>>> GraphAlgs::findStronglyConnectedComponentsTarjanRecursive(std::vector<std::shared_ptr<Node>> nodesToSearch){
    //First, we will create a map for the node properties used during the traversal and initialize their values
    std::map<std::shared_ptr<Node>, int> num;
    std::map<std::shared_ptr<Node>, int> lowLink;
    std::map<std::shared_ptr<Node>, bool> inStack;

    for(const std::shared_ptr<Node> &node : nodesToSearch){
        num[node] = 0;
        lowLink[node] = 0;
        inStack[node] = false;
    }

    std::vector<std::shared_ptr<Node>> stack;

    std::set<std::set<std::shared_ptr<Node>>> connectedComponents;
    int idx = 1;


    for(const std::shared_ptr<Node> node : nodesToSearch){
        //Check if the node has already been traversed by a recursive call
        if(num[node] == 0){
            tarjanStronglyConnectedComponentRecursiveWork(node, num, lowLink, stack, inStack, idx, connectedComponents);
        }
    }

    return connectedComponents;
}


void GraphAlgs::tarjanStronglyConnectedComponentRecursiveWork(std::shared_ptr<Node> node,
                                          std::map<std::shared_ptr<Node>, int> &num,
                                          std::map<std::shared_ptr<Node>, int> &lowLink,
                                          std::vector<std::shared_ptr<Node>> &stack,
                                          std::map<std::shared_ptr<Node>, bool> &inStack,
                                          int &idx,
                                          std::set<std::set<std::shared_ptr<Node>>> &connectedComponents){
    lowLink[node] = idx;
    num[node] = idx;
    idx++;

    stack.push_back(node);
    inStack[node] = true;

    std::set<std::shared_ptr<Arc>> outArcs = node->getOutputArcs();
    for(const std::shared_ptr<Arc> &outArc : outArcs){
        std::shared_ptr<Node> dstNode = outArc->getDstPort()->getParent();

        //TODO: Remove check
        if(!GeneralHelper::contains(dstNode, num)){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unexpected node found during Strongly Connected Component"));
        }

        if(num[dstNode]==0){
            //Dst node not visited, recurse on it
            tarjanStronglyConnectedComponentRecursiveWork(dstNode, num, lowLink, stack, inStack, idx, connectedComponents);
            lowLink[node] = lowLink[node] < lowLink[dstNode] ? lowLink[node] : lowLink[dstNode]; //Update the lowLink of this node based on the lowLink of the dstNode
        }else if(num[dstNode] < num[node]){
            //Found an arc back to a node which has already been traversed (ie. is not in the spanning tree defined by the DFS search).  Called a frond or a cross edge in the orig paper
            if(inStack[dstNode]){
                //If it in the stack, it could potentially be the root of the DFS subtree that defines a strongly connected component
                lowLink[node] = lowLink[node] < num[dstNode] ? lowLink[node] : num[dstNode];
                //Note: multiple descriptions of the algorithm (including the original) mix lowLink and num here.  It is intentional
            }
        }

        if(num[node] == lowLink[node]){
            //node is the root of a strongly connected component (root of the subtree in the DFS tree defining the strongly connected component)
            //Dump the strongly connected component
            std::set<std::shared_ptr<Node>> scc;

            while(!stack.empty() && num[stack[stack.size()-1]] >= num[node]){
                std::shared_ptr<Node> nodeInSCC = stack[stack.size()-1];
                scc.insert(nodeInSCC);
                stack.pop_back();
                inStack[nodeInSCC] = false;
            }

            //No need to add "node" explicitly as it was added to the stack earlier
            connectedComponents.insert(scc);
        }

    }

}

/**
 * @brief This contains the logic for when a node is visited during the DFS Traversal of Targan's algorithm.  A node can be visited in 2 cases
 * The first time the node is traversed either by the first call to the recursive Tarjan function or that the node is the direct descendant of a node
 * Subsequent times, if the node placed additional nodes on the DFS stack which were its children, it was because a descendant node finished.
 * At this point, there are either more direct descendants to travers or the node has no more children and is complete.
 *
 * The Tarjan algorithm is intrinsically recursive and this function effectively implements recursion with its own stack
 * (using std::vector where storage is allocated on the heap).  This should avoid overrunning the call stack with large designs
 * Because of this, you may still see references to recursion in the comments of the function.  In this case, it is referring
 * to the effective recursion through the DFS traversal and backtracking using the explicitly defined DFS stack.
 *
 *
 * @return if this node has finished evaluating (removed itself from the DFS stack) returns a pointer to its node, otherwise returns a nullptr
 */
std::shared_ptr<Node> GraphAlgs::tarjanStronglyConnectedComponentNonRecursiveWorkDFSTraverse(std::vector<DFS_Entry> &dfsStack,
                                                                 std::shared_ptr<Node> backTrackedNode,
                                                               std::map<std::shared_ptr<Node>, int> &num,
                                                               std::map<std::shared_ptr<Node>, int> &lowLink,
                                                               std::vector<std::shared_ptr<Node>> &stack,
                                                               std::map<std::shared_ptr<Node>, bool> &inStack,
                                                               int &idx,
                                                               std::set<std::set<std::shared_ptr<Node>>> &connectedComponents,
                                                               std::set<std::shared_ptr<Node>> &excludeNodes){
    //Working on the top of the dfs stack
    DFS_Entry &currentNode = dfsStack[dfsStack.size()-1];

    if(!currentNode.traversedBefore){
        //Perform the entry logic of the Tarjan Algorithm
        lowLink[currentNode.node] = idx;
        num[currentNode.node] = idx;
        idx++;
        stack.push_back(currentNode.node);
        inStack[currentNode.node] = true;
        currentNode.traversedBefore = true;
    }else{
        //We are returning to this node after back tracking in DFS.  The last node traversed was placed on the stack by this node
        //and we need to perform the appropriate lowLink update

        //TODO: remove this check
        if(backTrackedNode == nullptr || !GeneralHelper::contains(backTrackedNode, currentNode.childrenToSearch)){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unexpected backtrack node while finding Strongly Connected Components"));
        }

        //TODO: Remove check
        if(!GeneralHelper::contains(backTrackedNode, num)){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unexpected node found during Strongly Connected Component"));
        }

        lowLink[currentNode.node] = lowLink[currentNode.node] < lowLink[backTrackedNode] ? lowLink[currentNode.node] : lowLink[backTrackedNode]; //Update the lowLink of this node based on the lowLink of the dstNode

        //Remove node from list of children to search
        currentNode.childrenToSearch.erase(backTrackedNode);
    }

    bool traversingChild = false;
    while(!currentNode.childrenToSearch.empty()){
        //We still have children to process

        std::shared_ptr<Node> child = *currentNode.childrenToSearch.begin();

        if(GeneralHelper::contains(child, excludeNodes)){
            //The child node was in the list of nodes to exclude/ignore
            //For example, the master nodes should typically be ignored and will often not be included in the nodes
            //to search.  However, nodes may be connected to these masters, especially the output masters

            //Just remove it from the child list
            currentNode.childrenToSearch.erase(child);
        }else {
            //TODO: Remove check
            if(!GeneralHelper::contains(child, num)){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Unexpected child node found during Strongly Connected Component"));
            }

            //The action required for the child in Tarjan's algorithm
            if (num[child] == 0) {
                //Need to traverse this node (recursive call of Tarjan)
                //Put it on the DFS stack and break out of this loop.  Leave it in the children to be searched list
                //The processing for this node will finish when DFS backtracks.  At that point, processing for other children
                //will continue

                dfsStack.emplace_back(child, child->getDependentNodes1Degree());
                traversingChild = true;
                break;
            } else {
                if (num[child] < num[currentNode.node]) {
                    if (inStack[child]) {
                        lowLink[currentNode.node] =
                                lowLink[currentNode.node] < num[child] ? lowLink[currentNode.node] : num[child];
                    }
                }

                //Node processed without a recursive traversal
                currentNode.childrenToSearch.erase(child);

                //Don't need to break out of the loop in this case
            }
        }
    }

    //Are we done processing this node (ie. have all children been processed)
    std::shared_ptr<Node> nodeToReturn = nullptr;
    if(currentNode.childrenToSearch.empty() && !traversingChild){ //Do not do this if a new node is being traversed.  Need to handle the backtrack from that child first
        //Check if this is the root of a strongly connected component and pop off Tarjan stack
        if(num[currentNode.node] == lowLink[currentNode.node]){
            //node is the root of a strongly connected component (root of the subtree in the DFS tree defining the strongly connected component)
            //Dump the strongly connected component
            std::set<std::shared_ptr<Node>> scc;

            while(!stack.empty() && num[stack[stack.size()-1]] >= num[currentNode.node]){
                std::shared_ptr<Node> nodeInSCC = stack[stack.size()-1];
                scc.insert(nodeInSCC);
                stack.pop_back();
                inStack[nodeInSCC] = false;
            }

            //No need to add "node" explicitly as it was added to the Tarjan stack earlier
            connectedComponents.insert(scc);
        }

        //Remove this node from the dfs stack and return it as the backtrack node
        nodeToReturn = currentNode.node;
        dfsStack.pop_back();
    }

    return nodeToReturn;
}

std::set<std::set<std::shared_ptr<Node>>> GraphAlgs::findStronglyConnectedComponents(std::vector<std::shared_ptr<Node>> nodesToSearch, std::set<std::shared_ptr<Node>> &excludeNodes){
    //First, we will create a map for the node properties used during the traversal and initialize their values
    std::map<std::shared_ptr<Node>, int> num;
    std::map<std::shared_ptr<Node>, int> lowLink;
    std::map<std::shared_ptr<Node>, bool> inStack;

    for(const std::shared_ptr<Node> &node : nodesToSearch){
        num[node] = 0;
        lowLink[node] = 0;
        inStack[node] = false;
    }

    std::vector<std::shared_ptr<Node>> stack;

    std::set<std::set<std::shared_ptr<Node>>> connectedComponents;
    int idx = 1;

    std::vector<DFS_Entry> dfsStack;

    for(const std::shared_ptr<Node> &node : nodesToSearch){
        //Check if the node has already been traversed by a recursive call
        if(num[node] == 0){
            std::shared_ptr<Node> backTrackNode = nullptr;
            dfsStack.emplace_back(node, node->getDependentNodes1Degree());
            while(!dfsStack.empty()){
                backTrackNode = tarjanStronglyConnectedComponentNonRecursiveWorkDFSTraverse(dfsStack,
                                                                                            backTrackNode,
                                                                                            num,
                                                                                            lowLink,
                                                                                            stack,
                                                                                            inStack,
                                                                                            idx,
                                                                                            connectedComponents,
                                                                                            excludeNodes);
            }
        }
    }

    return connectedComponents;
}

int GraphAlgs::findPartitionInSubSystem(std::shared_ptr<SubSystem> subsystem){
    std::set<std::shared_ptr<Node>> children = subsystem->getChildren();
    for(const std::shared_ptr<Node> &child : children){
        int childPart = child->getPartitionNum();
        if(childPart != -1){
            return childPart;
        }
    }

    //If we could not find the partition directly from the children, try going into nested subsystems
    for(const std::shared_ptr<Node> &child : children){
        std::shared_ptr<SubSystem> asSubsystem = GeneralHelper::isType<Node, SubSystem>(child);
        if(asSubsystem){
            return findPartitionInSubSystem(asSubsystem);
        }
    }

    return -1;
}