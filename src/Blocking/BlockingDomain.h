//
// Created by Christopher Yarp on 8/23/21.
//

#ifndef VITIS_BLOCKINGDOMAIN_H
#define VITIS_BLOCKINGDOMAIN_H

#include "GraphCore/SubSystem.h"
#include "GraphCore/ContextRoot.h"
#include "GraphCore/NodeFactory.h"

#include "BlockingInput.h"
#include "BlockingOutput.h"

/**
 * \addtogroup Blocking Blocking Support Nodes
 * @brief Nodes to support blocked sample processing
 *
 * @{
 */

/**
 * @brief Represents a set of nodes which operate on a sub-block of information.
 *
 * Arcs going into and out of this blocking domain are expanded along one dimension with inputs looped over in the
 * domain.  BlockingInput and Blocking nodes are placed at the boundaries of the Blocking domain
 */
class BlockingDomain : public SubSystem, public ContextRoot {
    friend class NodeFactory;
    int blockingLen; ///< The number of samples at the boarder of this blocking domain
    int subBlockingLen; ///< The number of samples processed at once inside of the blocking domain (the increment for each loop of the blocking domain)
    std::vector<std::shared_ptr<BlockingInput>> blockInputs; ///< The inputs to this blocking domain
    std::vector<std::shared_ptr<BlockingOutput>> blockOutputs; ///< The outputs from this blocking domain
    //TODO: Need I/O Ports?

protected:
    /**
     * @brief Default constructor
     */
    BlockingDomain();

    /**
     * @brief Construct a node with a given parent.
     * @param parent parent of the new node
     */
    explicit BlockingDomain(std::shared_ptr<SubSystem> parent);

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
    BlockingDomain(std::shared_ptr<SubSystem> parent, BlockingDomain* orig);

public:
    int getBlockingLen() const;
    void setBlockingLen(int blockingLen);
    int getSubBlockingLen() const;
    void setSubBlockingLen(int subBlockingLen);
    std::vector<std::shared_ptr<BlockingInput>> getBlockInputs() const;
    void setBlockInputs(const std::vector<std::shared_ptr<BlockingInput>> &blockInputs);
    std::vector<std::shared_ptr<BlockingOutput>> getBlockOutputs() const;
    void setBlockOutputs(const std::vector<std::shared_ptr<BlockingOutput>> &blockOutputs);
    void removeBlockInput(std::shared_ptr<BlockingInput> blockInput);
    void removeBlockOutput(std::shared_ptr<BlockingOutput> blockOutput);

    Variable getBlockIndVar();

    /**
     * @brief Get the number of sub-blocks evaluated per block.
     * @return
     */
    int getNumberSubBlocks();

    void addBlockInput(std::shared_ptr<BlockingInput> input);
    void addBlockOutput(std::shared_ptr<BlockingOutput> output);

    void validate() override;

    std::string typeNameStr() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type) override;
    std::set<GraphMLParameter> graphMLParameters() override;
    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;
    void shallowCloneWithChildren(std::shared_ptr<SubSystem> parent, std::vector<std::shared_ptr<Node>> &nodeCopies, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode) override;

    //==== Implement Context Root Functions ====
    int getNumSubContexts() const override; //Only 1 Subcontext
    std::vector<std::shared_ptr<Arc>> getContextDecisionDriver() override; //No direct driver.  Static For loop.  Difficult to convert to a context driver (wrapping counter) because the contents of the loop need to run more than once per compute function.  Outside would need to somehow run at a higher rate, and then execute other sections less frequently.

    //No additional variables need to be declared outside of the scope since the BlockingOutputs are the gateway for all
    //outputs of the blocking subsystem.

    /**
     * @brief Gets the variables for the BlockingOutputs because they need to be declared outside of the BlockingDomain
     *
     * @return
     */
    std::vector<Variable> getCContextVars() override;
    Variable getCContextVar(int contextVarIndex) override;

    bool requiresContiguousContextEmits() override; //Yes because the blocking domain is emitted as a single, static, for loop.  Emitting in multiple for loops is not possible in the case of loops with delays < block length

    void emitCContextOpenFirst(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber, int partitionNum) override;
    void emitCContextOpenMid(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber, int partitionNum) override;
    void emitCContextOpenLast(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber, int partitionNum) override;

    void emitCContextCloseFirst(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber, int partitionNum) override;
    void emitCContextCloseMid(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber, int partitionNum) override;
    void emitCContextCloseLast(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber, int partitionNum) override;

    bool shouldReplicateContextDriver() override; //No need since there should not be any

    /**
     * @brief Discover and mark contexts for nodes at and within this blocking domain
     *
     * Propagates blocking domain contexts to its nodes (and recursively to nested enabled subsystems).
     *
     * This function is almost identical to EnabledSubsystem::discoverAndMarkContexts
     *
     * @param contextStack the context stack up to this node
     * @return nodes in the context
     */
    std::vector<std::shared_ptr<Node>> discoverAndMarkContexts(std::vector<Context> contextStack);
};

/*! @} */

#endif //VITIS_BLOCKINGDOMAIN_H
