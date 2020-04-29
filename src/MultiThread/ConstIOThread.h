//
// Created by Christopher Yarp on 10/4/19.
//

#ifndef VITIS_CONSTIOTHREAD_H
#define VITIS_CONSTIOTHREAD_H

#include <vector>
#include "ThreadCrossingFIFO.h"
#include <string>

/**
 * \addtogroup MultiThread Multi-Thread Support
 * @{
 */

class ConstIOThread {
public:
/**
     * @brief Emits an I/O handler for multi-threaded emit focused on benchmarking.  This version feeds constant values to the design
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
    static void emitConstIOThreadC(std::vector<std::shared_ptr<ThreadCrossingFIFO>> inputFIFOs, std::vector<std::shared_ptr<ThreadCrossingFIFO>> outputFIFOs, std::string path, std::string fileNamePrefix, std::string designName, unsigned long blockSize, std::string fifoHeaderFile, bool threadDebugPrint);

};

/*! @} */

#endif //VITIS_CONSTIOTHREAD_H
