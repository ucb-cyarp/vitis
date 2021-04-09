//
// Created by Christopher Yarp on 4/29/20.
//

#ifndef VITIS_DOWNSAMPLECLOCKDOMAIN_H
#define VITIS_DOWNSAMPLECLOCKDOMAIN_H

#include "ClockDomain.h"
#include "GraphCore/ContextRoot.h"

/**
 * \addtogroup MultiRate Multi-Rate Support Nodes
 * @{
*/

class DownsampleClockDomain : public ClockDomain, public ContextRoot{
friend class NodeFactory;

private:
    std::shared_ptr<Arc> contextDriver;///<The Context driver arc for this DownsampleClockDomain which is created by DownsampleClockDomain::createSupportNodes

    /**
     * @brief Default constructor
     */
    DownsampleClockDomain();

    /**
     * @brief Construct a node with a given parent.
     * @param parent parent of the new node
     */
    explicit DownsampleClockDomain(std::shared_ptr<SubSystem> parent);

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
    DownsampleClockDomain(std::shared_ptr<SubSystem> parent, DownsampleClockDomain* orig);

public:
    std::shared_ptr <Arc> getContextDriver() const;
    void setContextDriver(const std::shared_ptr <Arc> &contextDriver);

    bool isSpecialized() override;

    void validate() override;

    std::string typeNameStr() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type) override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    void shallowCloneWithChildren(std::shared_ptr<SubSystem> parent, std::vector<std::shared_ptr<Node>> &nodeCopies, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode) override;

    /**
     * @brief Discover and mark contexts for nodes at and within this clock domain.
     *
     * Propogates clock domain contexts to its nodes (and recursively to nested  subsystems).
     *
     * @param contextStack the context stack up to this node
     * @return nodes in the context
     */
    std::vector<std::shared_ptr<Node>> discoverAndMarkContexts(std::vector<Context> contextStack) override;

    void orderConstrainZeroInputNodes(std::vector<std::shared_ptr<Node>> predecessorNodes,
                                      std::vector<std::shared_ptr<Node>> &new_nodes,
                                      std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                      std::vector<std::shared_ptr<Arc>> &new_arcs,
                                      std::vector<std::shared_ptr<Arc>> &deleted_arcs) override;

    void createSupportNodes(std::vector<std::shared_ptr<Node>> &nodesToAdd,
                            std::vector<std::shared_ptr<Node>> &nodesToRemove,
                            std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                            std::vector<std::shared_ptr<Arc>> &arcToRemove,
                            bool includeContext, bool includeOutputBridgeNodes) override;

    void setClockDomainDriver(std::shared_ptr<Arc> newDriver) override;

    //==== Implement Context Root Functions ====
    int getNumSubContexts() const override;

    std::vector<std::shared_ptr<Arc>> getContextDecisionDriver() override;

    std::vector<Variable> getCContextVars() override;
    Variable getCContextVar(int contextVarIndex) override;

    bool requiresContiguousContextEmits() override;

    void emitCContextOpenFirst(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber, int partitionNum) override;
    void emitCContextOpenMid(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber, int partitionNum) override;
    void emitCContextOpenLast(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber, int partitionNum) override;

    void emitCContextCloseFirst(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber, int partitionNum) override;
    void emitCContextCloseMid(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber, int partitionNum) override;
    void emitCContextCloseLast(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber, int partitionNum) override;

    bool shouldReplicateContextDriver() override;

    bool allowFIFOAbsorption() override;

    //Also need to modify GraphAlgs::topologicalSortDestructive to handle this type of ContextRoot
};

/*! @} */

#endif //VITIS_DOWNSAMPLECLOCKDOMAIN_H
