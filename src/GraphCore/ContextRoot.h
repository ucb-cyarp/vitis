//
// Created by Christopher Yarp on 10/23/18.
//

#ifndef VITIS_CONTEXTROOT_H
#define VITIS_CONTEXTROOT_H

#include <memory>
#include <vector>

#include "Node.h"
#include "Variable.h"

//class Node;

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief A class containing information and helper methods for nodes that create contexts
 */
class ContextRoot {
private:
    std::vector<std::vector<std::shared_ptr<Node>>> nodesInSubContexts; ///<A vector of nodes in the context
    int numSubContexts = 0;///<The number of sub-contexts created by this node

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
     * @brief Gets a vector of nodes in the specified sub-context vector
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

    int getNumSubContexts() const;

    void setNumSubContexts(int numSubContexts);

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
     * @brief Emits the statement to open a context.  This function should be called the first time a context in this family is emitted (ie. when an 'if' or 'switch' statement is used)
     * @param cStatementQueue The current cStatement queue.  Will be modified by the call to this function
     * @param subContextNumber the sub-context being emitted
     */
    virtual void emitCContextOpenFirst(std::vector<std::string> &cStatementQueue, int subContextNumber) = 0;

    /**
     * @brief Emits the statement to open a context.  This function should be called subsequent times contexts in this family are emitted (ie. when an 'else if' statement is used)
     * @param cStatementQueue The current cStatement queue.  Will be modified by the call to this function
     * @param subContextNumber the sub-context being emitted
     */
    virtual void emitCContextOpenMid(std::vector<std::string> &cStatementQueue, int subContextNumber) = 0;

    /**
     * @brief Emits the statement to open a context.  This function should be called when the last context in this family is emitted (ie. when an 'else' or 'default' statement is used)
     * @param cStatementQueue The current cStatement queue.  Will be modified by the call to this function
     * @param subContextNumber the sub-context being emitted
     */
    virtual void emitCContextOpenLast(std::vector<std::string> &cStatementQueue, int subContextNumber) = 0;

    /**
     * @brief Emits the statement to close a context.  This function should be called the first time a context in this family is closed (ie. when an 'if' or 'switch' statement is used)
     * @param cStatementQueue The current cStatement queue.  Will be modified by the call to this function
     * @param subContextNumber the sub-context being emitted
     */
    virtual void emitCContextCloseFirst(std::vector<std::string> &cStatementQueue, int subContextNumber) = 0;

    /**
     * @brief Emits the statement to close a context.  This function should be called subsequent times contexts in this family are closed (ie. when an 'else if' statement is used)
     * @param cStatementQueue The current cStatement queue.  Will be modified by the call to this function
     * @param subContextNumber the sub-context being emitted
     */
    virtual void emitCContextCloseMid(std::vector<std::string> &cStatementQueue, int subContextNumber) = 0;

    /**
     * @brief Emits the statement to open a context.  This function should be called when the last context in this family is emitted (ie. when an 'else' or 'default' statement is used)
     * @param cStatementQueue The current cStatement queue.  Will be modified by the call to this function
     * @param subContextNumber the sub-context being emitted
     */
    virtual void emitCContextCloseLast(std::vector<std::string> &cStatementQueue, int subContextNumber) = 0;

};

/*@}*/

#endif //VITIS_CONTEXTROOT_H
