//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_ENABLEINPUT_H
#define VITIS_ENABLEINPUT_H

#include "EnableNode.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Represents a special input port at the interface of an enabled subsystem.
 *
 * If not enabled, the downstream logic from this node is not executed in the given clock cycle and the previous state
 * is held.  If enabled, the downstream logic from this node is allowed to execute as usual and state is updated.
 */
class EnableInput : public EnableNode {
public:
};

/*@}*/

#endif //VITIS_ENABLEINPUT_H
