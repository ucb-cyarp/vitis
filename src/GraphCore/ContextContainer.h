//
// Created by Christopher Yarp on 10/22/18.
//

#ifndef VITIS_CONTEXTCONTAINER_H
#define VITIS_CONTEXTCONTAINER_H

#include <memory>
#include <vector>
#include "SubSystem.h"
#include "Context.h"
#include "NodeFactory.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief A container class used to contain nodes within a context.
 *
 * This container is typically not conisdered part of the design hierarchy.
 * It is used, however, when performing actions that need to keep
 */
class ContextContainer : public SubSystem {
    friend class NodeFactory;

private:
    Context containerContext; ///<The context corresponding to this context container
    //For nodes that have multiple contexts (ex. muxes), a context container is created for each context
    //The contexts are contained in a ContextFamilyContainer

    //Can construct context container hierarchy by looking at the top level ContextRoot nodes and then following nodes within their contexts (finding ContextRoot nodes within their contexts)

    /**
     * @brief Default constructor.  Vector initialized using default behavior.
     */
    ContextContainer();

    /**
     * @brief Construct SubSystem with given parent node.  Calls Node constructor.
     */
    explicit ContextContainer(std::shared_ptr<SubSystem> parent);

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
    ContextContainer(std::shared_ptr<SubSystem> parent, ContextContainer* orig);

public:
    Context getContainerContext() const;

    void setContainerContext(const Context &context);

    std::set<GraphMLParameter> graphMLParameters() override;
    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    /**
     * @brief Find the set of state nodes within this context (but not in nested contexts_)
     * @return the set of state nodes within this context
     */
    std::vector<std::shared_ptr<Node>> findStateNodesInContext();

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

};

/*@}*/

#endif //VITIS_CONTEXTCONTAINER_H
