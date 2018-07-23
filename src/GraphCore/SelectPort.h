//
// Created by Christopher Yarp on 7/17/18.
//

#ifndef VITIS_SELECTPORT_H
#define VITIS_SELECTPORT_H

#include "InputPort.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief The representation of a mux node's select port
 *
 * Primarily serves as a container for pointers to arcs connected to the given port.
 */
class SelectPort : public InputPort {
public:
    /**
     * @brief Construct an empty select port object
     *
     * The port object has number 0, no parent, and no arcs.
     */
    SelectPort();

    /**
     * @brief Construct a port select object with the specified parent and port numbers.
     *
     * The port object is initialized with an empty arc list
     *
     * @param parent The node this port belongs to
     * @param portNum The port number
     */
    SelectPort(Node* parent, int portNum);

    /**
     * @brief Validate if the connections to this port are correct.
     *
     *   - For Select Ports, verify that the input port has an integer type and width 1.
     *
     * If an invalid configuration is detected, the function will throw an exception
     */
    void validate() override;

    /**
     * @brief Get an aliased shared pointer to this SelectPort
     *
     * This is a more specialized version of @ref Port::getSharedPointer
     *
     * @note Does not override @ref Port::getSharedPointer due to the different return type
     *
     * @return an aliased shared pointer to the port
     */
    std::shared_ptr<SelectPort> getSharedPointerSelectPort(); //NOTE: should never return a bare pointer or an unaliased shared pointer to a port

};

/*@}*/

#endif //VITIS_SELECTPORT_H
