//
// Created by Christopher Yarp on 7/17/18.
//

#ifndef VITIS_MEDIUMLEVELNODE_H
#define VITIS_MEDIUMLEVELNODE_H

#include "GraphCore/Node.h"
#include "GraphCore/ExpandedNode.h"

/**
 * \addtogroup MediumLevelNodes Medium Level Nodes
 *
 * @brief Expandable to Primitives But Typically has a Single Implementation Type
 *
 * A Convenience For Referring to a Common Structure
 * @{
*/

/**
 * @brief Represents a Medium Level Node which is Expandable to Primitives But Typically has a Single Implementation Type
 *
 * A Convenience For Referring to a Common Structure
 */
class MediumLevelNode : public Node {
protected:
    /**
     * @brief Constructs a default medium level node
     */
    MediumLevelNode();

    /**
     * @brief Constructs a default medium level node with a given parent.
     *
     * @param parent parent node
     */
    explicit MediumLevelNode(std::shared_ptr<SubSystem> parent);

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
    MediumLevelNode(std::shared_ptr<SubSystem> parent, MediumLevelNode* orig);

public:
    //Medium level nodes can be expanded
    bool canExpand() override;
};

/*! @} */

#endif //VITIS_MEDIUMLEVELNODE_H
