//
// Created by Christopher Yarp on 6/25/18.
//

#ifndef VITIS_SUBSYSTEM_H
#define VITIS_SUBSYSTEM_H

#include <set>
#include <memory>
#include "Node.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Represents a Sub-system within the flow graph.
 *
 * While sub-systems are Nodes, they also can contain children.  Typically arcs will not terminate at a subsystem unless
 * there is a specific emit function for that subsystem.
 */
class SubSystem : public Node{
friend class NodeFactory;

protected:
    std::set<std::shared_ptr<Node>> children; ///< Nodes contained within this sub-system

    //==== Constructors ====
    /**
     * @brief Default constructor.  Vector initialized using default behavior.
     */
    SubSystem();

    /**
     * @brief Construct SubSystem with given parent node.  Calls Node constructor.
     */
    SubSystem(std::shared_ptr<SubSystem> parent);

    /**
     * @brief Emits the subgraph entry for this subsystem as well as calling the emitGraphML functions on each child node
     *
     * Was broken into a seperate helper function so the logic could be re-used in enabled subsystem
     *
     * @param doc the XML document thisNode resides in
     * @param thisNode the XML DOMElement node corresponding to this node
     * @return a pointer to the new XML subgraph node
     */
    xercesc::DOMElement* emitGramphMLSubgraphAndChildren(xercesc::DOMDocument* doc, xercesc::DOMElement* thisNode);

public:

    //==== Functions ====
    /**
     * @brief Adds a child of this node to the children list
     * @param child The child to add
     */
    void addChild(std::shared_ptr<Node> child);

    /**
     * @brief Removes a child from this node's list of children
     * @param child The child to remove
     */
    void removeChild(std::shared_ptr<Node> child);

    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

};

/*@}*/

#endif //VITIS_SUBSYSTEM_H
