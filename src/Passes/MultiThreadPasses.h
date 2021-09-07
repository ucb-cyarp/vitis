//
// Created by Christopher Yarp on 10/4/19.
//

#ifndef VITIS_MULTITHREADPASSES_H
#define VITIS_MULTITHREADPASSES_H

#include <map>
#include <vector>
#include <string>
#include "GraphCore/Node.h"
#include "GraphCore/Arc.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/ContextFamilyContainer.h"
#include "GraphCore/ContextContainer.h"
#include "MultiThread/ThreadCrossingFIFO.h"
#include "General/GeneralHelper.h"
#include "General/ErrorHelpers.h"
#include "MultiRate/RateChange.h"
#include "GraphCore/Design.h"
#include "PrimitiveNodes/Mux.h"

/**
 * \addtogroup Passes Design Passes/Transforms
 *
 * @brief A group of compiler passes which operate over a design.
 * @{
 */

/**
 * @brief A helper class containing methods for transforming a design for multi-thread emit
 */
namespace MultiThreadPasses {
    /**
     * @brief Class used to report if a delay was fully absorbed, partially absorbed, or not absorbed
     */
    enum class AbsorptionStatus{
        NO_ABSORPTION,
        FULL_ABSORPTION,
        PARTIAL_ABSORPTION_FULL_FIFO,
        PARTIAL_ABSORPTION_MERGE_INIT_COND
    };

    /**
     * @brief Inserts partition (thread) crossing FIFOs into the design at the specified points.
     *
     * One FIFO is inserted for each group of arcs passed to the function.  The FIFO is placed in the partition
     * of the src node for these arcs.  This is due to the scheduling behavior
     *
     * The FIFO is placed in the context of the source unless the source is an EnableOutput or RateChange output in which case it is placed outside (1 context up)
     *
     * The FIFO shares the same parent as its source unless it is an EnableOutput in which case its parent is another level up
     *
     * @param arcGroups a map of partition pairs to groups of vectors crossing that particular partition boundary
     * @param new_nodes a vector of nodes added to the design
     * @param deleted_nodes a vector of nodes to be deleted in the design
     * @param new_arcs a vector of new arcs added to the design
     * @param deleted_arcs a vector of arcs to be deleted from the design
     * @return The same map as provided in the arcGroups arguments excpet the groups have been replaced with the FIFO inserted for the grouo
     */
    template <typename T> //The FIFO type
    std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>>
    insertPartitionCrossingFIFOs(std::map<std::pair<int, int>, std::vector<std::vector<std::shared_ptr<Arc>>>> arcGroups,
    std::vector<std::shared_ptr<Node>> &new_nodes,
            std::vector<std::shared_ptr<Node>> &deleted_nodes,
    std::vector<std::shared_ptr<Arc>> &new_arcs,
            std::vector<std::shared_ptr<Arc>> &deleted_arcs){

        std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifoMap;
        //Note: each arc group should only be from a single src

        for(auto partitionPair = arcGroups.begin(); partitionPair != arcGroups.end(); partitionPair++){
            std::vector<std::shared_ptr<ThreadCrossingFIFO>> crossingFIFOs;

            std::vector<std::vector<std::shared_ptr<Arc>>> partitionPairGroups = partitionPair->second;

            for(int groupInd = 0; groupInd < partitionPairGroups.size(); groupInd++){
                //TODO: Remove Check
                if(partitionPairGroups[groupInd].empty()){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Error when inserting FIFOs. Found a partition crossing group with no arcs."));
                }

                std::shared_ptr<OutputPort> srcPort = partitionPairGroups[groupInd][0]->getSrcPort();

                //TODO: Remove Check
                if(srcPort->getParent()->getPartitionNum() != partitionPair->first.first){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Error when inserting FIFOs. Found the source of an arc group which disagrees with the partition pair given"));
                }

                //Create a FIFO for each group
                int fifoPartition = srcPort->getParent()->getPartitionNum();
                std::vector<Context> fifoContext = srcPort->getParent()->getContext();
                std::shared_ptr<SubSystem> fifoParent;

                std::shared_ptr<EnableOutput> srcAsEnableOutput = GeneralHelper::isType<Node, EnableOutput>(srcPort->getParent());
                std::shared_ptr<Mux> srcAsMux = GeneralHelper::isType<Node, Mux>(srcPort->getParent());
                std::shared_ptr<ContextRoot> srcAsContextRoot = GeneralHelper::isType<Node, ContextRoot>(srcPort->getParent());
                std::shared_ptr<RateChange> srcAsRateChange = GeneralHelper::isType<Node, RateChange>(srcPort->getParent());
                bool rateChangeOutput = false;
                if(srcAsRateChange){
                    rateChangeOutput = !srcAsRateChange->isInput();
                }

                //Note, the first getParent() gets the src node
                std::shared_ptr<SubSystem> srcParent = srcPort->getParent()->getParent();

                //TODO: This relies on EnableOutputs from being directly under the ContextContainers or EnableSubsystems.  Re-evaluate if susbsystem re-assembly attempts are made in the future
                //TODO: Come up with a more general way of performing this
                if(srcAsEnableOutput != nullptr || srcAsContextRoot != nullptr || srcAsMux != nullptr || rateChangeOutput) {
                    if (!fifoContext.empty() && srcAsContextRoot == nullptr) {//May be empty if contexts have not yet been discovered
                        fifoContext.pop_back(); //FIFO should be outside of EnabledSubsystem context of the EnableOutput which is driving it (if src is enabled output).

                        // Should also be outside of the context of a context root which is driving it (ex. outside of a Mux context if a mux is driving it)
                        // However, mote that context roots do not contain their own context in their context stack but are moved under (one of) their ContextFamilyContainer
                        // during encapsulation
                    }

                    //Check for encapsulation
                    if (GeneralHelper::isType<Node, ContextContainer>(srcParent) != nullptr) {
                        //Encapsulation has already occured.  Need to go 2 levels up
                        if (srcAsContextRoot == nullptr) {
                            //If the src is not a context root, the node is one of the ContextContainers and going up 2 levels is nessiary
                            fifoParent = srcParent->getParent()->getParent();
                        } else {
                            throw std::runtime_error(ErrorHelpers::genErrorStr("Discovered src encapsulated in ContextContainer during FIFO insertion but expected src not to be ContextRoot"));
                        }
                    } else if(GeneralHelper::isType<Node, ContextFamilyContainer>(srcParent) != nullptr){
                        if(srcAsContextRoot != nullptr){
                            //If the src is a context root, the node is just inside the ContextFamilyContainer and we only need to go up one level
                            //This is the case with mux
                            fifoParent = srcParent->getParent();
                        }else{
                            throw std::runtime_error(ErrorHelpers::genErrorStr("Discovered src encapsulated in ContextFamilyContainer during FIFO insertion but expected src to be ContextRoot"));
                        }
                    } else {
                        //Encapsulation has not occured yet, only need 1 level up
                        //Only need to go up if the src is not a ContextRoot
                        //ie. need to go up a level if the node is a EnableOutput or rateChangeOutput since taking the parent of these nodes would
                        //place the FIFO inside the EnabledSubsystem or ClockDomain context
                        if (srcAsContextRoot == nullptr) {
                            //Not a context root, it must be either an EnableOutput or a RateChange ouput
                            fifoParent = srcParent->getParent();
                        } else {
                            fifoParent = srcParent;
                        }
                    }
                }else{
                    //Get the parent of the src node
                    fifoParent = srcParent;
                }

                std::shared_ptr<ThreadCrossingFIFO> fifo = NodeFactory::createNode<T>(fifoParent); //Create a FIFO of the specified type
                std::string srcPartitionName = partitionPair->first.first >= 0 ? GeneralHelper::to_string(partitionPair->first.first) : "N"+GeneralHelper::to_string(-partitionPair->first.first);
                std::string dstPartitionName = partitionPair->first.second >= 0 ? GeneralHelper::to_string(partitionPair->first.second) : "N"+GeneralHelper::to_string(-partitionPair->first.second);
                fifo->setName("PartitionCrossingFIFO_" + srcPartitionName + "_TO_" + dstPartitionName + "_" + GeneralHelper::to_string(groupInd));
                fifo->setPartitionNum(fifoPartition);
                fifo->setContext(fifoContext);
                new_nodes.push_back(fifo);

                //Add to the lowest level context
                if(!fifoContext.empty()){
                    Context specificContext = fifoContext[fifoContext.size()-1];
                    specificContext.getContextRoot()->addSubContextNode(specificContext.getSubContext(), fifo);
                }

                //Create an arc from the src to the FIFO

                DataType groupDataType = partitionPairGroups[groupInd][0]->getDataType();
                std::shared_ptr<Arc> srcArc = Arc::connectNodes(srcPort, fifo->getInputPortCreateIfNot(0), groupDataType, partitionPairGroups[groupInd][0]->getSampleTime());
                new_arcs.push_back(srcArc);

                //Re-wire the arcs in the group to the output of the FIFO
                for(int arcInd = 0; arcInd < partitionPairGroups[groupInd].size(); arcInd++){
                    //TODO: Remove Check
                    if(partitionPairGroups[groupInd][arcInd]->getSrcPort() != srcPort){
                        std::string origSrcPort = srcPort->getParent()->getFullyQualifiedName() + " " + GeneralHelper::to_string(srcPort->getPortNum());
                        std::string newSrcPort = partitionPairGroups[groupInd][arcInd]->getSrcPort()->getParent()->getFullyQualifiedName() + " " + GeneralHelper::to_string(partitionPairGroups[groupInd][arcInd]->getSrcPort()->getPortNum());
                        throw std::runtime_error(ErrorHelpers::genErrorStr("Error when inserting FIFOs. Found an in a partition crossing arc group which had different source port than another arc in the group"));
                    }

                    //TODO: Remove Check
                    if(partitionPairGroups[groupInd][arcInd]->getDstPort()->getParent()->getPartitionNum() != partitionPair->first.second){
                        throw std::runtime_error(ErrorHelpers::genErrorStr("Error when inserting FIFOs. Found the dst of an arc group which disagrees with the partition pair given"));
                    }

                    partitionPairGroups[groupInd][arcInd]->setSrcPortUpdateNewUpdatePrev(fifo->getOutputPortCreateIfNot(0));
                }

                crossingFIFOs.push_back(fifo);
            }

            fifoMap[partitionPair->first] = crossingFIFOs;
        }

        return fifoMap;
    }

    /**
     * @brief Absorbs delays that are adjacent to FIFOs into the FIFO itself as initial state.
     *
     * Note: Only absorbs on the input if the FIFO is the sole node connected to the output of the delay and the FIFO does not have order constraint input arcs
     * Note: Only absorbs on the output if the delay is the sole node connected to the FIFO output and the FIFO does not have order constraint output arcs
     *
     * Does not absorb delays in other partitions (or contexts)
     *
     * TODO: Consider investigating order constraint requirements
     *
     * Will iterate on a FIFO until no more delays can be absorbed
     *
     * @param fifos a map of partition crossings to FIFOs
     * @param new_nodes a vector of nodes added to the design
     * @param deleted_nodes a vector of nodes to be deleted in the design
     * @param new_arcs a vector of new arcs added to the design
     * @param deleted_arcs a vector of arcs to be deleted from the design
     * @param printActions if true, prints when delay nodes are absorbed.  If false, does not print any status reports
     */
    void absorbAdjacentDelaysIntoFIFOs(std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifos,
    std::vector<std::shared_ptr<Node>> &new_nodes,
            std::vector<std::shared_ptr<Node>> &deleted_nodes,
    std::vector<std::shared_ptr<Arc>> &new_arcs,
            std::vector<std::shared_ptr<Arc>> &deleted_arcs,
    bool printActions = true);

    /**
     * @brief Absorbs a delay at the input of the FIFO if it is allowed
     *
     * Note: Only absorbs on the input if the FIFO is the sole node connected to the output of the delay and the FIFO does not have order constraint input arcs
     *
     * TODO: Consider investigating order constraint requirements
     *
     * @param fifo the FIFO to absorb the delay into
     * @param new_nodes a vector of nodes added to the design
     * @param deleted_nodes a vector of nodes to be deleted in the design
     * @param new_arcs a vector of new arcs added to the design
     * @param deleted_arcs a vector of arcs to be deleted from the design
     * @param printActions if true, prints when delay nodes are absorbed.  If false, does not print any status reports
     * @return if a full, partial, or no absorption of a delay occured
     */
    AbsorptionStatus absorbAdjacentInputDelayIfPossible(std::shared_ptr<ThreadCrossingFIFO> fifo,
                                                               int inputPartition,
                                                               std::vector<std::shared_ptr<Node>> &new_nodes,
                                                               std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                               std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                               std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                               bool printActions = true);

    /**
     * @brief Absorbs a delay at the input of the FIFO if it is allowed
     *
     * Note: Only absorbs on the output if delays are the only outputs of the FIFO, each delay has the same initial conditions, and the FIFO does not have order constraint output arcs
     *
     * TODO: Consider investigating order constraint requirements
     *
     * @param fifo the FIFO to absorb the delay into
     * @param new_nodes a vector of nodes added to the design
     * @param deleted_nodes a vector of nodes to be deleted in the design
     * @param new_arcs a vector of new arcs added to the design
     * @param deleted_arcs a vector of arcs to be deleted from the design
     * @param printActions if true, prints when delay nodes are absorbed.  If false, does not print any status reports
     * @return if a full, partial, or no absorption of a delay occured
     */
    AbsorptionStatus absorbAdjacentOutputDelayIfPossible(std::shared_ptr<ThreadCrossingFIFO> fifo,
                                                                int outputPartition,
                                                                std::vector<std::shared_ptr<Node>> &new_nodes,
                                                                std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                                std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                                std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                                bool printActions = true);

    /**
     * @brief The number of initial conditions (in elements) must be an integer multiple of the block size of the FIFO
     *
     * This function takes the remaining initial conditions and moves them to a delay node at the input of the FIFO
     *
     * @param fifo
     * @param new_nodes
     * @param deleted_nodes
     * @param new_arcs
     * @param deleted_arcs
     * @param printActions
     * @return
     */
    void reshapeFIFOInitialConditionsForBlockSize(std::shared_ptr<ThreadCrossingFIFO> fifo,
                                                         std::vector<std::shared_ptr<Node>> &new_nodes,
                                                         std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                         std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                         std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                         bool printActions = true);

    void reshapeFIFOInitialConditionsToSizeBlocks(std::shared_ptr<ThreadCrossingFIFO> fifo,
                                                   int targetSizeBlocks,
                                                   std::vector<std::shared_ptr<Node>> &new_nodes,
                                                   std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                   std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                   std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                   bool printActions = true);

    /**
     * @brief Reduces the number of initial conditions in a FIFO by pushing some into a delay connected to the FIFO
     * By default, the delay is placed on the input of the FIFO
     * However, if the src port of the FIFO is a MasterInput node, the delay is placed on the output
     *
     * @param fifo
     * @param numElementsToMove These are elements in the FIFO and not in the intial conditions.  They can be scalar, vectors, or matricies
     * @param new_nodes
     * @param deleted_nodes
     * @param new_arcs
     * @param deleted_arcs
     */
    void reshapeFIFOInitialConditions(std::shared_ptr<ThreadCrossingFIFO> fifo,
                                             int numElementsToMove,
                                             std::vector<std::shared_ptr<Node>> &new_nodes,
                                             std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                             std::vector<std::shared_ptr<Arc>> &new_arcs,
                                             std::vector<std::shared_ptr<Arc>> &deleted_arcs);

    /**
     * @brief Propgagates partition to nodes and propagates down.  Subsystems can specify a partition for their decendents
     *
     * To specify a partition, the partition of the subsystem cannot be -1;
     *
     * @param nodes
     * @param partition
     */
    void propagatePartitionsFromSubsystemsToChildren(std::set<std::shared_ptr<Node>>& nodes, int partition);

    /**
     * @brief Merges FIFOs going between a pair of partitions together.  This helps amortize overheads associated with
     *        FIFOs including performing empty/full checks.
     *
     *        Either all of the arcs from one partition to another are merged together (ignoreContexts=true) or
     *        only arcs from one partition to another where the contexts match (ignoreContexts=false).  Which one is
     *        selected depends if on-demand/lazy evaluation of FIFOs in contexts is being used.  If so, FIFOs to/from
     *        contexts need to be treated differently.  However, arcs contained within the contexts can be combined.
     *
     *        @note Clock domains are a special case type of context and are not considered, even when
     *              ignoreContexts=false.  This is because different clock domains can be accommodated by a single FIFO
     *              by adjusting relative block sizes of the ports.
     *
     *        @param fifoMap: A map of FIFOs going between a given pair of partitions (unidirectional).  Will be modified after merging
     *        @param ignoreContexts if true, merge all FIFOs from one partition to another.  If false, only merge FIFOs when their contexts match
     *        @param verbose if true, print information about FIFO merging
     */
    void mergeFIFOs(std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> &fifoMap,
                    std::vector<std::shared_ptr<Node>> &nodesToAdd,
                    std::vector<std::shared_ptr<Node>> &nodesToRemove,
                    std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                    std::vector<std::shared_ptr<Arc>> &arcsToRemove,
                    std::vector<std::shared_ptr<Node>> &addToTopLevel,
                    bool ignoreContexts, bool verbose);

    void propagatePartitionsFromSubsystemsToChildren(Design &design);
};

/*! @} */


#endif //VITIS_MULTITHREADPASSES_H
