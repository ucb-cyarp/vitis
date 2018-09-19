//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_ENABLENODE_H
#define VITIS_ENABLENODE_H

#include <memory>
#include "Node.h"
#include "SubSystem.h"
#include "EnablePort.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Represents a special port at the interface of an enabled subsystem
 *
 * @note This class is an abstract class with concrete classes being @ref EnableInput and @ref EnableOutput
 */
class EnableNode : public Node {
friend class NodeFactory;

protected:
    //NOTE: made into a unique pointer so that a pointer to the port can be reliably shared.
    std::unique_ptr<EnablePort> enablePort; ///< The enable port.  The input of this port determines if a new value is propagated or not

    /**
     * @brief Default constructor
     */
    EnableNode();

    /**
     * @brief Construct a node with a given parent
     * @param parent parent of the new node
     */
    explicit EnableNode(std::shared_ptr<SubSystem> parent);

    /**
     * @brief Constructs a new node with a shallow copy of parameters from the origional node.  Ports are not copied and neither is the parent reference.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @note If copying a graph, the parent should be one of the copies and not from the origional graph.
     *
     * @warning Because pointer (this) is passed to ports, nodes must be allocated on the heap and not moved.  All interaction should be via pointers.
     *
     * @param parent parent node
     * @param orig The origional node from which a shallow copy is being made
     */
    EnableNode(std::shared_ptr<SubSystem> parent, std::shared_ptr<EnableNode> orig);

public:
    /**
    * @brief Set the enable arc of the given node, updating referenced objects in the process
    *
    * Adds an input arc to this node.  Removes the arc from the orig port if not NULL.
    * Updates the given arc so that the destination port is set to the specified input
    * port of this node.  If the port object for the given number is not yet created, it
    * will be created during the call.
    * @param arc The arc to add
    */
    void setEnableArcUpdatePrevUpdateArc(std::shared_ptr<Arc> arc);

    /**
     * @brief Get a reference to the enable port
     * @return reference to the enable port
     */
    std::shared_ptr<Port> getEnablePort();

    /**
     * @brief Performs check of @ref Node in addition to checking the enable port
     */
    void validate() override;

    bool canExpand() override;
};

/*@}*/

#endif //VITIS_ENABLENODE_H
