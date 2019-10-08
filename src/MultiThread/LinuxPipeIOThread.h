//
// Created by Christopher Yarp on 10/4/19.
//

#ifndef VITIS_LINUXPIPEIOTHREAD_H
#define VITIS_LINUXPIPEIOTHREAD_H

#include <utility>
#include <string>
#include "ThreadCrossingFIFO.h"

class LinuxPipeIOThread {
public:

    static std::string getInputStructDefn(std::shared_ptr<MasterInput> inputMaster, std::string structTypeName, int blockSize);

    static std::string getOutputStructDefn(std::shared_ptr<MasterOutput> outputMaster, std::string structTypeName, int blockSize);

    /**
     * @brief Gets a mapping of input port numbers to FIFOs (and FIFO ports).  Each input may be used by more than 1 FIFO, thus why the map returns an array.
     * @param inputFIFOs
     * @return
     */
    static std::map<int, std::vector<std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>>> getInputPortFIFOMapping(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs);

    /**
     * @brief Gets a mapping of output port numbers to FIFOs (and FIFO ports).  There should only be 1 FIFO per output
     * @param outputFIFOs
     * @return
     */
    static std::map<int, std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>> getOutputPortFIFOMapping(std::shared_ptr<MasterOutput> masterOutput);

    static void copyIOInputsToFIFO(std::ofstream &ioThread, std::vector<Variable> masterInputVars, std::map<int, std::vector<std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>>> inputPortFifoMap, std::string linuxInputPipeName, int blockSize);

    static void copyFIFOToIOOutputs(std::ofstream &ioThread, std::vector<Variable> masterOutputVars, std::map<int, std::pair<std::shared_ptr<ThreadCrossingFIFO>, int>> outputPortFifoMap, std::shared_ptr<MasterOutput> outputMaster, std::string linuxOutputPipeName, int blockSize);

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
     * @param threadDebugPrint
     */
    static void emitLinuxPipeIOThreadC(std::shared_ptr<MasterInput> inputMaster, std::shared_ptr<MasterOutput> outputMaster, std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, std::string path, std::string fileNamePrefix, std::string designName, unsigned long blockSize, std::string fifoHeaderFile, bool threadDebugPrint);
};


#endif //VITIS_LINUXPIPEIOTHREAD_H
