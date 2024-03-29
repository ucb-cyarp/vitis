//
// Created by Christopher Yarp on 10/4/19.
//

#ifndef VITIS_STREAMIOTHREAD_H
#define VITIS_STREAMIOTHREAD_H

#include <utility>
#include <string>
#include "ThreadCrossingFIFO.h"

#define DEFAULT_VITIS_SOCKET_LISTEN_PORT 8080
#define DEFAULT_VITIS_SOCKET_LISTEN_ADDR "INADDR_ANY"

/**
 * @brief A class containing code for generating stream I/O drivers
 * This includes a linux pipe version and a network sockets version
 */
namespace StreamIOThread {
    enum class StreamType{
        PIPE, ///<A linux named pipe (FIFO)
        SOCKET, ///<A network socket
        POSIX_SHARED_MEM ///< POSIX Shared Memory
    };

    /**
     * @brief Gets a mapping of input port numbers to FIFOs (and FIFO ports).  Each input may be used by more than 1 FIFO, thus why the map returns an array.
     * @param inputFIFOs
     * @return
     */
    std::map<int, std::vector<std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>>> getInputPortFIFOMapping(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs);

    /**
     * @brief Gets a mapping of output port numbers to FIFOs (and FIFO ports).  There should only be 1 FIFO per output
     * @param outputFIFOs
     * @return
     */
    std::map<int, std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>> getOutputPortFIFOMapping(std::shared_ptr<MasterOutput> masterOutput);

    /**
     * @brief
     *
     * @warning Note that masterInputVars should have datatypes which are already expanded for blocking
     *
     * @param ioThread
     * @param masterInputVars
     * @param inputPortFifoMap
     * @param linuxInputPipeName
     */
    void copyIOInputsToFIFO(std::ofstream &ioThread, std::vector<Variable> masterInputVars, std::map<int, std::vector<std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>>> inputPortFifoMap, std::string linuxInputPipeName);

    /**
     * @brief
     *
     * @warning Note that masterInputVars should have datatypes which are already expanded for blocking
     *
     * @param ioThread
     * @param masterOutputVars
     * @param outputPortFifoMap
     * @param outputMaster
     * @param linuxOutputPipeName
     */
    void copyFIFOToIOOutputs(std::ofstream &ioThread, std::vector<Variable> masterOutputVars, std::map<int, std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>> outputPortFifoMap, std::shared_ptr<MasterOutput> outputMaster, std::string linuxOutputPipeName);

    /**
     * @brief Emits an I/O handler for multi-threaded emit focused on benchmarking.  This version opens 2 linux named pipes (FIFOs).  Data is provided by an external program and
     *
     * @param inputFIFOs
     * @param outputFIFOs
     * @param path
     * @param fileNamePrefix
     * @param designName
     * @param blockSize
     * @param fifoHeaderFile
     * @param ioFifoSize The size of the fifo in blocks (only pertains to POSIX shared memory)
     * @param threadDebugPrint
     * @param printTelem
     */
    void emitStreamIOThreadC(std::shared_ptr<MasterInput> inputMaster, std::shared_ptr<MasterOutput> outputMaster,
                             std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs,
                             std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs,
                             std::string path, std::string fileNamePrefix, std::string designName,
                             StreamType streamType, unsigned long blockSize, std::string fifoHeaderFile,
                             std::string fifoSupportFile, int32_t ioFifoSize, bool threadDebugPrint,
                             bool printTelem, EmitterHelpers::TelemetryLevel telemLevel,
                             int telemReportFreqBlockFreq, double reportPeriodSeconds, std::string telemDumpFilePrefix,
                             bool telemAvg, PartitionParams::FIFOIndexCachingBehavior fifoIndexCachingBehavior,
                             std::string streamNameSuffix);

    void emitSocketClientLib(std::shared_ptr<MasterInput> inputMaster, std::shared_ptr<MasterOutput> outputMaster, std::string path, std::string fileNamePrefix, std::string fifoHeaderFile, std::string designName);

    void sortIntoBundles(std::vector<Variable> inputMasterVars, std::vector<Variable> outputMasterVars,
                         std::vector<int> inputBlockSizes, std::vector<int> outputBlockSizes,
                         std::map<int, std::pair<std::vector<Variable>, std::vector<int>>> &masterInputBundles,
                         std::map<int, std::pair<std::vector<Variable>, std::vector<int>>> &masterOutputBundles, std::set<int> &bundles);

    /**
     * @brief Emits the helper files for working with shared memory fifos which use the POSIX API
     * @param path the path to emit the file to
     * @return the header filename
     */
    std::string emitSharedMemoryFIFOHelperFiles(std::string path);

    std::string emitFileStreamHelpers(std::string path, std::string fileNamePrefix);
};


#endif //VITIS_STREAMIOTHREAD_H
