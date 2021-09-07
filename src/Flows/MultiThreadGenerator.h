//
// Created by Christopher Yarp on 9/3/21.
//

#ifndef VITIS_MULTITHREADGENERATOR_H
#define VITIS_MULTITHREADGENERATOR_H

#include "GraphCore/Design.h"
#include "General/TopologicalSortParameters.h"

/**
 * \addtogroup Flows Compiler Flows
 * @brief Compiler flows for translating a design to the desired output
 * @{
 */

/**
 * @brief Flow for generating a multi-threaded C implementation of a given design
 */
namespace MultiThreadGenerator {
    /**
     * @brief Emits the design as a series of C functions.  Each (clock domain in) each partition is emitted as a C
     * function.  A function is created for each thread which includes the intra-thread scheduler.  This scheduler is
     * responsible for calling the partition functions as appropriate.  The scheduler also has access to the input and
     * output FIFOs for the thread and checks the inputs for the empty state and the outputs for the full state to decide
     * if functions should be executed.  A setup function will also be created which allocates the inter-thread
     * FIFOs, creates the threads, binds them to cores, and them starts execution.  Seperate function will be created
     * that enqueue data onto the input FIFOs (when not full) and dequeue data off the output FIFOs (when not empty).
     * Blocking and non-blocking variants of these functions will be created.  Benchmarking code should call the setup
     * function then interact with the input/output FIFO functions.  This will require the use of a core.
     *
     * @note (For framework devloper) The conditional statement about whether or not a context excutes needs to be made in each context it exists
     * in.
     *
     * @note Partitioning should have already occurred before calling this function.
     *
     * @note Design expansion and validation should be run before calling this function
     *
     * @note To avoid dead code being emitted, prune the design before calling this function
     *
     * @param path path to where the output files will be generated
     * @param fileName name of the output files (.h and a .c file will be created)
     * @param designName The name of the design (used as the function name)
     * @param schedType Schedule type
     * @param fifoLength the length of the FIFOs in blocks
     * @param blockSize the block size
     * @param propagatePartitionsFromSubsystems if true, propagates partition information from subsystems to children (from VITIS_PARTITION directives for example)
     * @param partitionMap a vector indicating the mapping of partitions to logical CPUs.  The first element is the I/O thread.  The subsequent entries are for partitions 0, 1, 2, .... If an empty array, I/O thread is placed on CPU0 and the other partitions are placed on the CPU that equals their partition number (ex. partition 1 is placed on CPU1)
     * @param threadDebugPrint if true, inserts print statements into the generated code which indicate the progress of the different threads as they execute
     * @param ioFifoSize the I/O FIFO size in blocks to allocate (only used for shared memory FIFO I/O)
     * @param printTelem if true, telemetry is printed
     * @param telemDumpPrefix if not empty, specifies a file prefix into which telemetry from each compute thread is dumped
     * @param telemLevel the level of telemetry collected
     * @param telemCheckBlockFreq the number of blocks between checking if the telemetry duration has elapsed.  Used to reduce how often the timer is checked
     * @param telemReportPeriodSec the (inexact) period in seconds between telemetry being reported.  Inexact because how often the reporting timer is checked is governed by telemCheckBlockFreq
     * @param memAlignment the aligment (in bytes) used for FIFO buffer allocation
     * @param useSCHEDFIFO if true, pthreads are created which will request to be run with the max RT priority under the linux SCHED_FIFO scheduler
     * @param fifoIndexCachingBehavior indicates when FIFOs check the head/tail pointers and when they rely on a priori information first
     * @param fifoDoubleBuffer indicates what FIFO double buffering behavior to use
     * @param pipeNameSuffix defines as a suffix to be appended to the names of POSIX Pipes or Shared Memory streams
     */
    void emitMultiThreadedC(Design &design, std::string path, std::string fileName, std::string designName,
                            SchedParams::SchedType schedType, TopologicalSortParameters schedParams,
                            ThreadCrossingFIFOParameters::ThreadCrossingFIFOType fifoType,
                            bool emitGraphMLSched, bool printSched, int fifoLength, unsigned long blockSize,
                            bool propagatePartitionsFromSubsystems, std::vector<int> partitionMap,
                            bool threadDebugPrint, int ioFifoSize, bool printTelem, std::string telemDumpPrefix,
                            EmitterHelpers::TelemetryLevel telemLevel, int telemCheckBlockFreq, double telemReportPeriodSec,
                            unsigned long memAlignment, bool useSCHEDFIFO,
                            PartitionParams::FIFOIndexCachingBehavior fifoIndexCachingBehavior,
                            MultiThreadEmit::ComputeIODoubleBufferType fifoDoubleBuffer,
                            std::string pipeNameSuffix);

};

/*! @} */

#endif //VITIS_MULTITHREADGENERATOR_H
