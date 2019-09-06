//
// Created by Christopher Yarp on 9/3/19.
//

#ifndef VITIS_LOCKLESSTHREADCROSSINGFIFO_H
#define VITIS_LOCKLESSTHREADCROSSINGFIFO_H

#include "ThreadCrossingFIFO.h"

/**
 * \addtogroup MultiThread Multi-Thread Support Nodes
 * @{
 */

class LocklessThreadCrossingFIFO : public ThreadCrossingFIFO {

protected:
    Variable cWritePtr; ///<The C variable corresponding to the write pointer
    Variable cReadPtr; ///<The C variable corresponding to the read pointer

};

/*! @} */


#endif //VITIS_LOCKLESSTHREADCROSSINGFIFO_H
