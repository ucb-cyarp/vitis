//
// Created by Christopher Yarp on 7/17/18.
//

#ifndef VITIS_OUTPUTPORT_H
#define VITIS_OUTPUTPORT_H

#include "Port.h"

/**
* \addtogroup GraphCore Graph Core
*/
/*@{*/

/**
 * @brief The representation of a node's output port
 *
 * Primarily serves as a container for pointers to arcs connected to the given port.
 */
class OutputPort : public Port{
public:
    /**
     * @brief Construct an empty output port object
     *
     * The port object has number 0, no parent, and no arcs.
     */
    OutputPort();

    /**
     * @brief Construct a port output object with the specified parent and port numbers.
     *
     * The port object is initialized with an empty arc list
     *
     * @param parent The node this port belongs to
     * @param portNum The port number
     */
    OutputPort(Node* parent, int portNum);

    /**
     * @brief Validate if the connections to this port are correct.
     *
     *   - For Output Ports, verify that all output arcs have the same type.  Also check that there is at least 1 arc (unconnected ports should have an arc to the unconnected master)
     *
     * If an invalid configuration is detected, the function will throw an exception
     */
    void validate() override;

    /**
     * @brief Get an aliased shared pointer to this OutputPort
     *
     * This is a more specialized version of @ref Port:getSharedPointer
     *
     * @note Does not override @ref Port:getSharedPointer due to the different return type
     *
     * @return an aliased shared pointer to the port
     */
    std::shared_ptr<OutputPort> getSharedPointerOutputPort(); //NOTE: should never return a bare pointer or an unaliased shared pointer to a port

};

/*@}*/

#endif //VITIS_OUTPUTPORT_H
