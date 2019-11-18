//
// Created by Christopher Yarp on 9/17/19.
//

#ifndef VITIS_THREADCROSSINGFIFOPARAMETERS_H
#define VITIS_THREADCROSSINGFIFOPARAMETERS_H

#include <string>

/**
 * \addtogroup MultiThread Multi-Thread Support
 * @{
 */

/**
 * @brief A container class for thread crossing FIFO parameters
 */
class ThreadCrossingFIFOParameters {
public:
    /***
     * @brief An enum for describing the underlying implementation of the thread crossing FIFO
     */
    enum class ThreadCrossingFIFOType{
        LOCKLESS_X86 ///< A lockeless FIFO that relies on the memory semantics of x86 processors (atomic write and write order preservation from single core)
    };

    static ThreadCrossingFIFOType parseThreadCrossingFIFOType(std::string str);

    static std::string threadCrossingFIFOTypeToString(ThreadCrossingFIFOType threadCrossingFifoType);
};

/*! @} */

#endif //VITIS_THREADCROSSINGFIFOPARAMETERS_H
