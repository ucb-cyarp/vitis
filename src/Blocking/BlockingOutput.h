//
// Created by Christopher Yarp on 8/24/21.
//

#ifndef VITIS_BLOCKINGOUTPUT_H
#define VITIS_BLOCKINGOUTPUT_H

#include "BlockingBoundary.h"
#include "GraphCore/NodeFactory.h"

/**
 * \addtogroup Blocking Blocking Support Nodes
 * @{
 */

class BlockingOutput : public BlockingBoundary {
    friend NodeFactory;

protected:
    /**
     * @brief Default constructor
     */
    BlockingOutput();

    /**
     * @brief Construct a node with a given parent
     * @param parent parent of new node
     */
    explicit BlockingOutput(std::shared_ptr<SubSystem> parent);

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
    BlockingOutput(std::shared_ptr<SubSystem> parent, BlockingOutput* orig);

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

    /**
     * @brief Get the output variable that is declared outside of the BlockingDomain
     * @return
     */
    Variable getOutputVar();

    //TODO: Evaluate declaring the output variables as context variables.  These need to be declared outside of the context
    //      The alternative is to declare this node as having state.  However, that state would be stored in the state structure and not as an automatic


};

/*! @} */

#endif //VITIS_BLOCKINGOUTPUT_H
