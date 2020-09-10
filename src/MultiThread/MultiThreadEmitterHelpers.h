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

#define VITIS_PLATFORM_PARAMS_NAME "vitisPlatformParams"
#define VITIS_NUMA_ALLOC_HELPERS "vitisNumaAllocHelpers"

//Forward Declare
class Node;
class Arc;
class ThreadCrossingFIFO;
class MasterOutput;
class ClockDomain;

/**
 * \addtogroup MultiThread Multi-Thread Support
 * @{
 */

#define IO_PARTITION_NUM -2
#define BLOCK_IND_VAR_PREFIX "blkInd"

/**
 * @brief Contains helper methods for Multi-Threaded Emitters
 */
namespace MultiThreadEmitterHelpers {
    /**
     * @brief Finds the input and output FIFOs for a partition given a partition crossing to FIFO map
     * @param fifoMap
     * @param partitionNum
     * @param inputFIFOs
     * @param outputFIFOs
     */
    void findPartitionInputAndOutputFIFOs(std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifoMap, int partitionNum, std::vector<std::shared_ptr<ThreadCrossingFIFO>> &inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> &outputFIFOs);

    /**
     * @brief Emits cStatements copying from the thread argument structure to local variables
     * @param inputFIFOs
     * @param outputFIFOs
     * @param structName
     * @return
     */
    std::string emitCopyCThreadArgs(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, std::string structName, std::string structTypeName);

    /**
     * @brief Emits C code to check the state of FIFOs
     *
     * @param fifos
     * @param producer if true, checks if the FIFO is full, if false, checks if the FIFO is empty
     * @param checkVarName the name of the variable used to identify if the FIFO check succeded or not.  This should be unique
     * @param shortCircuit if true, as soon as a FIFO is not ready the check loop repeats.  If false, all FIFOs are checked
     * @param blocking if true, the FIFO check will repeat until all are ready.  If false, the FIFO check will not block the execution of the code proceeding it
     * @param includeThreadCancelCheck if true, includes a call to pthread_testcancel durring the FIFO check (to determine if the thread should exit)
     * @return
     */
    std::string emitFIFOChecks(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, bool producer, std::string checkVarName, bool shortCircuit, bool blocking, bool includeThreadCancelCheck);

    std::vector<std::string> createAndInitFIFOLocalVars(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos);

    std::vector<std::string> createFIFOReadTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos);

    std::vector<std::string> createFIFOWriteTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos);

    //Outer vector is for each fifo, the inner vector is for each port within the specified FIFO
    std::vector<std::string> createAndInitializeFIFOWriteTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, std::vector<std::vector<NumericValue>> defaultVal);

    std::vector<std::string> readFIFOsToTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, bool forcePull = false, bool pushAfter = true);

    std::vector<std::string> writeFIFOsFromTemps(std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos, bool forcePull = false, bool updateAfter = true);

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
    std::pair<std::string, std::string> getCThreadArgStructDefn(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, std::string designName, int partitionNum);

    /**
     * @brief Emits a header file with the structure definitions used by the thread crossing FIFOs
     * @param path
     * @param filenamePrefix
     * @param fifos
     * @returns the filename of the header file
     */
    std::string emitFIFOStructHeader(std::string path, std::string fileNamePrefix, std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos);

    /**
     * @brief Get the core number for the specified partition number
     * @param parititon
     * @param partitionMap
     * @return
     */
    int getCore(int parititon, const std::vector<int> &partitionMap, bool print = false);

    /**
     * @brief Emits the benchmark kernel function for multi-threaded emit.  This includes allocating FIFOs and creating/starting threads.
     * @param fifoMap
     * @param ioBenchmarkSuffix io_constant for constant benchmark
     * @param papiHelperHeader if not empty, initializes PAPI in the kernel function
     */
    void emitMultiThreadedBenchmarkKernel(std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifoMap,
                                          std::map<int, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> inputFIFOMap,
                                          std::map<int, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> outputFIFOMap,
                                          std::set<int> partitions, std::string path, std::string fileNamePrefix,
                                          std::string designName, std::string fifoHeaderFile, std::string ioBenchmarkSuffix,
                                          std::vector<int> partitionMap, std::string papiHelperHeader);

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
    void emitMultiThreadedDriver(std::string path, std::string fileNamePrefix, std::string designName, std::string ioBenchmarkSuffix, std::vector<Variable> inputVars);

    /**
     * @brief Emits a makefile for benchmarking multithreaded emits
     *
     * @note This can be used with different types of I/O handlers
     * @param path
     * @param fileNamePrefix
     * @param designName
     * @param ioBenchmarkSuffix
     * @param includeLrt if true, includes -lrt to the linker options
     * @param includePAPI if true, includes -lpapi to the linker options
     */
    void emitMultiThreadedMakefile(std::string path, std::string fileNamePrefix, std::string designName, std::set<int> partitions, std::string ioBenchmarkSuffix, bool includeLrt, std::vector<std::string> additionalSystemSrc, bool includePAPI);

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
     * @param blockSize the size of the block (in samples) that is processed in each call to the emitted C function.  This is the size for the base rate.
     * @param fifoHeaderFile the filename of the FIFO Header which defines FIFO structures (if needed)
     * @param threadDebugPrint if true, inserts print statements into the thread function to report when it reaches various points in the execution loop
     * @param printTelem if true, prints telemetry on the thread's execution
     * @param telemDumpFilePrefix if not empty, specifies a file into which telemetry from the compute thread is dumped
     * @param telemAvg if true, the telemetry is averaged over the entire run.  If false, the telemetry is only an average of the measurement period
     * @param papiHelperHeader if not empty, collects performance counter information from the PAPI library.  Note that this will have an adverse effect on performance.  printTelem || !telemDumpFilePrefix.empty() must be true for this to be collected
     */
    void emitPartitionThreadC(int partitionNum, std::vector<std::shared_ptr<Node>> nodesToEmit,
                              std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs,
                              std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, std::string path,
                              std::string fileNamePrefix, std::string designName, SchedParams::SchedType schedType,
                              std::shared_ptr<MasterOutput> outputMaster, unsigned long blockSize,
                              std::string fifoHeaderFile, bool threadDebugPrint, bool printTelem,
                              std::string telemDumpFilePrefix, bool telemAvg, std::string papiHelperHeader);

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
    std::string getPartitionComputeCFunctionArgPrototype(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, int blockSize);

    /**
     * @brief Gets the statement calling the partition compute function
     * @param computeFctnName
     * @param inputFIFOs
     * @param outputFIFOs
     * @return
     */
    std::string getCallPartitionComputeCFunction(std::string computeFctnName, std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, int blockSize);

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
    void emitSelectOpsSchedStateUpdateContext(std::ofstream &cFile, std::vector<std::shared_ptr<Node>> &nodesToEmit, SchedParams::SchedType schedType, std::shared_ptr<MasterOutput> outputMaster, int blockSize = 1, std::string indVarName = "");

    //Checks that the only nodes that are in the I/O partition are ThreadCrossingFIFOs or subsystems
    bool checkNoNodesInIO(std::vector<std::shared_ptr<Node>> nodes);

    void writeTelemConfigJSONFile(std::string path, std::string telemDumpPrefix, std::string designName, std::map<int, int> partitionToCPU, int ioPartitionNumber, std::string graphmlSchedFile);

    /**
     * @brief Writes a file that contains configuration info for the target platform including cache line size
     * @param path
     * @param filename
     */
    void writePlatformParameters(std::string path, std::string filename, int memAlignment);

    void writeNUMAAllocHelperFiles(std::string path, std::string filename);

    /**
     * @brief Get the index variable name for a particular clock domain rate relative to the base rate
     * @param clkDomainRate
     * @param counter if true, this is the counter variable used for tracking when to increment the associated index.  If false, it is the index
     * @return
     */
    std::string getClkDomainIndVarName(std::pair<int, int> clkDomainRate, bool counter);

    /**
     * @brief Get the index variable name for a particular clock domain
     * @param clkDomainRate
     * @return
     */
    std::string getClkDomainIndVarName(std::shared_ptr<ClockDomain> clkDomain, bool counter);

};

/*! @} */

#endif //VITIS_MULTITHREADEMITTERHELPERS_H
