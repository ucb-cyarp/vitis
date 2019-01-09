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

#include <iostream>

std::set<std::shared_ptr<Node>>
GraphAlgs::scopedTraceBackAndMark(std::shared_ptr<InputPort> traceFrom, std::map<std::shared_ptr<Arc>, bool> &marks) {
    std::set<std::shared_ptr<Node>> contextNodes;

    //Get the set of arcs connected to this input port
    std::set<std::shared_ptr<Arc>> arcs = traceFrom->getArcs();

    //Look at each incoming arc
    for(auto it = arcs.begin(); it != arcs.end(); it++){

        std::shared_ptr<Node> srcNode = (*it)->getSrcPort()->getParent();

        //Check if the src node is a state element or a enable node (input or output)
        if(!(srcNode->hasState()) && GeneralHelper::isType<Node, EnableNode>(srcNode) == nullptr){
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
                throw std::runtime_error("Traceback marked an arc twice.  Should never occur.");
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

        }//else, stop traversal.  This is a state element or enable node.  Do not mark and do not recurse
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

        //Check if the dst node is a state element or a enable node (input or output)
        if(!(dstNode->hasState()) && GeneralHelper::isType<Node, EnableNode>(dstNode) == nullptr){
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
                throw std::runtime_error("Traceback marked an arc twice.  Should never occur.");
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

        }//else, stop traversal.  This is a state element or enable node.  Do not mark and do not recurse
    }

    return contextNodes;
}

void GraphAlgs::moveNodePreserveHierarchy(std::shared_ptr<Node> nodeToMove, std::shared_ptr<SubSystem> moveUnder,
                                          std::vector<std::shared_ptr<Node>> &newNodes, std::string moveSuffix) {

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

void GraphAlgs::discoverAndUpdateContexts(std::set<std::shared_ptr<Node>> nodesToSearch,
                                          std::vector<Context> contextStack,
                                          std::vector<std::shared_ptr<Mux>> &discoveredMux,
                                          std::vector<std::shared_ptr<EnabledSubSystem>> &discoveredEnabledSubSystems,
                                          std::vector<std::shared_ptr<Node>> &discoveredGeneral) {

    for(auto it = nodesToSearch.begin(); it != nodesToSearch.end(); it++){
        //Set the context
        (*it)->setContext(contextStack);

        //Recurse
        if(GeneralHelper::isType<Node, Mux>(*it) != nullptr){
            discoveredMux.push_back(std::dynamic_pointer_cast<Mux>(*it));
        }else if(GeneralHelper::isType<Node, EnabledSubSystem>(*it) != nullptr){//Check this first because EnabledSubSystems are SubSystems
            discoveredEnabledSubSystems.push_back(std::dynamic_pointer_cast<EnabledSubSystem>(*it));
        }else if(GeneralHelper::isType<Node, SubSystem>(*it) != nullptr){
            std::shared_ptr<SubSystem> subSystem = std::dynamic_pointer_cast<SubSystem>(*it);
            subSystem->discoverAndUpdateContexts(contextStack, discoveredMux, discoveredEnabledSubSystems,
                                                 discoveredGeneral);
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
            std::set<std::shared_ptr<Node>> childrenSet = subSystem->getChildren();
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

std::vector<std::shared_ptr<Node>> GraphAlgs::topologicalSortDestructive(std::vector<std::shared_ptr<Node>> nodesToSort,
                                                                         std::vector<std::shared_ptr<Arc>> &arcsToDelete,
                                                                         std::shared_ptr<MasterOutput> outputMaster,
                                                                         std::shared_ptr<MasterInput> inputMaster,
                                                                         std::shared_ptr<MasterOutput> terminatorMaster,
                                                                         std::shared_ptr<MasterUnconnected> unconnectedMaster,
                                                                         std::shared_ptr<MasterOutput> visMaster) {
    std::vector<std::shared_ptr<Node>> schedule;

    //Find nodes with 0 in degree at this context level (and not in nested contexts)
    std::set<std::shared_ptr<Node>> nodesWithZeroInDeg;
    for(unsigned long i = 0; i<nodesToSort.size(); i++){
        //Do not add subsystems to the list of zero in degree nodes, they do not need to be scheduled.  The nodes within them do.
        if(GeneralHelper::isType<Node, SubSystem>(nodesToSort[i]) == nullptr) {
            unsigned long inDeg = nodesToSort[i]->inDegree();
            if (inDeg == 0) {
                nodesWithZeroInDeg.insert(nodesToSort[i]);
            }
        }
    }

    //Handle the case where the output is only connected to nodes with state (or is unconnected).  When this happens, all the arcs
    //to the output are removed and it is its own connected component.  It will never be considered a candidate.  Add it to the
    //list of nodes with zero in deg if this is the case.  Note that if there is at least 1 arc to a node without state, it will be
    //considered a candidate
    if(outputMaster->inDegree() == 0){
        nodesWithZeroInDeg.insert(outputMaster);
    }

    //Find Candidate Nodes
    std::set<std::shared_ptr<Node>> candidateNodes;
    for(auto it = nodesWithZeroInDeg.begin(); it != nodesWithZeroInDeg.end(); it++){
        std::set<std::shared_ptr<Node>> moreCandidates = (*it)->getConnectedNodes();
        candidateNodes.insert(moreCandidates.begin(), moreCandidates.end());
    }

    //Remove the master nodes from the candidate list as well as any nodes that are about to be removed
    candidateNodes.erase(unconnectedMaster);
    candidateNodes.erase(terminatorMaster);
    candidateNodes.erase(visMaster);
    candidateNodes.erase(inputMaster);
//    candidateNodes.erase(outputMaster); //Actually, schedule this master

    //Schedule Nodes
    while(!nodesWithZeroInDeg.empty()){
        //Schedule Nodes with Zero In Degree
        //Disconnect, erase nodes, remove from candidate set (if it is included)
        for(auto it = nodesWithZeroInDeg.begin(); it != nodesWithZeroInDeg.end(); it++){
            //Disconnect the node
            std::set<std::shared_ptr<Arc>> arcsToRemove = (*it)->disconnectNode();
            arcsToDelete.insert(arcsToDelete.end(), arcsToRemove.begin(), arcsToRemove.end());

            //====Check if the node is a ContextContainerFamily====
            if(GeneralHelper::isType<Node, ContextFamilyContainer>(*it) != nullptr){
                std::shared_ptr<ContextFamilyContainer> familyContainer = std::static_pointer_cast<ContextFamilyContainer>(*it);

                //Recursively schedule the nodes in this ContextContainerFamily
                //Schedule the subcontexts first

                std::vector<std::shared_ptr<ContextContainer>> subContextContainers = familyContainer->getSubContextContainers();
                for(unsigned long i = 0; i<subContextContainers.size(); i++){
                    std::set<std::shared_ptr<Node>> childrenSet = subContextContainers[i]->getChildren();
                    std::vector<std::shared_ptr<Node>> childrenVector;
                    childrenVector.insert(childrenVector.end(), childrenSet.begin(), childrenSet.end());

                    std::vector<std::shared_ptr<Node>> subSched = GraphAlgs::topologicalSortDestructive(childrenVector, arcsToDelete, outputMaster, inputMaster, terminatorMaster, unconnectedMaster, visMaster);
                    //Add to schedule
                    schedule.insert(schedule.end(), subSched.begin(), subSched.end());
                }

                //Schedule the contextRoot
                std::shared_ptr<ContextRoot> contextRoot = familyContainer->getContextRoot();
                std::vector<std::shared_ptr<Node>> nodesToSched;

                if(contextRoot == nullptr){
                    throw std::runtime_error("Tried to schedule a ContextRoot that is null");
                }else if(GeneralHelper::isType<ContextRoot, Mux>(contextRoot) != nullptr){
                    //The context root is a mux, we just schedule this
                    nodesToSched.push_back(std::dynamic_pointer_cast<Mux>(contextRoot)); //TODO: fix diamond inheritance issue
                }else if(GeneralHelper::isType<ContextRoot, EnabledSubSystem>(contextRoot) != nullptr){
                    //Add the subsystem and EnableNodes
                    std::shared_ptr<EnabledSubSystem> asEnabledSubsystem = std::dynamic_pointer_cast<EnabledSubSystem>(contextRoot); //TODO: fix diamond inheritance issue
                    nodesToSched.push_back(asEnabledSubsystem);

                    //Add EnableInputs and EnableOutputs
                    std::vector<std::shared_ptr<EnableInput>> enableInputs = asEnabledSubsystem->getEnableInputs();
                    nodesToSched.insert(nodesToSched.end(), enableInputs.begin(), enableInputs.end());

                    std::vector<std::shared_ptr<EnableOutput>> enableOutputs = asEnabledSubsystem->getEnableOutputs();
                    nodesToSched.insert(nodesToSched.end(), enableOutputs.begin(), enableOutputs.end());
                }else{
                    throw std::runtime_error("When scheduling, a context root was encountered which is not yet implemented");
                }

                //Schedule context root
                std::vector<std::shared_ptr<Node>> contexRootSched = GraphAlgs::topologicalSortDestructive(nodesToSched, arcsToDelete, outputMaster, inputMaster, terminatorMaster, unconnectedMaster, visMaster);
                //Add to schedule
                schedule.insert(schedule.end(), contexRootSched.begin(), contexRootSched.end());

            }else{//----End node is a ContextContainerFamily----
                schedule.push_back(*it);
            }

            candidateNodes.erase(*it);
        }

        //Reset nodes with zero in degree
        nodesWithZeroInDeg.clear();

        //Find nodes with zero in degree from candidates list
        for(auto it = candidateNodes.begin(); it != candidateNodes.end(); it++){
            std::shared_ptr<Node> candidateNode = *it;
            if(candidateNode->inDegree() == 0){
                nodesWithZeroInDeg.insert(candidateNode);
            }
        }

        //Update candidates list
        for(auto it = nodesWithZeroInDeg.begin(); it != nodesWithZeroInDeg.end(); it++){
            std::shared_ptr<Node> zeroInDegNode = *it;
            std::set<std::shared_ptr<Node>> newCandidates = zeroInDegNode->getConnectedNodes();
            candidateNodes.insert(newCandidates.begin(), newCandidates.end());
        }

        //Remove master nodes from candidates list
        candidateNodes.erase(unconnectedMaster);
        candidateNodes.erase(terminatorMaster);
        candidateNodes.erase(visMaster);
        candidateNodes.erase(inputMaster);
//        candidateNodes.erase(outputMaster); //Actually, schedule this master

    }

    //If there are still viable candidate nodes, there was a cycle.
    if(!candidateNodes.empty()){
        throw std::runtime_error("Topological Sort: Encountered Cycle, Unable to Sort");
    }

    return schedule;
}
