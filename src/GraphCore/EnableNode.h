//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_ENABLENODE_H
#define VITIS_ENABLENODE_H

#include "Port.h"
#include "Node.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Represents a special port at the interface of an enabled subsystem
 *
 * @note This class is an abstract class with concrete classes being @ref EnableInput and @ref EnableOutput
 */
class EnableNode : public Node {
protected:
    Port enablePort; ///< The enable port.  The input of this port determines if a new value is propagated or not
};

/*@}*/

#endif //VITIS_ENABLENODE_H
