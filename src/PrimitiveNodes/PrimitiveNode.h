//
// Created by Christopher Yarp on 7/17/18.
//

#ifndef VITIS_PRIMITIVENODE_H
#define VITIS_PRIMITIVENODE_H

#include "GraphCore/Node.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 *
 * @brief A set of primitive nodes in a Data Flow Graph
 */
/*@{*/

/**
 * @brief Represents a primitive node in the Date Flow Graph
 *
 * This is an abstract class.  It is not a friend of the NodeFactory class and the constructors are kept protected
 * as a result
 */
class PrimitiveNode : virtual public Node{
protected:
    /**
     * @brief Constructs a default primitive node
     */
    PrimitiveNode();

    /**
     * @brief Constructs a default primitive node with a given parent.
     *
     * @param parent parent node
     */
    explicit PrimitiveNode(std::shared_ptr<SubSystem> parent);

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
    PrimitiveNode(std::shared_ptr<SubSystem> parent, Node* orig);

public:
    //Primitive nodes cannot be expanded
    bool canExpand() override;
};

/*@}*/

#endif //VITIS_PRIMITIVENODE_H
