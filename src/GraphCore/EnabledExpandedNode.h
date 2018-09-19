//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_ENABLEDEXPANDEDNODE_H
#define VITIS_ENABLEDEXPANDEDNODE_H

#include "EnabledSubSystem.h"
#include "ExpandedNode.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Represents a node which, when expanded, became an enabled subsystem.
 *
 * Extends both @ref EnabledSubSystem and @ref ExpandedNode, both of which extend @ref SubSystem
 */
class EnabledExpandedNode : public EnabledSubSystem, public ExpandedNode {
friend class NodeFactory;

protected:
    /**
     * @brief Default constructor
     */
    EnabledExpandedNode();

    /**
     * @brief Construct a new node with a given parent
     * @param parent the parent for the new node
     */
    EnabledExpandedNode(std::shared_ptr<SubSystem> parent);

    /**
     * @brief Construct a node with a given parent and given orig node.
     *
     * Copies the Input/Output ports of the orig node to aid in restoration of orig node if required (see note in @ref ExpandedNode description).
     *
     * @warning Enable port needs to be handled manually as it is not known which port is connected to the enable.
     *
     * @note Expansion should be run after arcs have been added to the design
     *
     * @param parent of new node
     * @param orig original node
     * @param nop needed an extra argument to seperate this from the clone constructor
     */
    EnabledExpandedNode(std::shared_ptr<SubSystem> parent, std::shared_ptr<Node> orig, void* nop);

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
    EnabledExpandedNode(std::shared_ptr<SubSystem> parent, std::shared_ptr<EnabledExpandedNode> orig);

    //Init should be overwritten by EnabledSubSystem

    std::string labelStr() override ;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;
};

/*@}*/

#endif //VITIS_ENABLEDEXPANDEDNODE_H
