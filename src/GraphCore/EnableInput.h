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
 *
 * Note: While the enable input does not need to be explically scheduled (it simply passes through the upstream result)
 * it is useful for providing ordering constraints for the scheduler for any node inside the enabled subsystem.
 *
 * If there is a need to remove EnableInput nodes, it is important to create an ordering dependency between the enable
 * driver and the nodes that directly depend on the
 *
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
    EnableInput(std::shared_ptr<SubSystem> parent, EnableInput* orig);

public:

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

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    /**
     * @brief Emits the C Statement for the EnableInput
     *
     * The EnableInput only passes values through.  It's presence in the graph is primarily to provide an ordering
     * constraint for any node inside the enabled subsystem on the enable driver.
     */
    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) override;

};

/*@}*/

#endif //VITIS_ENABLEINPUT_H
