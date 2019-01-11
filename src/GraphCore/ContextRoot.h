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
//#include "ContextFamilyContainer.h"

//class Node;

class ContextFamilyContainer;

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief A class containing information and helper methods for nodes that create contexts
 */
class ContextRoot {
private:
    std::vector<std::vector<std::shared_ptr<Node>>> nodesInSubContexts; ///<A vector of nodes in the context (but not in sub-contexts)
    std::vector<std::shared_ptr<ContextVariableUpdate>> contextVariableUpdateNodes; ///<A list of ContextVariableUpdate nodes associated with this ContextRoot
    std::shared_ptr<ContextFamilyContainer> contextFamilyContainer; ///<The corresponding context family container (if it exists) for this ContextRoot

public:
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
     * @brief Gets a vector of nodes in the specified sub-context (but not in sub-contexts)
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

    /**
     * @brief Gets the arc(s) that drive the decision for the context
     * @return a vector of arcs that drive the decision for the context
     */
    virtual std::vector<std::shared_ptr<Arc>> getContextDecisionDriver() = 0;

    std::shared_ptr<ContextFamilyContainer> getContextFamilyContainer() const;
    void setContextFamilyContainer(const std::shared_ptr<ContextFamilyContainer> &contextFamilyContainer);

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
     */
    virtual void emitCContextOpenFirst(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber) = 0;

    /**
     * @brief Emits the statement to open a context.  This function should be called subsequent times contexts in this family are emitted (ie. when an 'else if' statement is used)
     * @param cStatementQueue The current cStatement queue.  Will be modified by the call to this function
     * @param schedType the scheduler used (parameter may not be used unless the C emit for the given node is different depending on the scheduler used - ex. if the scheduler is context aware)
     * @param subContextNumber the sub-context being emitted
     */
    virtual void emitCContextOpenMid(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber) = 0;

    /**
     * @brief Emits the statement to open a context.  This function should be called when the last context in this family is emitted (ie. when an 'else' or 'default' statement is used)
     * @param cStatementQueue The current cStatement queue.  Will be modified by the call to this function
     * @param schedType the scheduler used (parameter may not be used unless the C emit for the given node is different depending on the scheduler used - ex. if the scheduler is context aware)
     * @param subContextNumber the sub-context being emitted
     */
    virtual void emitCContextOpenLast(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber) = 0;

    /**
     * @brief Emits the statement to close a context.  This function should be called the first time a context in this family is closed (ie. when an 'if' or 'switch' statement is used)
     * @param cStatementQueue The current cStatement queue.  Will be modified by the call to this function
     * @param schedType the scheduler used (parameter may not be used unless the C emit for the given node is different depending on the scheduler used - ex. if the scheduler is context aware)
     * @param subContextNumber the sub-context being emitted
     */
    virtual void emitCContextCloseFirst(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber) = 0;

    /**
     * @brief Emits the statement to close a context.  This function should be called subsequent times contexts in this family are closed (ie. when an 'else if' statement is used)
     * @param cStatementQueue The current cStatement queue.  Will be modified by the call to this function
     * @param schedType the scheduler used (parameter may not be used unless the C emit for the given node is different depending on the scheduler used - ex. if the scheduler is context aware)
     * @param subContextNumber the sub-context being emitted
     */
    virtual void emitCContextCloseMid(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber) = 0;

    /**
     * @brief Emits the statement to open a context.  This function should be called when the last context in this family is emitted (ie. when an 'else' or 'default' statement is used)
     * @param cStatementQueue The current cStatement queue.  Will be modified by the call to this function
     * @param schedType the scheduler used (parameter may not be used unless the C emit for the given node is different depending on the scheduler used - ex. if the scheduler is context aware)
     * @param subContextNumber the sub-context being emitted
     */
    virtual void emitCContextCloseLast(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber) = 0;

};

/*@}*/

#endif //VITIS_CONTEXTROOT_H
