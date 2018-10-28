//
// Created by Christopher Yarp on 10/27/18.
//

#ifndef VITIS_ORDERCONSTRAINTOUTPUTPORT_H
#define VITIS_ORDERCONSTRAINTOUTPUTPORT_H

#include "OutputPort.h"

/**
* \addtogroup GraphCore Graph Core
*/
/*@{*/

/**
 * @brief A virtual port which serves as the sorce for arcs constraining the scheduling order of nodes when the destination node does not directly consume the result produced by the src node.
 *
 * This port allows sink nodes to have order constraints on nodes to be scheduled after them.
 *
 * Used to constrain state update nodes
 *
 * Also, unlike standard ports, the OrderConstraintOutputPort is allowed to be disconnected.
 *
 */

class OrderConstraintOutputPort : public OutputPort{
public:
    /**
     * @brief Construct an empty OrderConstraintInput port object
     *
     * The port object has number 0, no parent, and no arcs.
     */
    OrderConstraintOutputPort();

    /**
     * @brief Construct a OrderConstraintInput port object with the specified parent.
     *
     * The port number is set to 0 because there is no need for seperate OrderConstraintInput ports.
     *
     * @param parent
     */
    explicit OrderConstraintOutputPort(Node* parent);

    /**
     * @brief Validates if connections to the port are correct
     *
     * The OrderConstraintOutputPort is allowed to be disconnected
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
    std::shared_ptr<OrderConstraintOutputPort> getSharedPointerOrderConstraintPort();

};

/*@}*/

#endif //VITIS_ORDERCONSTRAINTOUTPUTPORT_H
