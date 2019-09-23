//
// Created by Christopher Yarp on 9/3/19.
//

#ifndef VITIS_EMITTERHELPERS_H
#define VITIS_EMITTERHELPERS_H

#include <vector>
#include <memory>
#include <GraphCore/Node.h>
#include <GraphCore/Arc.h>
#include <MultiThread/ThreadCrossingFIFO.h>
#include <General/ErrorHelpers.h>
#include "GeneralHelper.h"
#include <GraphCore/EnableOutput.h>
#include <PrimitiveNodes/BlackBox.h>

/**
 * \addtogroup General General Helper Classes
 *
 * @brief A set of general helper classes.
 * @{
*/

/**
 * @brief Contains helper methods for Emitters
 */
class EmitterHelpers {
public:

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
     * The FIFO is placed in the context of the source unless the source is an EnableOutput in which case it is placed outside (1 context up)
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
    static std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>>
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
                if(srcAsEnableOutput){
                    if(!fifoContext.empty()){//May be empty if contexts have not yet been discovered
                        fifoContext.pop_back(); //FIFO should be outside of EnabledSubsystem context of the EnableOutput which is driving it
                    }

                    fifoParent = srcPort->getParent()->getParent()->getParent(); //Get the parent another level up
                }else{
                    fifoParent = srcPort->getParent()->getParent();
                }

                std::shared_ptr<ThreadCrossingFIFO> fifo = NodeFactory::createNode<T>(fifoParent); //Create a FIFO of the specified type
                fifo->setName("PartitionCrossingFIFO_" + GeneralHelper::to_string(partitionPair->first.first) + "->" + GeneralHelper::to_string(partitionPair->first.second) + "_" + GeneralHelper::to_string(groupInd));
                fifo->setPartitionNum(fifoPartition);
                fifo->setContext(fifoContext);
                new_nodes.push_back(fifo);

                //Add to the lowest level context
                if(!fifoContext.empty()){
                    Context specificContext = fifoContext[fifoContext.size()-1];
                    specificContext.getContextRoot()->addSubContextNode(specificContext.getSubContext(), fifo);
                }

                //Create an arc from the src to the FIFO

                std::shared_ptr<Arc> srcArc = Arc::connectNodes(srcPort, fifo->getInputPortCreateIfNot(0), partitionPairGroups[groupInd][0]->getDataType(), partitionPairGroups[groupInd][0]->getSampleTime());
                new_arcs.push_back(srcArc);

                //Re-wire the arcs in the group to the output of the FIFO
                for(int arcInd = 0; arcInd < partitionPairGroups[groupInd].size(); arcInd++){
                    //TODO: Remove Check
                    if(partitionPairGroups[groupInd][arcInd]->getSrcPort() != srcPort){
                        throw std::runtime_error(ErrorHelpers::genErrorStr("Error when inserting FIFOs. Found the arc of an arc in an arc group which was different from another arc in the group"));
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
    static void absorbAdjacentDelaysIntoFIFOs(std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifos,
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
    static AbsorptionStatus absorbAdjacentInputDelayIfPossible(std::shared_ptr<ThreadCrossingFIFO> fifo,
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
    static AbsorptionStatus absorbAdjacentOutputDelayIfPossible(std::shared_ptr<ThreadCrossingFIFO> fifo,
                                                        int outputPartition,
                                                        std::vector<std::shared_ptr<Node>> &new_nodes,
                                                        std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                        std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                        std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                        bool printActions = true);

    /**
     * @brief Finds the input and output FIFOs for a partition given a partition crossing to FIFO map
     * @param fifoMap
     * @param partitionNum
     * @param inputFIFOs
     * @param outputFIFOs
     */
    static void findPartitionInputAndOutputFIFOs(std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifoMap, int partitionNum, std::vector<std::shared_ptr<ThreadCrossingFIFO>> &inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> &outputFIFOs);

    static std::vector<std::shared_ptr<Node>> findNodesWithState(std::vector<std::shared_ptr<Node>> &nodesToSearch);

    static std::vector<std::shared_ptr<Node>> findNodesWithGlobalDecl(std::vector<std::shared_ptr<Node>> &nodesToSearch);

    static std::vector<std::shared_ptr<ContextRoot>> findContextRoots(std::vector<std::shared_ptr<Node>> &nodesToSearch);

    static std::vector<std::shared_ptr<BlackBox>> findBlackBoxes(std::vector<std::shared_ptr<Node>> &nodesToSearch);

    /**
     * @brief Emits cStatements copying from the thread argument structure to local variables
     * @param inputFIFOs
     * @param outputFIFOs
     * @param structName
     * @return
     */
    static std::string emitCopyCThreadArgs(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, std::string structName, std::string structTypeName);

    /**
     * @brief Emits C code to check the state of FIFOs
     *
     * @param fifos
     * @param partition
     * @param checkFull if true, checks if the FIFO is full, if false, checks if the FIFO is empty
     * @param checkVarName the name of the variable used to identify if the FIFO check succedded or not.  This should be unique
     * @param shortCircuit if true, as soon as a FIFO is not ready the check loop repeats.  If false,
     * @return
     */
    static std::string emitFIFOChecks(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, int partition, bool checkFull, std::string checkVarName, bool shortCircuit);

    static std::vector<std::string> createFIFOReadTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos);

    static std::vector<std::string> createFIFOWriteTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos);

    static std::vector<std::string> readFIFOsToTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos);

    static std::vector<std::string> writeFIFOsFromTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos);
};

/*! @} */

#endif //VITIS_EMITTERHELPERS_H
