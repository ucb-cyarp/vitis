//
// Created by Christopher Yarp on 9/3/21.
//

#include "IntraPartitionScheduling.h"
#include <iostream>
#include "GraphCore/StateUpdate.h"
#include "PrimitiveNodes/Constant.h"
#include "General/GraphAlgs.h"
#include "GraphMLTools/GraphMLExporter.h"
#include "General/ErrorHelpers.h"
#include "PrimitiveNodes/BlackBox.h"
#include "Passes/ContextPasses.h"
#include "Passes/DesignPasses.h"
#include "MultiThread/ThreadCrossingFIFO.h"

unsigned long IntraPartitionScheduling::scheduleTopologicalStort(Design &design, TopologicalSortParameters params, bool prune, bool rewireContexts, std::string designName, std::string dir, bool printNodeSched, bool schedulePartitions) {
    //TODO: WARNING: UpsampleClockDomains currently rely on all of their nodes being scheduled together (ie not being split up).
    //This currently is provided by the hierarchical implementation of the scheduler.  However, if this were to be changed
    //later, a method for having vector intermediates would be required.

    std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> origToClonedNodes;
    std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> clonedToOrigNodes;
    std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> origToClonedArcs;
    std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> clonedToOrigArcs;

    //Make a copy of the design to conduct the destructive topological sort on
    Design designClone = design.copyGraph(origToClonedNodes, clonedToOrigNodes, origToClonedArcs, clonedToOrigArcs);

    unsigned long numNodesPruned=0;
    if(prune){
        numNodesPruned = DesignPasses::prune(designClone, true);
        std::cerr << "Nodes Pruned: " << numNodesPruned << std::endl;
    }

    //==== Remove input master and constants.  Disconnect output arcs from nodes with state (that do not contain combo paths)====
    //++++ Note: excpetions explained below ++++
    std::set<std::shared_ptr<Arc>> arcsToDelete = designClone.getInputMaster()->disconnectNode();
    std::set<std::shared_ptr<Node>> nodesToRemove;

    {
        std::vector<std::shared_ptr<Node>> nodes = designClone.getNodes();
        for (unsigned long i = 0; i < nodes.size(); i++) {
            if (GeneralHelper::isType<Node, Constant>(nodes[i]) != nullptr) {
                //This is a constant node, disconnect it and remove it from the graph to be ordered
                std::set<std::shared_ptr<Arc>> newArcsToDelete = nodes[i]->disconnectNode();
                arcsToDelete.insert(newArcsToDelete.begin(), newArcsToDelete.end());

                nodesToRemove.insert(nodes[i]);
            } else if (GeneralHelper::isType<Node, BlackBox>(nodes[i]) != nullptr) {
                std::shared_ptr<BlackBox> asBlackBox = std::dynamic_pointer_cast<BlackBox>(nodes[i]);

                //We need to treat black boxes a little differently.  They may have state but their outputs may not
                //be registered (could be combinationally dependent on the input signals).
                //We should only disconnect arcs to outputs that are registered.

                std::vector<int> registeredPortNumbers = asBlackBox->getRegisteredOutputPorts();

                std::vector<std::shared_ptr<OutputPort>> outputPorts = nodes[i]->getOutputPorts();
                for (unsigned long j = 0; j < outputPorts.size(); j++) {
                    //Check if the port number is in the list of registered output ports
                    if (std::find(registeredPortNumbers.begin(), registeredPortNumbers.end(),
                                  outputPorts[j]->getPortNum()) != registeredPortNumbers.end()) {
                        //This port is registered, its output arcs can be removed

                        std::set<std::shared_ptr<Arc>> outputArcs = outputPorts[j]->getArcs();
                        for (auto it = outputArcs.begin(); it != outputArcs.end(); it++) {
                            std::shared_ptr<StateUpdate> dstAsStateUpdate = GeneralHelper::isType<Node, StateUpdate>(
                                    (*it)->getDstPort()->getParent());
                            if (dstAsStateUpdate == nullptr || (dstAsStateUpdate != nullptr &&
                                                                dstAsStateUpdate->getPrimaryNode() !=
                                                                (*it)->getSrcPort()->getParent())) {
                                (*it)->disconnect();
                                arcsToDelete.insert(*it);
                            }
                            //Else, this is a State node connected to its StateUpdate node and should not be removed
                        }
                    }
                }

            } else if (nodes[i]->hasState() && !nodes[i]->hasCombinationalPath()) {
                //Do not disconnect enabled outputs even though they have state.  They are actually more like transparent latches and do pass signals directly (within the same cycle) when the subsystem is enabled.  Otherwise, the pass the previous output
                //Disconnect output arcs (we still need to calculate the inputs to the state element, however the output of the output appears like a constant from a scheduling perspecitve)

                //Thread crossing FIFOs will have their outputs disconnected by this method as they declare that they have state and no combinational path

                //Note, arcs to state update nodes should not be removed (except for ThreadCrossingFIFOs).

                std::set<std::shared_ptr<Arc>> outputArcs = nodes[i]->getOutputArcs();
                for (auto it = outputArcs.begin(); it != outputArcs.end(); it++) {
                    //Check if the dst is a state update.  Keep arcs to state update nodes
                    std::shared_ptr<StateUpdate> dstAsStateUpdate = GeneralHelper::isType<Node, StateUpdate>(
                            (*it)->getDstPort()->getParent());
                    if (dstAsStateUpdate == nullptr) {
                        (*it)->disconnect();
                        arcsToDelete.insert(*it);
                    } else if (GeneralHelper::isType<Node, ThreadCrossingFIFO>(nodes[i])) {
                        //The destination is a state update but the src is a ThreadCrossingFIFO.
                        //The ThreadCrossingFIFO data should be from another partition and the dependency is effectively handled by
                        //reading rhe ThreadCrossingFIFO at the start of function execution (or on demand during execution if that
                        //model is implemented).  The possible exception to this is when the ThreadCrossingFIFO's input is
                        //a stateful element in which case an order constraint arc is possible and should be honnored.  Note that
                        //ThreadCrossingFIFOs exist in the src partition of the transfer.  It should not be possible for the FIFO's own
                        //state update node to be in a different partition.  This condition will be checked.
                        //Therefore, the arc will be removed only if the ThreadCrossingFIFO and StateUpdate are in different partitions.

                        if (nodes[i]->getPartitionNum() != dstAsStateUpdate->getPartitionNum()) {
                            if (dstAsStateUpdate->getPrimaryNode() == nodes[i]) {
                                throw std::runtime_error(ErrorHelpers::genErrorStr(
                                        "Arc from ThreadCrossingFIFO to its own state update in another partition",
                                        nodes[i]));
                            } else {
                                (*it)->disconnect();
                                arcsToDelete.insert(*it);
                            }
                        }
                        //Else, this arc to the state update
                    }
                    //Else, this is a State node connected to its StateUpdate node and should not be removed
                }
            }
        }
    }

    //Remove Nodes and Arcs
    std::set<std::shared_ptr<Node>> emptyNodeVec;
    std::set<std::shared_ptr<Arc>> emptyArcVec;
    designClone.addRemoveNodesAndArcs(emptyNodeVec, nodesToRemove, emptyArcVec, arcsToDelete);

    //Rewire the remaining arcs in the designClone
    if(rewireContexts){
        std::vector<std::shared_ptr<Arc>> origArcs;
        std::vector<std::vector<std::shared_ptr<Arc>>> newArcsGrouped;

        ContextPasses::rewireArcsToContexts(designClone, origArcs, newArcsGrouped);

        std::vector<std::shared_ptr<Arc>> newArcs;
        for(unsigned long i = 0; i<newArcsGrouped.size(); i++){
            for(unsigned long j = 0; j<newArcsGrouped[i].size(); j++){
                newArcs.push_back(newArcsGrouped[i][j]);
            }
        }

        //Disconnect and Remove the orig arcs for scheduling
        std::vector<std::shared_ptr<Node>> emptyNodeVector;
        std::vector<std::shared_ptr<Arc>> emptyArcVector;
        for(unsigned long i = 0; i<origArcs.size(); i++){
            origArcs[i]->disconnect();
        }
        designClone.addRemoveNodesAndArcs(emptyNodeVector, emptyNodeVector, newArcs, origArcs);
        designClone.assignArcIDs();
    }

    //==== Topological Sort (Destructive) ====
    if(schedulePartitions) {
        std::set<int> partitions = design.listPresentPartitions();

        for(auto partitionBeingScheduled = partitions.begin(); partitionBeingScheduled != partitions.end(); partitionBeingScheduled++) {
            std::vector<std::shared_ptr<Node>> schedule = topologicalSortDestructive(designClone, designName, dir, params, *partitionBeingScheduled);

            if (printNodeSched) {
                std::cout << "Schedule [Partition: " << *partitionBeingScheduled << "]" << std::endl;
                for (unsigned long i = 0; i < schedule.size(); i++) {
                    std::cout << i << ": " << schedule[i]->getFullyQualifiedName() << std::endl;
                }
                std::cout << std::endl;
            }

            //==== Back Propagate Schedule ====
            for (unsigned long i = 0; i < schedule.size(); i++) {
                //Index is the schedule number
                std::shared_ptr<Node> origNode = clonedToOrigNodes[schedule[i]];
                origNode->setSchedOrder(i);
            }
        }
    }else{
        std::vector<std::shared_ptr<Node>> schedule = topologicalSortDestructive(designClone, designName, dir, params);

        if (printNodeSched) {
            std::cout << "Schedule" << std::endl;
            for (unsigned long i = 0; i < schedule.size(); i++) {
                std::cout << i << ": " << schedule[i]->getFullyQualifiedName() << std::endl;
            }
            std::cout << std::endl;
        }

        //==== Back Propagate Schedule ====
        for (unsigned long i = 0; i < schedule.size(); i++) {
            //Index is the schedule number
            std::shared_ptr<Node> origNode = clonedToOrigNodes[schedule[i]];
            origNode->setSchedOrder(i);
        }
    }

    return numNodesPruned;
}

std::vector<std::shared_ptr<Node>> IntraPartitionScheduling::topologicalSortDestructive(Design &design, std::string designName, std::string dir, TopologicalSortParameters params) {
    std::vector<std::shared_ptr<Arc>> arcsToDelete;
    std::vector<std::shared_ptr<Node>> topLevelContextNodes = GraphAlgs::findNodesStopAtContextFamilyContainers(design.getTopLevelNodes());
    topLevelContextNodes.push_back(design.getOutputMaster());

    std::vector<std::shared_ptr<Node>> sortedNodes;

    bool failed = false;
    std::exception err;
    try {
        sortedNodes = GraphAlgs::topologicalSortDestructive(params,
                                                            topLevelContextNodes,
                                                            false,
                                                            -1,
                                                            arcsToDelete,
                                                            design.getOutputMaster(),
                                                            design.getInputMaster(),
                                                            design.getTerminatorMaster(),
                                                            design.getUnconnectedMaster(),
                                                            design.getVisMaster());
    }catch(const std::exception &e){
        failed = true;
        err = e;
    }

    //Delete the arcs
    //TODO: Remove this?  May not be needed for most applications.  We are probably going to destroy the design anyway
    std::vector<std::shared_ptr<Node>> emptyNodeVec;
    std::vector<std::shared_ptr<Arc>> emptyArcVec;
    design.addRemoveNodesAndArcs(emptyNodeVec, emptyNodeVec, emptyArcVec, arcsToDelete);

    if(failed) {
        std::cout << "Emitting: " + dir + "/" + designName + "_sort_error_scheduleGraph.graphml" << std::endl;
        GraphMLExporter::exportGraphML(dir + "/" + designName + "_sort_error_scheduleGraph.graphml", design);

        throw err;
    }

    return sortedNodes;
}

std::vector<std::shared_ptr<Node>> IntraPartitionScheduling::topologicalSortDestructive(Design &design, std::string designName, std::string dir, TopologicalSortParameters params, int partitionNum) {
    std::vector<std::shared_ptr<Arc>> arcsToDelete;
    std::vector<std::shared_ptr<Node>> topLevelContextNodesInPartition = GraphAlgs::findNodesStopAtContextFamilyContainers(design.getTopLevelNodes(), partitionNum);

    if(partitionNum == IO_PARTITION_NUM) {
        topLevelContextNodesInPartition.push_back(design.getOutputMaster());
        topLevelContextNodesInPartition.push_back(design.getVisMaster());
    }

    std::vector<std::shared_ptr<Node>> sortedNodes;

    bool failed = false;
    std::exception err;
    try {
        sortedNodes = GraphAlgs::topologicalSortDestructive(params,
                                                            topLevelContextNodesInPartition,
                                                            true,
                                                            partitionNum,
                                                            arcsToDelete,
                                                            design.getOutputMaster(),
                                                            design.getInputMaster(),
                                                            design.getTerminatorMaster(),
                                                            design.getUnconnectedMaster(),
                                                            design.getVisMaster());
    }catch(const std::exception &e){
        failed = true;
        err = e;
    }

    //Delete the arcs
    //TODO: Remove this?  May not be needed for most applications.  We are probably going to destroy the design anyway
    std::vector<std::shared_ptr<Node>> emptyNodeVec;
    std::vector<std::shared_ptr<Arc>> emptyArcVec;
    design.addRemoveNodesAndArcs(emptyNodeVec, emptyNodeVec, emptyArcVec, arcsToDelete);

    if(failed) {
        std::cerr << "Error when scheduling partition " << partitionNum << std::endl;
        std::cerr << "Emitting: " + dir + "/" + designName + "_sort_error_scheduleGraph.graphml" << std::endl;
        GraphMLExporter::exportGraphML(dir + "/" + designName + "_sort_error_scheduleGraph.graphml", design);

        throw err;
    }

    return sortedNodes;
}
