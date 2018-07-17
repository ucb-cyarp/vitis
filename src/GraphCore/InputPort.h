//
// Created by Christopher Yarp on 7/17/18.
//

#ifndef VITIS_INPUTPORT_H
#define VITIS_INPUTPORT_H

#include "Port.h"

/**
* \addtogroup GraphCore Graph Core
*/
/*@{*/

/**
 * @brief The representation of a node's input port
 *
 * Primarily serves as a container for pointers to arcs connected to the given port.
 */
class InputPort : public Port{
public:
    /**
     * @brief Construct an empty input port object
     *
     * The port object has number 0, no parent, and no arcs.
     */
    InputPort();

    /**
     * @brief Construct a port input object with the specified parent and port numbers.
     *
     * The port object is initialized with an empty arc list
     *
     * @param parent The node this port belongs to
     * @param portNum The port number
     */
    InputPort(Node* parent, int portNum);

    /**
     * @brief Validate if the connections to this port are correct.
     *
     *   - Verify that 1 and only 1 arc is connected (unconnected ports should have an arc from the unconnected master node)
     *
     * If an invalid configuration is detected, the function will throw an exception
     */
    void validate() override;

    /**
     * @brief Get an aliased shared pointer to this InputPort
     *
     * This is a more specialized version of @ref Port:getSharedPointer
     *
     * @note Does not override @ref Port:getSharedPointer due to the different return type
     *
     * @return an aliased shared pointer to the port
     */
    std::shared_ptr<InputPort> getSharedPointerInputPort(); //NOTE: should never return a bare pointer or an unaliased shared pointer to a port
};

/*@}*/


#endif //VITIS_INPUTPORT_H
