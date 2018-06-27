//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_ENABLEOUTPUT_H
#define VITIS_ENABLEOUTPUT_H

#include "EnableNode.h"

/**
 * @brief Represents a special output port at the interface of an enabled subsystem.
 *
 * If not enabled, the previous value of this node is fed to the downstream logic.
 * If enabled, the calculated value is fed to the downstream logic as usual.
 */
class EnableOutput : EnableNode{
public:
    std::string emitCpp(int outputPort) override ;
};


#endif //VITIS_ENABLEOUTPUT_H
