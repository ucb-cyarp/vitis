//
// Created by Christopher Yarp on 10/23/18.
//

#ifndef VITIS_CONTEXTROOT_H
#define VITIS_CONTEXTROOT_H

#include <memory>
#include <vector>

#include "Node.h"

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

    //TODO: Remove if another virtual method is every created
    /**
     * @brief Need a virtual method to allow RTTI for dynamic cast.
     */
    virtual void dummy();

};

/*@}*/

#endif //VITIS_CONTEXTROOT_H
