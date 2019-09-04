//
// Created by Christopher Yarp on 10/16/18.
//

#ifndef VITIS_ORDERCONSTRAINTINPUTPORT_H
#define VITIS_ORDERCONSTRAINTINPUTPORT_H

#include "Port.h"
#include "InputPort.h"

/**
* \addtogroup GraphCore Graph Core
* @{
*/

/**
 * @brief A virtual port which serves as the endpoint for arcs constraining the scheduling order of nodes when the destination node does not directly consume the result produced by the src node.
 *
 * Used to constrain nodes in mux or enabled subsystem contexts
 *
 * Unlike standard input ports, the OrderConstraintInputPort is capable of having multiple incoming arcs.  This is
 * because the arcs do not actually drive a signal but enforce ordering constraints for nodes.
 * Also, unlike standard ports, the OrderConstraintInputPort is allowed to be disconnected.
 *
 */
class OrderConstraintInputPort : public InputPort {
public:
    /**
     * @brief Construct an empty OrderConstraintInput port object
     *
     * The port object has number 0, no parent, and no arcs.
     */
    OrderConstraintInputPort();

    /**
     * @brief Construct a OrderConstraintInput port object with the specified parent.
     *
     * The port number is set to 0 because there is no need for seperate OrderConstraintInput ports.
     *
     * @param parent
     */
    explicit OrderConstraintInputPort(Node* parent);

    /**
     * @brief Validates if connections to the port are correct
     *
     * The OrderConstraintInputPort is allowed to be disconnected and to have multiple input arcs.
     *
     * Validation effectivly does nothing.
     */
    void validate() override;

    /**
     * @brief Get an aliased shared pointer to this OrderConstraint
     *
     * This is a more specialized version of @ref Port::getSharedPointer
     *
     * @note Does not override @ref Port::getSharedPointer due to the different return type
     *
     * @return an aliased shared pointer to the port
     */
    std::shared_ptr<OrderConstraintInputPort> getSharedPointerOrderConstraintPort();

    /**
     * @brief Checks if the port experiences internal fanout
     * @param imag if true, check if the imagionary component has internal fanout, if false, check if the real component has internal fanout
     * @return true if the input is internally fanned out, false otherwise
     *
     * Since nodes do not directly use the singals from this port, this method returns false.
     */
    bool hasInternalFanout(bool imag) override;

};

/*! @} */


#endif //VITIS_ORDERCONSTRAINTINPUTPORT_H
