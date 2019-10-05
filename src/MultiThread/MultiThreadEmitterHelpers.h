//
// Created by Christopher Yarp on 9/3/19.
//

#ifndef VITIS_MULTITHREADEMITTERHELPERS_H
#define VITIS_MULTITHREADEMITTERHELPERS_H

#include <vector>
#include <memory>
#include "GraphCore/NumericValue.h"
#include "GraphCore/SchedParams.h"
#include "GraphCore/Variable.h"
#include <set>
#include <map>
#include <vector>

//Forward Declare
class Node;
class Arc;
class ThreadCrossingFIFO;
class MasterOutput;

/**
 * \addtogroup MultiThread Multi-Thread Support
 * @{
 */

#define IO_PARTITION_NUM -2

/**
 * @brief Contains helper methods for Multi-Threaded Emitters
 */
class MultiThreadEmitterHelpers {
public:
    /**
     * @brief Finds the input and output FIFOs for a partition given a partition crossing to FIFO map
     * @param fifoMap
     * @param partitionNum
     * @param inputFIFOs
     * @param outputFIFOs
     */
    static void findPartitionInputAndOutputFIFOs(std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifoMap, int partitionNum, std::vector<std::shared_ptr<ThreadCrossingFIFO>> &inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> &outputFIFOs);

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

};

/*! @} */

#endif //VITIS_MULTITHREADEMITTERHELPERS_H
