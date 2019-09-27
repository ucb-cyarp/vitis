//
// Created by Christopher Yarp on 10/22/18.
//

#ifndef VITIS_CONTEXTFAMILYCONTAINER_H
#define VITIS_CONTEXTFAMILYCONTAINER_H

#include "SubSystem.h"
#include "ContextContainer.h"

/**
 * \addtogroup GraphCore Graph Core
 * @{
*/

/**
 * @brief Container for context containers that should be scheduled with some order
 *
 * This is not considered a standard part of the design hierarchy but is used while scheduling
 *
 * @note Contexts can have multiple ContextFamilyContainers, one for each partition the context resides in
 *
 * A different ContextFamilyContainer exists for each partition because scheduling is handled seperatly for each partition
 */
class ContextFamilyContainer : public SubSystem {
    friend NodeFactory;

private:
    std::vector<std::shared_ptr<ContextContainer>> subContextContainers; ///<An ordered list of context containers, one for each subcontext.  SubContextContainer should also be children
    std::shared_ptr<ContextRoot> contextRoot; ///<The node creating this ContextFamily (should be a child after enacapsulation has completed)
    std::map<int, std::shared_ptr<ContextFamilyContainer>> siblingContainers;

    /**
     * @brief Default constructor.  Vector initialized using default behavior.
     */
    ContextFamilyContainer();

    /**
     * @brief Construct SubSystem with given parent node.  Calls Node constructor.
     */
    explicit ContextFamilyContainer(std::shared_ptr<SubSystem> parent);

    /**
     * @brief Constructs a new node with a shallow copy of parameters from the original node.  Ports are not copied and neither is the parent reference.  Children are also not copied.  This node is not added to the children list of the parent.
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
    ContextFamilyContainer(std::shared_ptr<SubSystem> parent, ContextFamilyContainer* orig);

public:
    std::vector<std::shared_ptr<ContextContainer>> getSubContextContainers() const;
    void setSubContextContainers(const std::vector<std::shared_ptr<ContextContainer>>& subContextContainers);

    std::shared_ptr<ContextRoot> getContextRoot() const;
    void setContextRoot(const std::shared_ptr<ContextRoot>&contextRoot);

    const std::map<int, std::shared_ptr<ContextFamilyContainer>> getSiblingContainers() const;
    void setSiblingContainers(const std::map<int, std::shared_ptr<ContextFamilyContainer>> &siblingContainers);

    /**
     * @brief Get a pointer to the sub-context container specified
     * @param subContext
     * @return
     */
    std::shared_ptr<ContextContainer> getSubContextContainer(unsigned long subContext);

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;
    void shallowCloneWithChildren(std::shared_ptr<SubSystem> parent, std::vector<std::shared_ptr<Node>> &nodeCopies, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode) override;

    std::set<GraphMLParameter> graphMLParameters() override;
    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    /**
     * @brief For nodes within this context family which are connected to nodes outside of the context family, the arcs are re-wired to terminate/origionate at the ContextFamilyContainer
     *
     * This is used so that the ContextFamilyContainer is scheduled only when all nodes within it have their pre-reqs scheduled
     *
     * This also allows all of the out arcs to be simultanously cleared when the context is done being scheduled.
     *
     * If a ContextFamilyContainer is encountered in the children list, this function is called recursivley on it.  After the recursive call, any arc connected to that ContextFamilyContainer is evaluated to determine if it should be rewired again.  This handles the case of n input traversing through muliple contexts before being used.
     *
     * @note This function only rewires arcs to/from nodes that are direct children of this context family.  Nodes in nested contexts are rewired by their parent ContextFamilyContainers
     *
     * @note This function should be called on the top level ContextFamilyContainers
     *
     * @warning This function should be called after ContextFamilyContainers and ContextContainers are properly encapsulated with their child nodes
     */
    void rewireArcsToContextFamilyContainerAndRecurse(std::vector<std::shared_ptr<Arc>> &arcs_to_delete);
};

/*! @} */

#endif //VITIS_CONTEXTFAMILYCONTAINER_H
