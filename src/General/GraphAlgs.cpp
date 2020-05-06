//
// Created by Christopher Yarp on 10/16/18.
//

#include <GraphCore/ContextFamilyContainer.h>
#include "GraphAlgs.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/EnabledSubSystem.h"
#include "PrimitiveNodes/Mux.h"
#include "GraphCore/ContextFamilyContainer.h"
#include "GraphCore/ContextContainer.h"
#include "GraphCore/StateUpdate.h"
#include "General/ErrorHelpers.h"
#include "GraphMLTools/GraphMLExporter.h"
#include "MultiRate/RateChange.h"

#include <iostream>
#include <random>

std::set<std::shared_ptr<Node>>
GraphAlgs::scopedTraceBackAndMark(std::shared_ptr<InputPort> traceFrom, std::map<std::shared_ptr<Arc>, bool> &marks) {
    std::set<std::shared_ptr<Node>> contextNodes;

    //Get the set of arcs connected to this input port
    std::set<std::shared_ptr<Arc>> arcs = traceFrom->getArcs();

    //Look at each incoming arc
    for(auto it = arcs.begin(); it != arcs.end(); it++){

        std::shared_ptr<Node> srcNode = (*it)->getSrcPort()->getParent();

        //Check if the src node is a state element or a enable node (input or output), or a RateChange node
        if(!(srcNode->hasState()) && GeneralHelper::isType<Node, EnableNode>(srcNode) == nullptr && GeneralHelper::isType<Node, RateChange>(srcNode) == nullptr){
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
                    std::set<std::shared_ptr<Node>> moreNodesInContext = scopedTraceBackAndMark(inputPorts[i], marks);

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
GraphAlgs::scopedTraceForwardAndMark(std::shared_ptr<OutputPort> traceFrom, std::map<std::shared_ptr<Arc>, bool> &marks){
    std::set<std::shared_ptr<Node>> contextNodes;

    //Get the set of arcs connected to this output port
    std::set<std::shared_ptr<Arc>> arcs = traceFrom->getArcs();

    //Look at each outgoing arc
    for(auto it = arcs.begin(); it != arcs.end(); it++){

        std::shared_ptr<Node> dstNode = (*it)->getDstPort()->getParent();

        //Check if the dst node is a state element, a enable node (input or output), or RateChange node
        if(!(dstNode->hasState()) && GeneralHelper::isType<Node, EnableNode>(dstNode) == nullptr && GeneralHelper::isType<Node, RateChange>(dstNode) == nullptr){
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
                    std::set<std::shared_ptr<Node>> moreNodesInContext = scopedTraceForwardAndMark(outputPorts[i], marks);

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
                throw std::runtime_error(ErrorHelpers::genErrorStr("When discovering contexts, found an unspecialized ClockDomain.  Specialization into UpsampleClockDomain or DownsampleClockDomain should occure before context discovery", foundClkDomain));
            }
            discoveredClockDomains.push_back(foundClkDomain);
        }else if(GeneralHelper::isType<Node, EnabledSubSystem>(*it) != nullptr){//Check this first because EnabledSubSystems are SubSystems
            discoveredEnabledSubSystems.push_back(std::dynamic_pointer_cast<EnabledSubSystem>(*it));
        }else if(GeneralHelper::isType<Node, SubSystem>(*it) != nullptr){
            std::shared_ptr<SubSystem> subSystem = std::dynamic_pointer_cast<SubSystem>(*it);
            subSystem->discoverAndUpdateContexts(contextStack, discoveredMux, discoveredEnabledSubSystems,
                                                 discoveredClockDomains, discoveredGeneral);
        }else{
            discoveredGeneral.push_back(*it);
        }
    }

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

            //Schedule the contextRoot
            std::shared_ptr<ContextRoot> contextRoot = familyContainer->getContextRoot();
            bool schedContextRoot = true;
            if(limitRecursionToPartition){
                std::shared_ptr<Node> contextRootAsNode = GeneralHelper::isType<ContextRoot, Node>(contextRoot);
                if(contextRootAsNode) {
                    schedContextRoot = (contextRootAsNode->getPartitionNum() == partition);
                }else{
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Unable to cast a ContextRoot to a Node"));
                }
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
                } else {
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "When scheduling, a context root was encountered which is not yet implemented"));
                }
            }

        }else{//----End node is a ContextContainerFamily----
            schedule.push_back(to_sched);
        }

        //Find discovered nodes from the candidate list (that are in the nodes to be sorted set)
        //Also, find nodes with zero in degree
        for(auto it = candidateNodes.begin(); it != candidateNodes.end(); it++) {
            std::shared_ptr<Node> candidateNode = *it;

            if (candidateNode != unconnectedMaster && candidateNode != terminatorMaster && candidateNode != visMaster &&
                candidateNode != inputMaster) {
                if (std::find(nodesToSort.begin(), nodesToSort.end(), candidateNode) != nodesToSort.end()) {
                    discoveredNodes.insert(candidateNode);

                    if (candidateNode->inDegree() == 0) {
                        nodesWithZeroInDeg.push_back(candidateNode); //Push Back for BFS and DFS.  Dequeuing determines BFS or DFS
                    }
                }
            }
        }
    }

    //If there are still viable discovered nodes, there was a cycle.
    if(!discoveredNodes.empty()){
        std::cerr << ErrorHelpers::genErrorStr("Topological Sort: Cycle Encountered.  Candidate Nodes: ") << discoveredNodes.size() << std::endl;
        for(auto it = discoveredNodes.begin(); it != discoveredNodes.end(); it++){
            std::shared_ptr<Node> candidateNode = *it;
            std::cerr << ErrorHelpers::genErrorStr(candidateNode->getFullyQualifiedName(true) + " ID: " + GeneralHelper::to_string(candidateNode->getId()) + " DirectInDeg: " + GeneralHelper::to_string(candidateNode->directInDegree()) + " TotalInDeg: " + GeneralHelper::to_string(candidateNode->inDegree())) <<std::endl;
            std::set<std::shared_ptr<Node>> connectedInputNodesPtrOrder = candidateNode->getConnectedInputNodes();
            std::set<std::shared_ptr<Node>> connectedInputNodes; //Order by ID for output
            connectedInputNodes.insert(connectedInputNodesPtrOrder.begin(), connectedInputNodesPtrOrder.end());
            for(auto connectedInputNodeIt = connectedInputNodes.begin(); connectedInputNodeIt != connectedInputNodes.end(); connectedInputNodeIt++){
                std::shared_ptr<Node> connectedInputNode = *connectedInputNodeIt;
                std::cerr << ErrorHelpers::genErrorStr("\tConnected to " + (connectedInputNode)->getFullyQualifiedName(true) + " ID: " + GeneralHelper::to_string(connectedInputNode->getId()) + " DirectInDeg: " + GeneralHelper::to_string(connectedInputNode->directInDegree()) + " InDeg: " + GeneralHelper::to_string(connectedInputNode->inDegree())) << std::endl;
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
