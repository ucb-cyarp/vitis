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
friend class NodeFactory;

protected:
    /**
     * @brief Default constructor
     */
    EnableInput();

    /**
     * @brief Construct a node with a given parent
     * @param parent parent of new node
     */
    explicit EnableInput(std::shared_ptr<SubSystem> parent);

    /**
     * @brief Constructs a new node with a shallow copy of parameters from the original node.  Ports are not copied and neither is the parent reference.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @note If copying a graph, the parent should be one of the copies and not from the original graph.
     *
     * @warning Because pointer (this) is passed to ports, nodes must be allocated on the heap and not moved.  All interaction should be via pointers.
     *
     * @param parent parent node
     * @param orig The origional node from which a shallow copy is being made
     */
    EnableInput(std::shared_ptr<SubSystem> parent, std::shared_ptr<EnableInput> orig);

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    /**
     * @brief Validate if the connections to this port are correct.
     *
     *   - Verify that the input port is a boolean type (and width 1).  Also Verify 1 and only 1 arc is connected
     *   - Checks that the port is in the EnabledSubsystem EnabledInput list
     *
     * If an invalid configuration is detected, the function will throw an exception
     */
    void validate() override;
};

/*@}*/

#endif //VITIS_ENABLEINPUT_H
