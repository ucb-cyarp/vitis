//
// Created by Christopher Yarp on 8/24/21.
//

#ifndef VITIS_BLOCKINGINPUT_H
#define VITIS_BLOCKINGINPUT_H

#include "BlockingBoundary.h"
#include "GraphCore/NodeFactory.h"

/**
 * \addtogroup Blocking Blocking Support Nodes
 * @brief Nodes to support blocked sample processing
 *
 * @{
 */

/**
 * @brief Node representing data entering a blocking domain
 *
 * Handles indexing data for datatype dimensionality changes between inside and outside of the blocking domain
 */
class BlockingInput : public BlockingBoundary {
    friend class NodeFactory;

protected:
    /**
     * @brief Default constructor
     */
    BlockingInput();

    /**
     * @brief Construct a node with a given parent
     * @param parent parent of new node
     */
    explicit BlockingInput(std::shared_ptr<SubSystem> parent);

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
    BlockingInput(std::shared_ptr<SubSystem> parent, BlockingInput* orig);

public:
    bool canExpand() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;
    std::string typeNameStr() override;

    void validate() override;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    /**
     * @brief Emits the C Statement for the EnableInput
     *
     * The EnableInput only passes values through.  It's presence in the graph is primarily to provide an ordering
     * constraint for any node inside the enabled subsystem on the enable driver.
     */
    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) override;

    /**
     * @brief Also removes this node from the list of EnableInputs in the parent EnabledSubsystem
     */
    void removeKnownReferences() override;

    bool passesThroughInputs() override;
};

/*! @} */

#endif //VITIS_BLOCKINGINPUT_H
