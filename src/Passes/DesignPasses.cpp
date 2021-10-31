//
// Created by Christopher Yarp on 9/2/21.
//

#include "DesignPasses.h"
#include "GraphCore/Node.h"
#include "General/ErrorHelpers.h"
#include "General/GeneralHelper.h"
#include "GraphCore/ContextRoot.h"
#include "GraphCore/SubSystem.h"
#include "MasterNodes/MasterUnconnected.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterInput.h"
#include "GraphCore/ContextFamilyContainer.h"
#include "GraphCore/StateUpdate.h"
#include "General/GraphAlgs.h"
#include <iostream>

void DesignPasses::createStateUpdateNodes(Design &design, bool includeContext) {
    //Find nodes with state in design
    std::vector<std::shared_ptr<Node>> nodesWithState = design.findNodesWithState();

    std::vector<std::shared_ptr<Node>> newNodes;
    std::vector<std::shared_ptr<Node>> deletedNodes;
    std::vector<std::shared_ptr<Arc>> newArcs;
    std::vector<std::shared_ptr<Arc>> deletedArcs;

    for(unsigned long i = 0; i<nodesWithState.size(); i++){
        nodesWithState[i]->createStateUpdateNode(newNodes, deletedNodes, newArcs, deletedArcs, includeContext);
    }

    design.addRemoveNodesAndArcs(newNodes, deletedNodes, newArcs, deletedArcs);
}

void DesignPasses::orderConstrainZeroInputNodes(Design &design){

    std::vector<std::shared_ptr<Node>> new_nodes;
    std::vector<std::shared_ptr<Node>> deleted_nodes;
    std::vector<std::shared_ptr<Arc>> new_arcs;
    std::vector<std::shared_ptr<Arc>> deleted_arcs;

    {
        std::vector<std::shared_ptr<Node>> nodes = design.getNodes();
        //Call the recursive function on any subsystem at the top level
        for (unsigned long i = 0; i < nodes.size(); i++) {
            if (nodes[i]->inDegree() == 0) {
                //Needs to be order constrained if in a context

                std::vector<Context> contextStack = nodes[i]->getContext();
                if (contextStack.size() > 0) {
                    //Create new order constraint arcs for this node.

                    //Run through the context stack
                    for (unsigned long j = 0; j < contextStack.size(); j++) {
                        std::shared_ptr<ContextRoot> contextRoot = contextStack[j].getContextRoot();
                        std::vector<std::shared_ptr<Arc>> driverArcs = contextRoot->getContextDriversForPartition(
                                nodes[i]->getPartitionNum());
                        if (driverArcs.size() == 0 && contextRoot->getContextDecisionDriver().size() > 0) {
                            throw std::runtime_error(ErrorHelpers::genErrorStr(
                                    "When order constraining this node, found no context decision drivers for the partition it is in but found general context driver.  This function was likely called before partitioning and/or encapsulation",
                                    nodes[i]));
                        }

                        for (unsigned long k = 0; k < driverArcs.size(); k++) {
                            std::shared_ptr<OutputPort> driverSrc = driverArcs[k]->getSrcPort();
                            std::shared_ptr<Arc> orderConstraintArc = Arc::connectNodes(driverSrc,
                                                                                        nodes[i]->getOrderConstraintInputPortCreateIfNot(),
                                                                                        driverArcs[k]->getDataType(),
                                                                                        driverArcs[k]->getSampleTime());
                            new_arcs.push_back(orderConstraintArc);
                        }
                    }
                }
            }
        }
    }

    design.addRemoveNodesAndArcs(new_nodes, deleted_nodes, new_arcs, deleted_arcs);
}

void DesignPasses::cleanupEmptyHierarchy(Design &design, std::string reason){
    std::vector<std::shared_ptr<SubSystem>> emptySubsystems;

    {
        std::vector<std::shared_ptr<Node>> nodes = design.getNodes();
        for (int i = 0; i < nodes.size(); i++) {
            std::shared_ptr<SubSystem> asSubsys = GeneralHelper::isType<Node, SubSystem>(nodes[i]);
            std::shared_ptr<ContextRoot> asContextRoot = GeneralHelper::isType<Node, ContextRoot>(nodes[i]);
            std::shared_ptr<ContextFamilyContainer> asContextFamilyContainer = GeneralHelper::isType<Node, ContextFamilyContainer>(
                    nodes[i]);
            std::shared_ptr<ContextContainer> asContextContainer = GeneralHelper::isType<Node, ContextContainer>(
                    nodes[i]);
            if (asSubsys != nullptr && asContextRoot == nullptr && asContextFamilyContainer == nullptr &&
                asContextContainer == nullptr) {
                if (asSubsys->getChildren().empty()) {
                    emptySubsystems.push_back(asSubsys);
                }
            }
        }
    }

    std::vector<std::shared_ptr<Node>> emptyNodeVector;
    std::vector<std::shared_ptr<Arc>> emptyArcVector;

    for(int i = 0; i<emptySubsystems.size(); i++){
        std::cout << "Subsystem " << emptySubsystems[i]->getFullyQualifiedName() << " was removed because " << reason << std::endl;
        std::shared_ptr<SubSystem> parent = emptySubsystems[i]->getParent();

        std::vector<std::shared_ptr<Node>> nodesToRemove = {emptySubsystems[i]};
        design.addRemoveNodesAndArcs(emptyNodeVector, nodesToRemove, emptyArcVector, emptyArcVector);

        while(parent != nullptr && GeneralHelper::isType<Node, ContextRoot>(parent) == nullptr && GeneralHelper::isType<Node, ContextFamilyContainer>(parent) == nullptr && GeneralHelper::isType<Node, ContextContainer>(parent) == nullptr && parent->getChildren().empty()){//Should short circuit before checking for children if parent is null
            //This subsystem has no children, it should be removed
            std::vector<std::shared_ptr<Node>> nodesToRemove = {parent};
            std::cout << "Subsystem " << parent->getFullyQualifiedName() << " was removed because " << reason << std::endl;

            //Get its parent before removing it
            parent = parent->getParent();
            design.addRemoveNodesAndArcs(emptyNodeVector, nodesToRemove, emptyArcVector, emptyArcVector);
        }
    }
}

void DesignPasses::pruneUnconnectedArcs(Design &design, bool removeVisArcs) {
    std::set<std::shared_ptr<OutputPort>> outputPortsWithArcDisconnected;

    //Unconnected master
    std::set<std::shared_ptr<Arc>> arcsToDisconnectFromUnconnectedMaster = design.getUnconnectedMaster()->getInputArcs();
    for(auto it = arcsToDisconnectFromUnconnectedMaster.begin(); it != arcsToDisconnectFromUnconnectedMaster.end(); it++){
        outputPortsWithArcDisconnected.insert((*it)->getSrcPort());
    }
    std::set<std::shared_ptr<Arc>> unconnectedArcsSet = design.getUnconnectedMaster()->disconnectNode();
    std::vector<std::shared_ptr<Arc>> unconnectedArcs;
    unconnectedArcs.insert(unconnectedArcs.end(), unconnectedArcsSet.begin(), unconnectedArcsSet.end());
    std::vector<std::shared_ptr<Arc>> emptyArcSet;
    std::vector<std::shared_ptr<Node>> emptyNodeSet;
    design.addRemoveNodesAndArcs(emptyNodeSet, emptyNodeSet, emptyArcSet, unconnectedArcs);

    //Terminator master
    std::set<std::shared_ptr<Arc>> arcsToDisconnectFromTerminatorMaster = design.getTerminatorMaster()->getInputArcs();
    for(auto it = arcsToDisconnectFromTerminatorMaster.begin(); it != arcsToDisconnectFromTerminatorMaster.end(); it++){
        outputPortsWithArcDisconnected.insert((*it)->getSrcPort());
    }
    std::set<std::shared_ptr<Arc>> terminatedArcsSet = design.getTerminatorMaster()->disconnectNode();
    std::vector<std::shared_ptr<Arc>> terminatedArcs;
    terminatedArcs.insert(terminatedArcs.end(), terminatedArcsSet.begin(), terminatedArcsSet.end());
    design.addRemoveNodesAndArcs(emptyNodeSet, emptyNodeSet, emptyArcSet, terminatedArcs);

    //Vis Master (TODO: Re-enable later?)
    if(removeVisArcs) {
        std::set<std::shared_ptr<Arc>> arcsToDisconnectFromVisMaster = design.getVisMaster()->getInputArcs();
        for (auto it = arcsToDisconnectFromVisMaster.begin(); it != arcsToDisconnectFromVisMaster.end(); it++) {
            outputPortsWithArcDisconnected.insert((*it)->getSrcPort());
        }
        std::set<std::shared_ptr<Arc>> visArcsSet = design.getVisMaster()->disconnectNode();
        std::vector<std::shared_ptr<Arc>> visArcs;
        visArcs.insert(visArcs.end(), visArcsSet.begin(), visArcsSet.end());
        design.addRemoveNodesAndArcs(emptyNodeSet, emptyNodeSet, emptyArcSet, visArcs);
    }

    //Emit a notification of ports that are disconnected (pruned)
    //TODO: clean this up so that the disconnect warning durring emit refers back to the origional node path
    for(auto it = outputPortsWithArcDisconnected.begin(); it != outputPortsWithArcDisconnected.end(); it++) {
        if((*it)->getArcs().empty()) {
            std::cout << "Pruned: All Arcs from Output Port " << (*it)->getPortNum() << " of " <<
                      (*it)->getParent()->getFullyQualifiedName(true) << " [ID: " << (*it)->getParent()->getId() << "]"
                      << std::endl;
        }
    }
}

unsigned long DesignPasses::prune(Design &design, bool includeVisMaster) {
    //TODO: Check connected components to make sure that there is a link to an output.  If not, prune connected component.  This eliminates cycles that do no useful work.

    //Want sets to have a consistent ordering across runs.  Therefore, will not use pointer address for set comparison

    //Find nodes with 0 out-degree when not counting the various master nodes
    std::set<std::shared_ptr<Node>> nodesToIgnore; //This can be a standard set as order is unimportant for checking against the ignore set
    nodesToIgnore.insert(design.getUnconnectedMaster());
    nodesToIgnore.insert(design.getTerminatorMaster());
    if(includeVisMaster){
        nodesToIgnore.insert(design.getVisMaster());
    }

    std::set<std::shared_ptr<Node>, Node::PtrID_Compare> nodesWithZeroOutDeg;
    {
        std::vector<std::shared_ptr<Node>> nodes = design.getNodes();
        for (unsigned long i = 0; i < nodes.size(); i++) {
            //Do not remove subsystems or state updates (they will have outdeg 0)
            if (GeneralHelper::isType<Node, SubSystem>(nodes[i]) == nullptr &&
                GeneralHelper::isType<Node, StateUpdate>(nodes[i]) == nullptr) {
                if (nodes[i]->outDegreeExclusingConnectionsTo(nodesToIgnore) == 0) {
                    nodesWithZeroOutDeg.insert(nodes[i]);
                }
            }
        }
    }

    //Get the candidate nodes which may have out-deg 0 after removing the nodes
    //Note that all of the connected nodes have zero out degree so connected nodes are either connected to the input ports or are the master nodes
    std::set<std::shared_ptr<Node>, Node::PtrID_Compare> candidateNodes;
    for(auto it = nodesWithZeroOutDeg.begin(); it != nodesWithZeroOutDeg.end(); it++){
        std::set<std::shared_ptr<Node>> moreCandidates = (*it)->getConnectedNodes(); //Should be ok if this is ordered by ptr since it is being inserted into the set ordered by ID
        candidateNodes.insert(moreCandidates.begin(), moreCandidates.end());
    }

    //Remove the master nodes from the candidate list as well as any nodes that are about to be removed
    candidateNodes.erase(design.getUnconnectedMaster());
    candidateNodes.erase(design.getTerminatorMaster());
    candidateNodes.erase(design.getVisMaster());
    candidateNodes.erase(design.getInputMaster());
    candidateNodes.erase(design.getOutputMaster());

    //Erase
    std::vector<std::shared_ptr<Node>> nodesDeleted;
    std::vector<std::shared_ptr<Arc>> arcsDeleted;

//    for(auto it = nodesWithZeroOutDeg.begin(); it != nodesWithZeroOutDeg.end(); it++){
//        std::cout << "Node with Indeg 0: " <<  (*it)->getFullyQualifiedName() << std::endl;
//    }

    while(!nodesWithZeroOutDeg.empty()){
        //Disconnect, erase nodes, remove from candidate set (if it is included)
        for(auto it = nodesWithZeroOutDeg.begin(); it != nodesWithZeroOutDeg.end(); it++){
            std::set<std::shared_ptr<Arc>> arcsToRemove = (*it)->disconnectNode();
            //All nodes need to be removed from their parent while some node, such as EnableInput and EnableOutput,
            //need to remove additional references to themselves before being deleted.  These tasks are now handled
            //by removeKnownReferences
            (*it)->removeKnownReferences();
            //Now integrated into design.addRemoveNodesAndArcs but will keep here so that any removed references are considered in the next round of pruning

            arcsDeleted.insert(arcsDeleted.end(), arcsToRemove.begin(), arcsToRemove.end());
            nodesDeleted.push_back(*it);
            candidateNodes.erase(*it);
        }

        //Reset nodes with zero out degree
        nodesWithZeroOutDeg.clear();

        //Find nodes with zero out degree from candidates list
        for(auto it = candidateNodes.begin(); it != candidateNodes.end(); it++){
            if((*it)->outDegreeExclusingConnectionsTo(nodesToIgnore) == 0){
                nodesWithZeroOutDeg.insert(*it);
            }
        }

        //Update candidates list
        for(auto it = nodesWithZeroOutDeg.begin(); it != nodesWithZeroOutDeg.end(); it++){
            std::set<std::shared_ptr<Node>> newCandidates = (*it)->getConnectedNodes();
            candidateNodes.insert(newCandidates.begin(), newCandidates.end());
        }

        //Remove master nodes from candidates list
        candidateNodes.erase(design.getUnconnectedMaster());
        candidateNodes.erase(design.getTerminatorMaster());
        candidateNodes.erase(design.getVisMaster());
        candidateNodes.erase(design.getInputMaster());
        candidateNodes.erase(design.getOutputMaster());
    }

    //Delete nodes and arcs from design
    for(auto it = nodesDeleted.begin(); it != nodesDeleted.end(); it++){
        std::shared_ptr<Node> nodeToDelete = *it;
        std::cout << "Pruned Node: " << nodeToDelete->getFullyQualifiedName(true) << " [ID: " << nodeToDelete->getId() << "]" << std::endl;
    }

    std::vector<std::shared_ptr<Node>> emptyNodeVec;
    std::vector<std::shared_ptr<Arc>> emptyArcVec;
    design.addRemoveNodesAndArcs(emptyNodeVec, nodesDeleted, emptyArcVec, arcsDeleted);

    //Connect unconnected arcs to unconnected master node
    {
        std::vector<std::shared_ptr<Node>> nodes = design.getNodes();
        for (unsigned long i = 0; i < nodes.size(); i++) {
            std::vector<std::shared_ptr<Arc>> newArcs =
                    nodes[i]->connectUnconnectedPortsToNode(design.getUnconnectedMaster(), design.getUnconnectedMaster(), 0, 0);

            //Add newArcs to arc vector
            design.addRemoveNodesAndArcs(emptyNodeVec, emptyNodeVec, newArcs, emptyArcVec);
        }
    }

    return nodesDeleted.size();
}

void DesignPasses::assignPartitionsToUnassignedSubsystems(Design &design, bool printWarning, bool errorIfUnableToSet){
    std::vector<std::shared_ptr<Node>> nodes = design.getNodes();

    for(const std::shared_ptr<Node> &node : nodes){
        std::shared_ptr<SubSystem> asSubsys = GeneralHelper::isType<Node, SubSystem>(node);
        if(asSubsys){
            if(asSubsys->getPartitionNum() == -1){
                int partNum = GraphAlgs::findPartitionInSubSystem(asSubsys);
                if(partNum == -1 && errorIfUnableToSet){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Unable to find partition for Subsystem.  Ensure that there is at least one node nested in the subsystem which has a partition assigned"));
                }
                asSubsys->setPartitionNum(partNum);
                if(printWarning){
                    std::cerr << ErrorHelpers::genWarningStr("Setting Unassigned Subsystem (" + asSubsys->getName() + ") to Partition " + GeneralHelper::to_string(partNum), asSubsys) << std::endl;
                }
            }
        }
    }
}

void DesignPasses::assignSubBlockingLengthToUnassignedSubsystems(Design &design, bool printWarning, bool errorIfUnableToSet){
    std::vector<std::shared_ptr<Node>> nodes = design.getNodes();

    for(const std::shared_ptr<Node> &node : nodes){
        std::shared_ptr<SubSystem> asSubsys = GeneralHelper::isType<Node, SubSystem>(node);
        if(asSubsys){
            if(asSubsys->getBaseSubBlockingLen() == -1){
                int subBlockingLength = GraphAlgs::findBaseSubBlockingLengthInSubsystem(asSubsys);
                if(subBlockingLength == -1 && errorIfUnableToSet){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Unable to find base sub-blocking length for Subsystem.  Ensure that there is at least one node nested in the subsystem which has a partition assigned"));
                }
                asSubsys->setBaseSubBlockingLen(subBlockingLength);
                if(printWarning){
                    std::cerr << ErrorHelpers::genWarningStr("Setting Unassigned Subsystem (" + asSubsys->getName() + ") Base Sub-Blocking Length " + GeneralHelper::to_string(subBlockingLength), asSubsys) << std::endl;
                }
            }
        }
    }
}