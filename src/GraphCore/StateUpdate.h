//
// Created by Christopher Yarp on 10/25/18.
//

#ifndef VITIS_STATEUPDATE_H
#define VITIS_STATEUPDATE_H

#include "Node.h"
#include "NodeFactory.h"
#include <memory>

/**
* \addtogroup GraphCore Graph Core
*/
/*@{*/

/**
 * @brief A node that represents the state update for a node which contains state
 *
 * This node is unusual in that it is not typically part of a DSP design as state updates implicityally occure at the
 * next clock edge.  The state update node is used to schedule the state update when the data flow graph's execution
 * is serialized.  The state update can occur once its output has been used by all destination nodes.
 *
 * Allowing the state element to occur at locations other than the end of the file (or context) can be benificial as it
 * gives the scheduler more flexibility on where to place the state updates.
 *
 * It is also helpful when dealing with state element updates which are contextual (ex. enabled subsystems).  By
 * including the state update in the same context as the primary node, the emitter will automatically place it in the
 * right context.  The emitter does not need to know when it is done emitting a context before emitting the state updates
 * for the context.  This makes the emitter simpler.  It also allows it to handle the case when contexts may be
 * interleaved.
 *
 * This node does not supply many methods and replys on the primary node to supply the appropriate methods
 *
 * This node also reports that it does not contain state to avoid any recursive creation of
 *
 */
class StateUpdate : public Node {
    friend NodeFactory;

private:
    std::shared_ptr<Node> primaryNode; ///<The primary node with state for which this represents the state update

    //==== Constructors ====
    /**
     * @brief Constructs a default StateUpdate node with no primaryNode
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     */
    StateUpdate();

    /**
     * @brief Constructs a default StateUpdate node with no primaryNode, with a given parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit StateUpdate(std::shared_ptr<SubSystem> parent);

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
    StateUpdate(std::shared_ptr<SubSystem> parent, StateUpdate* orig);

public:
    //Getters & Setters
    std::shared_ptr <Node> getPrimaryNode() const;
    void setPrimaryNode(const std::shared_ptr <Node> &primaryNode);

    //Note: Currently is not exported as part of the XML
    //TODO: Implement XML Export/Import
    std::set<GraphMLParameter> graphMLParameters() override;
    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    bool canExpand() override;

    void validate() override;

    /**
     * @note Clone function does not set the reference for the primary node.  This needs to be done after all nodes
     * have been copied since we must be sure that the primary node was copied
     */
    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    /**
     * @brief Emits the C code to update the state variables of the primary node
     *
     * @param cStatementQueue a reference to the queue containing C statements for the function being emitted.  The state update statements for this node are enqueued onto this queue
     */
    void emitCStateUpdate(std::vector<std::string> &cStatementQueue) override;
};

/*@}*/

#endif //VITIS_STATEUPDATE_H
