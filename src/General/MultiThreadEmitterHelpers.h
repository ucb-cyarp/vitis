//
// Created by Christopher Yarp on 9/3/19.
//

#ifndef VITIS_MULTITHREADEMITTERHELPERS_H
#define VITIS_MULTITHREADEMITTERHELPERS_H

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

#define IO_PARTITION_NUM -2

/**
 * @brief Contains helper methods for Emitters
 */
class MultiThreadEmitterHelpers {
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
    static void reshapeFIFOInitialConditionsForBlockSize(std::shared_ptr<ThreadCrossingFIFO> fifo,
                                                                std::vector<std::shared_ptr<Node>> &new_nodes,
                                                                std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                                std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                                std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                                bool printActions = true);

    static void reshapeFIFOInitialConditionsToSize(std::shared_ptr<ThreadCrossingFIFO> fifo,
                                                         int targetSize,
                                                         std::vector<std::shared_ptr<Node>> &new_nodes,
                                                         std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                         std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                         std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                         bool printActions = true);

    static void reshapeFIFOInitialConditions(std::shared_ptr<ThreadCrossingFIFO> fifo,
                                             int numElementsToMove,
                                             std::vector<std::shared_ptr<Node>> &new_nodes,
                                             std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                             std::vector<std::shared_ptr<Arc>> &new_arcs,
                                             std::vector<std::shared_ptr<Arc>> &deleted_arcs);

    /**
     * @brief Merge FIFOs between a pair of partitions into a single FIFO
     * @param fifos
     * @param new_nodes
     * @param deleted_nodes
     * @param new_arcs
     * @param deleted_arcs
     * @param printActions
     * @return
     */
    static std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>>
            mergeFIFOs(std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifos,
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
     * @param checkFull if true, checks if the FIFO is full, if false, checks if the FIFO is empty
     * @param checkVarName the name of the variable used to identify if the FIFO check succeded or not.  This should be unique
     * @param shortCircuit if true, as soon as a FIFO is not ready the check loop repeats.  If false, all FIFOs are checked
     * @param blocking if true, the FIFO check will repeat until all are ready.  If false, the FIFO check will not block the execution of the code proceeding it
     * @param includeThreadCancelCheck if true, includes a call to pthread_testcancel durring the FIFO check (to determine if the thread should exit)
     * @return
     */
    static std::string emitFIFOChecks(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, bool checkFull, std::string checkVarName, bool shortCircuit, bool blocking, bool includeThreadCancelCheck);

    static std::vector<std::string> createFIFOReadTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos);

    static std::vector<std::string> createFIFOWriteTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos);

    //Outer vector is for each fifo, the inner vector is for each port within the specified FIFO
    static std::vector<std::string> createAndInitializeFIFOWriteTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, std::vector<std::vector<NumericValue>> defaultVal);

    static std::vector<std::string> readFIFOsToTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos);

    static std::vector<std::string> writeFIFOsFromTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos);

    /**
     * @brief Propgagates partition to nodes and propagates down.  Subsystems can specify a partition for their decendents
     *
     * To specify a partition, the partition of the subsystem cannot be -1;
     *
     * @param nodes
     * @param partition
     */
    static void propagatePartitionsFromSubsystemsToChildren(std::set<std::shared_ptr<Node>>& nodes, int partition);

    /**
     * @brief Get the structure definition for a particular partition's thread
     *
     * It includes the shared pointers for each FIFO into and out of the partition
     *
     * The structure definition takes the form of:
     *
     * typedef struct {
     *     type1 var1;
     *     type2 var2;
     *     ...
     * } designName_partition#_threadArgs_t
     *
     * @warning these pointers should likely be copied in the thread function with the proper volatile property
     *
     * @warning the variables for the FIFOs are expected to have unique names
     *
     * @return a pair of stringsn (C structure definition, structure type name)
     */
    static std::pair<std::string, std::string> getCThreadArgStructDefn(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, std::string designName, int partitionNum);

    /**
     * @brief Emits a header file with the structure definitions used by the thread crossing FIFOs
     * @param path
     * @param filenamePrefix
     * @param fifos
     * @returns the filename of the header file
     */
    static std::string emitFIFOStructHeader(std::string path, std::string fileNamePrefix, std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos);

    /**
     * @brief Emits the benchmark kernel function for multi-threaded emit.  This includes allocating FIFOs and creating/starting threads.
     * @param fifoMap
     * @param ioBenchmarkSuffix io_constant for constant benchmark
     */
    static void emitMultiThreadedBenchmarkKernel(std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifoMap, std::map<int, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> inputFIFOMap, std::map<int, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> outputFIFOMap, std::set<int> partitions, std::string path, std::string fileNamePrefix, std::string designName, std::string fifoHeaderFile, std::string ioBenchmarkSuffix, std::vector<int> partitionMap);

    //The following 2 functions can be reused for different I/O drivers

    /**
     * @brief Emits the benchmarking driver function for multithreaded emits.
     *
     * @note This can be used with different types of I/O handlers
     *
     * @param path
     * @param fileNamePrefix
     * @param designName
     * @param blockSize
     * @param ioBenchmarkSuffix
     * @param inputVars
     */
    static void emitMultiThreadedDriver(std::string path, std::string fileNamePrefix, std::string designName, int blockSize, std::string ioBenchmarkSuffix, std::vector<Variable> inputVars);

    /**
     * @brief Emits a makefile for benchmarking multithreaded emits
     *
     * @note This can be used with different types of I/O handlers
     * @param path
     * @param fileNamePrefix
     * @param designName
     * @param blockSize
     * @param ioBenchmarkSuffix
     */
    static void emitMultiThreadedMakefile(std::string path, std::string fileNamePrefix, std::string designName, int blockSize, std::set<int> partitions, std::string ioBenchmarkSuffix);

    /**
     * @brief Emits the C code for a thread for the a given partition (except the I/O thread which is handled seperatly)
     *
     * This includes the thread function itself as well as the core scheduler which checks FIFO status and calls the
     * thread function.
     *
     * @note Design expansion and validation should be run before calling this function
     *
     * @note To avoid dead code being emitted, prune the design before calling this function
     *
     * @param partitionNum the partition number being emitted
     * @param nodesToEmit a vector of nodes in the partition to be emitted
     * @param path path to where the output files will be generated
     * @param filenamePrefix prefix of the output files (.h and a .c file will be created).  _partition# will be appended to the filename
     * @param designName The name of the design (used as the function name)
     * @param schedType Schedule type
     * @param outputMaster a pointer to the output master of the design being emitted
     * @param blockSize the size of the block (in samples) that is processed in each call to the emitted C function
     * @param fifoHeaderFile the filename of the FIFO Header which defines FIFO structures (if needed)
     * @param threadDebugPrint if true, inserts print statements into the thread function to report when it reaches various points in the execution loop
     */
    static void emitPartitionThreadC(int partitionNum, std::vector<std::shared_ptr<Node>> nodesToEmit, std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, std::string path, std::string fileNamePrefix, std::string designName, SchedParams::SchedType schedType, std::shared_ptr<MasterOutput> outputMaster, unsigned long blockSize, std::string fifoHeaderFile, bool threadDebugPrint);

    /**
     * @brief Get the argument portion of the C function prototype for the partition compute function
     *
     * For partition compute functions, there is no returned output. Outputs are passed as references (scalar) or pointers (arrays) to the function.
     *
     * The inputs come from input FIFOs to the partition and outputs come from ouputFIFOs
     *
     * An example function prototype would be designName(double In_0_re, double In_0_im, bool &out_1)
     *
     * This function returns "double In_0_re, double In_0_im, bool &out_1"
     *
     * Complex types are split into 2 arguments, each is prepended with _re or _im for real and imagionary component respectivly.
     *
     * The DataType is converted to the smallest standard CPU type that can contain the type
     *
     * @warning Assumes the design has already been validated (ie. has at least one arc per port).
     *
     * @return argument portion of the C function prototype for this partition's compute function
     */
    static std::string getPartitionComputeCFunctionArgPrototype(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, int blockSize);

    /**
     * @brief Gets the statement calling the partition compute function
     * @param computeFctnName
     * @param inputFIFOs
     * @param outputFIFOs
     * @return
     */
    static std::string getCallPartitionComputeCFunction(std::string computeFctnName, std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, int blockSize);

    /**
     * @brief Emits a given set operators using the schedule emitter.  This emitter is context aware and supports emitting scheduled state updates
     *
     * @warning Unlike emitSingleThreadedOpsSchedStateUpdateContext, this function does not include the output node automatically.  If you would
     * like to schedule
     *
     * @param cFile the cFile to emit to
     * @param blockSize the size of the block (in samples) that are processed in each call to the function
     * @param indVarName the variable that specifies the index in the block that is being computed
     */
    static void emitSelectOpsSchedStateUpdateContext(std::ofstream &cFile, std::vector<std::shared_ptr<Node>> &nodesToEmit, SchedParams::SchedType schedType, std::shared_ptr<MasterOutput> outputMaster, int blockSize = 1, std::string indVarName = "");

    /**
     * @brief Emits operators for the given nodes (in the order given).  Does not emit function prototypes.  This emitter is context aware
     * @param cFile the cFile to emit to
     * @param schedType the scheduler being used.  Is passed to downstream context emit functions
     * @param orderedNodes the nodes to emit, given in the order they should be emitted
     * @param outputMaster a pointer to the output master of the design being emitted
     * @param blockSize the size of the block (in samples) that are processed in each call to the function
     * @param indVarName the variable that specifies the index in the block that is being computed
     * @param checkForPartitionChange if true, checks if the partition changes while emitting and throws an error if it does
     */
    static void emitOpsStateUpdateContext(std::ofstream &cFile, SchedParams::SchedType schedType, std::vector<std::shared_ptr<Node>> orderedNodes, std::shared_ptr<MasterOutput> outputMaster, int blockSize = 1, std::string indVarName = "", bool checkForPartitionChange = true);


};

/*! @} */

#endif //VITIS_MULTITHREADEMITTERHELPERS_H
