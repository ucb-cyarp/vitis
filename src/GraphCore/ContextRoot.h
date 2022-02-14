//
// Created by Christopher Yarp on 10/23/18.
//

#ifndef VITIS_CONTEXTROOT_H
#define VITIS_CONTEXTROOT_H

#include <memory>
#include <vector>

#include "Node.h"
#include "Variable.h"
#include "ContextVariableUpdate.h"
//#include "DummyReplica.h"
//#include "ContextFamilyContainer.h"

//class Node;

//Forward declaration
class ContextFamilyContainer;
class DummyReplica;

/**
 * \addtogroup GraphCore Graph Core
 * @{
*/

/**
 * @brief A class containing information and helper methods for nodes that create contexts
 */
class ContextRoot {
protected:
    std::vector<std::vector<std::shared_ptr<Node>>> nodesInSubContexts; ///<A vector of nodes in the context (but not nodes in nested contexts)
    std::vector<std::shared_ptr<ContextVariableUpdate>> contextVariableUpdateNodes; ///<A list of ContextVariableUpdate nodes associated with this ContextRoot
    std::map<int, std::shared_ptr<ContextFamilyContainer>> contextFamilyContainers; ///<The corresponding context family containers (if any exists) for this ContextRoot
    std::map<int, std::vector<std::shared_ptr<Arc>>> contextDriversPerPartition; ///<Contains a map of context driver arcs to individual partitions (may be different arcs for each partition).  Likely set in the encapsulateContexts method
    std::map<int, std::shared_ptr<DummyReplica>> dummyReplicas; ///<A map of DummyReplica nodes for partitions this context root exists in.  These are typically created if ContextDriver replication occurs

public:
    /**
     * Virtual destructor for context root
     */
    virtual ~ContextRoot() = default;

    /**
     * @brief Add a node to the specified sub-context vector
     *
     * Extends the nodesInSubContexts vector if required to reach the specified sub context
     *
     * @param subContext specified sub context to add the node to
     * @param node node to add to the specified sub-context
     */
    void addSubContextNode(unsigned long subContext, std::shared_ptr<Node> node);

    /**
     * @brief Gets a vector of nodes in the specified sub-context (but not in nested contexts)
     * @param subContext the sub-context to get the list of nodes for
     * @return a copy of the sub-context nodes vector
     */
    std::vector<std::shared_ptr<Node>> getSubContextNodes(unsigned long subContext);

    /**
     * @brief Removes a node from the specified sub-context vector
     * @param subContext the sub-context to remove the node from
     * @param node the node to remove
     */
    void removeSubContextNode(unsigned long subContext, std::shared_ptr<Node> node);

    /**
     * @brief Get the number of subcontexts created by this node
     * @return
     */
    virtual int getNumSubContexts() const = 0;

    std::vector<std::shared_ptr<ContextVariableUpdate>> getContextVariableUpdateNodes() const;

    void setContextVariableUpdateNodes(
            const std::vector<std::shared_ptr<ContextVariableUpdate>> &contextVariableUpdateNodes);

    void addContextVariableUpdateNode(std::shared_ptr<ContextVariableUpdate> contextVariableUpdateNode);

    std::map<int, std::shared_ptr<DummyReplica>> getDummyReplicas() const;

    void setDummyReplicas(const std::map<int, std::shared_ptr<DummyReplica>> &dummyReplicas);

    std::shared_ptr<DummyReplica> getDummyReplica(int partition);

    void setDummyReplica(int partition, std::shared_ptr<DummyReplica> dummyReplica);

    void clearNodesInSubContexts();

    /**
     * @brief Gets the arc(s) that drive the decision for the context
     * @return a vector of arcs that drive the decision for the context
     */
    virtual std::vector<std::shared_ptr<Arc>> getContextDecisionDriver() = 0;

    /**
     * @brief Tells the context encapsulation method that the context driver should be replicated for each partition
     * instead of a partition crossing arc being created
     *
     * @warning There is currently a limitation that, if this returns true, only a single node with no inputs is supported
     *          If that nodes has state, the state update node insertion should occur after the copies are created
     *
     * @return
     */
    virtual bool shouldReplicateContextDriver() = 0;

    /**
     * @brief Get a list of partitions that exist within the context
     *
     * Based on nodesInSubContexts
     *
     * @return
     */
    std::set<int> partitionsInContext();

    std::map<int, std::shared_ptr<ContextFamilyContainer>> getContextFamilyContainers() const;
    void setContextFamilyContainers(const std::map<int, std::shared_ptr<ContextFamilyContainer>> &contextFamilyContainers);

    std::map<int, std::vector<std::shared_ptr<Arc>>> getContextDriversPerPartition() const;

    void
    setContextDriversPerPartition(const std::map<int, std::vector<std::shared_ptr<Arc>>> &contextDriversPerPartition);

    /**
     * @brief Gets the context driver arc for a particular partition
     * @param partitionNum
     * @return The context driver arc going to partition=parititionNum.  If no such arc exists, an empty vector is returned
     */
    std::vector<std::shared_ptr<Arc>> getContextDriversForPartition(int partitionNum) const;

    /**
     * @brief Add a set of context driver arcs to the ContextDriversForPartition map
     * @param drivers the drivers to add
     * @param partitionNum the partition to add to
     */
    void addContextDriverArcsForPartition(std::vector<std::shared_ptr<Arc>> drivers, int partitionNum);

    //==== Emit Functions ====

    /**
     * @brief Returns a vector of variables that need to be declared outside of the context(s) created by this node
     * @return vector of variables which need to be declared before contexts are emitted
     */
    virtual std::vector<Variable> getCContextVars() = 0;

    /**
     * @brief Reports if the context root requires that all of the subContexts be emitted in one contiguous block.
     *
     * For ex. this would be true for 'switch' statements but could be false for 'if/else if/else' statements as
     * they can be split as long as the 'if' statment is used again once returning to this context family.
     *
     * @return True if the subContexts need to be emitted in a single contiguous block, false otherwise
     */
    virtual bool requiresContiguousContextEmits() = 0;

    /**
     * @brief Get the specified Context Variable
     * @param contextVarIndex the index of the context variable
     * @return the specified Context Variable
     */
    virtual Variable getCContextVar(int contextVarIndex) = 0;

    /**
     * @brief Create the corresponding ContextVariableUpdate nodes and insert them into the graph.
     *
     * @note This function should typically be called after context discovery and context expansion.  This is because
     * the StateUpdate generally inherits
     *
     * @param new_nodes a vector of nodes added to the design.  This includes the new ContextVariableUpdate nodes
     * @param deleted_nodes a vector of nodes to be deleted in the design
     * @param new_arcs a vector of new arcs added to the design
     * @param deleted_arcs a vector of arcs to be deleted from the design
     * @param setContext if true, sets the context of the created ContextVariableUpdate nodes, otherwise does not
     *
     * @return true if the ContextVariableUpdate nodes were created and inserted into the graph, false if not
     */
    virtual bool createContextVariableUpdateNodes(std::vector<std::shared_ptr<Node>> &new_nodes,
                                                  std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                  std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                  std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                  bool setContext = false);

    /**
     * @brief Emits the statement to open a context.  This function should be called the first time a context in this family is emitted (ie. when an 'if' or 'switch' statement is used)
     * @param cStatementQueue The current cStatement queue.  Will be modified by the call to this function
     * @param schedType the scheduler used (parameter may not be used unless the C emit for the given node is different depending on the scheduler used - ex. if the scheduler is context aware)
     * @param subContextNumber the sub-context being emitted
     * @param partitionNum the partition under which this context is being opened
     */
    virtual void emitCContextOpenFirst(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber, int partitionNum) = 0;

    /**
     * @brief Emits the statement to open a context.  This function should be called subsequent times contexts in this family are emitted (ie. when an 'else if' statement is used)
     * @param cStatementQueue The current cStatement queue.  Will be modified by the call to this function
     * @param schedType the scheduler used (parameter may not be used unless the C emit for the given node is different depending on the scheduler used - ex. if the scheduler is context aware)
     * @param subContextNumber the sub-context being emitted
     */
    virtual void emitCContextOpenMid(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber, int partitionNum) = 0;

    /**
     * @brief Emits the statement to open a context.  This function should be called when the last context in this family is emitted (ie. when an 'else' or 'default' statement is used)
     * @param cStatementQueue The current cStatement queue.  Will be modified by the call to this function
     * @param schedType the scheduler used (parameter may not be used unless the C emit for the given node is different depending on the scheduler used - ex. if the scheduler is context aware)
     * @param subContextNumber the sub-context being emitted
     */
    virtual void emitCContextOpenLast(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber, int partitionNum) = 0;

    /**
     * @brief Emits the statement to close a context.  This function should be called the first time a context in this family is closed (ie. when an 'if' or 'switch' statement is used)
     * @param cStatementQueue The current cStatement queue.  Will be modified by the call to this function
     * @param schedType the scheduler used (parameter may not be used unless the C emit for the given node is different depending on the scheduler used - ex. if the scheduler is context aware)
     * @param subContextNumber the sub-context being emitted
     */
    virtual void emitCContextCloseFirst(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber, int partitionNumber) = 0;

    /**
     * @brief Emits the statement to close a context.  This function should be called subsequent times contexts in this family are closed (ie. when an 'else if' statement is used)
     * @param cStatementQueue The current cStatement queue.  Will be modified by the call to this function
     * @param schedType the scheduler used (parameter may not be used unless the C emit for the given node is different depending on the scheduler used - ex. if the scheduler is context aware)
     * @param subContextNumber the sub-context being emitted
     */
    virtual void emitCContextCloseMid(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber, int partitionNum) = 0;

    /**
     * @brief Emits the statement to open a context.  This function should be called when the last context in this family is emitted (ie. when an 'else' or 'default' statement is used)
     * @param cStatementQueue The current cStatement queue.  Will be modified by the call to this function
     * @param schedType the scheduler used (parameter may not be used unless the C emit for the given node is different depending on the scheduler used - ex. if the scheduler is context aware)
     * @param subContextNumber the sub-context being emitted
     */
    virtual void emitCContextCloseLast(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber, int partitionNum) = 0;

    /**
     * @brief Signals if FIFO absorptions in this context are legal
     * @return
     */
    virtual bool allowFIFOAbsorption();
};

/*! @} */

#endif //VITIS_CONTEXTROOT_H
