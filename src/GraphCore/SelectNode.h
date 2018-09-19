//
// Created by Christopher Yarp on 9/19/18.
//

#ifndef VITIS_SELECTNODE_H
#define VITIS_SELECTNODE_H

#include "Node.h"
#include "NodeFactory.h"
#include "SelectPort.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief This is an abstract class that represent a node with a select input
 */
class SelectNode : virtual public Node{
    friend class NodeFactory;
protected:
    std::unique_ptr<SelectPort> selectorPort;///< The port which determines the selection

    /**
     * @brief Constructs an empty node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @warning Because pointer (this) is passed to ports, nodes must be allocated on the heap and not moved.  All interaction should be via pointers.
     */
    SelectNode();

    /**
     * @brief Constructs an empty node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @warning Because pointer (this) is passed to ports, nodes must be allocated on the heap and not moved.  All interaction should be via pointers.
     *
     * @param parent parent node
     */
    explicit SelectNode(std::shared_ptr<SubSystem> parent);

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
    SelectNode(std::shared_ptr<SubSystem> parent, SelectNode* orig);

public:
    //==== Getters/Setters ====
    /**
     * @brief Get an aliased shared pointer to the selector port.
     *
     * The pointer is aliased with this node as the stored pointer.
     *
     * @return aliased shared pointer to the selector port
     */
    std::shared_ptr<SelectPort> getSelectorPort() const;

    /**
    * @brief Add a select arc to the given node, updating referenced objects in the process
    *
    * Adds an select arc to this node.  Removes the arc from the orig port if not NULL.
    * Updates the given arc so that the destination port is set to the specified input
    * port of this node.  If the port object for the given number is not yet created, it
    * will be created during the call.
    * @param arc The arc to add
    */
    void addSelectArcUpdatePrevUpdateArc(std::shared_ptr<Arc> arc);

    void cloneInputArcs(std::vector<std::shared_ptr<Arc>> &arcCopies, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode, std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &origToCopyArc, std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &copyToOrigArc) override;

};

/*@}*/

#endif //VITIS_SELECTNODE_H
