//
// Created by Christopher Yarp on 7/17/18.
//

#ifndef VITIS_ENABLEPORT_H
#define VITIS_ENABLEPORT_H

#include "InputPort.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief The representation of an enable node's enable port
 *
 * Primarily serves as a container for pointers to arcs connected to the given port.
 */
class EnablePort : public InputPort {
public:
    /**
     * @brief Construct an empty enable port object
     *
     * The port object has number 0, no parent, and no arcs.
     */
    EnablePort();

    /**
     * @brief Construct a port enable object with the specified parent and port numbers.
     *
     * The port object is initialized with an empty arc list
     *
     * @param parent The node this port belongs to
     * @param portNum The port number
     */
    EnablePort(Node* parent, int portNum);

    /**
     * @brief Validate if the connections to this port are correct.
     *
     *   - Verify that the input port is a boolean type (and width 1).  Also Verify 1 and only 1 arc is connected
     *
     * If an invalid configuration is detected, the function will throw an exception
     */
    void validate() override;

    /**
     * @brief Get an aliased shared pointer to this EnablePort
     *
     * This is a more specialized version of @ref Port:getSharedPointer
     *
     * @note Does not override @ref Port:getSharedPointer due to the different return type
     *
     * @return an aliased shared pointer to the port
     */
    std::shared_ptr<EnablePort> getSharedPointerEnablePort(); //NOTE: should never return a bare pointer or an unaliased shared pointer to a port

};

/*@}*/

#endif //VITIS_ENABLEPORT_H
