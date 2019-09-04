//
// Created by Christopher Yarp on 8/22/18.
//

#ifndef VITIS_HIGHLEVELNODE_H
#define VITIS_HIGHLEVELNODE_H

#include "GraphCore/Node.h"

/**
 * \addtogroup HighLevelNodes High Level Nodes
 *
 * @brief Expandable to primitives and may have multiple implementation possibilities
 *
 * A Convenience For Referring to a Common Structure
 * @{
*/

/**
 * @brief Represents a High Level Node which is expandable to primitives and may have multiple implementation possibilities
 *
 */
class HighLevelNode : public Node {
protected:
    /**
     * @brief Constructs a default high level node
     */
    HighLevelNode();

    /**
     * @brief Constructs a default high level node with a given parent.
     *
     * @param parent parent node
     */
    explicit HighLevelNode(std::shared_ptr<SubSystem> parent);

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
    HighLevelNode(std::shared_ptr<SubSystem> parent, HighLevelNode* orig);

public:
    //High level nodes can be expanded
    bool canExpand() override;
};

/*! @} */

#endif //VITIS_HIGHLEVELNODE_H
