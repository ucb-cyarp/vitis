//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_EXPANDEDNODE_H
#define VITIS_EXPANDEDNODE_H

#include <memory>
#include "SubSystem.h"
#include "Node.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Represents a node that has been expanded.
 *
 * An expansion may be conducted by an external tool or by a pass over the data flow graph.  Expanded nodes become
 * Sub-systems which encapsulate the logic which implements them.  To facilitate the re-expansion or modification of
 * the expanded node, a reference to the original node (containing the original parameters used for expansion) is kept.
 *
 * @note The expanded node can have input and output ports.  These ports do not act as standard input/output ports but,
 * rather, act as containers for arcs connected to blocks contained within.  These ports can be used to restore the
 * orig (un-expanded) node.  No arcs should use the ports of the expanded Subsystem as a source or destination.
 *
 */
class ExpandedNode : public virtual SubSystem{
friend class NodeFactory;

protected:
    std::shared_ptr<Node> origNode; ///< A pointer to the original node before expansion

    /**
     * @brief Default Constructor.  Sets orig node to null.
     */
    ExpandedNode();

    /**
     * @brief Construct a node with a given parent.  Orig node is set to null
     * @param parent parent of new node
     */
    explicit ExpandedNode(std::shared_ptr<SubSystem> parent);

    /**
     * @brief Construct a node with a given parent and given orig node.
     *
     * Copies the Input/Output ports of the orig node to aid in restoration of orig node if required (see note in @ref ExpandedNode description).
     *
     * Sets the ID of the ExpandedNode to that of the original.
     * Sets name to Expanded(<OrigName>)
     *
     * @note Expansion should be run after arcs have been added to the design
     *
     * @param parent of new node
     * @param orig original node
     * @param nop needed another argument to seperate this from clone constructor
     */
    ExpandedNode(std::shared_ptr<SubSystem> parent, std::shared_ptr<Node> orig, void* nop);

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
    ExpandedNode(std::shared_ptr<SubSystem> parent, ExpandedNode* orig);

public:
    //==== Getters/Setters ====
    const std::shared_ptr<Node> getOrigNode() const;
    void setOrigNode(std::shared_ptr<Node> origNode);

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;
};

/*@}*/

#endif //VITIS_EXPANDEDNODE_H
