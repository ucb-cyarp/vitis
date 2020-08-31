//
// Created by Christopher Yarp on 8/31/20.
//

#ifndef VITIS_PARTITIONNODE_H
#define VITIS_PARTITIONNODE_H

#include "GraphCore/Node.h"
#include "GraphCore/NodeFactory.h"

/**
 * \addtogroup Estimators Estimators
 * @{
*/

/**
 * @brief This represent a partition as a node in a graph.  This is used to report information about communication as
 * well as to detect deadlock in multithreaded emit.
 *
 * @note This node is not
 */
class PartitionNode : public Node{
    friend NodeFactory;

    //==== Constructors ====
    /**
     * @brief Constructs an empty PartitionNode node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    PartitionNode();

    /**
     * @brief Constructs an empty PartitionNode node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit PartitionNode(std::shared_ptr<SubSystem> parent);

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
    PartitionNode(std::shared_ptr<SubSystem> parent, PartitionNode* orig);

public:
    //No need to import from GraphML
    //Do not need to generate C
    //Do not need to validate

    //==== Emit Functions ====
    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type) override ;

    std::string typeNameStr() override;

    std::string labelStr() override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    bool canExpand() override;

};

/*! @} */

#endif //VITIS_PARTITIONNODE_H
