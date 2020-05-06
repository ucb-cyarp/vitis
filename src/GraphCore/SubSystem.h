//
// Created by Christopher Yarp on 6/25/18.
//

#ifndef VITIS_SUBSYSTEM_H
#define VITIS_SUBSYSTEM_H

#include <set>
#include <memory>
#include "Node.h"

class EnabledSubSystem;
class ClockDomain;

/**
 * \addtogroup GraphCore Graph Core
 * @{
*/

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
    explicit SubSystem(std::shared_ptr<SubSystem> parent);

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
    SubSystem(std::shared_ptr<SubSystem> parent, SubSystem* orig);

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

    std::set<std::shared_ptr<Node>> getChildren() const;
    void setChildren(const std::set<std::shared_ptr<Node>>& children);

    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string typeNameStr() override;

    std::string labelStr() override ;

    bool canExpand() override;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    void shallowCloneWithChildren(std::shared_ptr<SubSystem> parent, std::vector<std::shared_ptr<Node>> &nodeCopies, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode) override;

    /**
     * @brief Extends the context of enabled subsystems contained within this subsystem.
     *
     * Extends hierarchically, starting at this level
     *
     * @param new_nodes A vector which will be filled with the new nodes created during expansion
     * @param deleted_nodes A vector which will be filled with the nodes deleted during expansion
     * @param new_arcs A vector which will be filled with the new arcs created during expansion
     * @param deleted_arcs A vector which will be filled with the arcs deleted during expansion
     */
    virtual void extendEnabledSubsystemContext(std::vector<std::shared_ptr<Node>> &new_nodes,
                                               std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                               std::vector<std::shared_ptr<Arc>> &new_arcs,
                                               std::vector<std::shared_ptr<Arc>> &deleted_arcs);

    /**
     * @brief Marks all the nodes under this subsystem (including at enabled subsystems and muxes), though marking them) with the given context stack.  Also finds muxes within this subsystem (and below).
     *
     * @note Does not recurse into enabled subsystems
     *
     * @param contextStack The context stack at this point.  Nodes under this subsystem
     * @param discoveredMux a vector modified to include discovered muxes
     * @param discoveredEnabledSubSystems a vector modified to include discovered EnabledSubsystems
     * @param discoveredClockDomains a vector modified to include discovered ClockDomains
     * @param discoveredGeneral a vector modified to include discovered general nodes
     */
    void discoverAndUpdateContexts(std::vector<Context> contextStack, std::vector<std::shared_ptr<Mux>> &discoveredMux,
                                   std::vector<std::shared_ptr<EnabledSubSystem>> &discoveredEnabledSubSystems,
                                   std::vector<std::shared_ptr<ClockDomain>> &discoveredClockDomains,
                                   std::vector<std::shared_ptr<Node>> &discoveredGeneral);

    /**
     * @brief Order Constrains Zero Input Nodes Within the Subsystems to be scheduled after all the nodes in the predecessorNodes list
     *
     * This function is called recursivly into each subsystem
     *
     * Is overwritten by EnabledSubsystem to add its own enable driver nodes to the list of predecessor Nodes
     *
     * @deprecated This has since been deprictaed after the introduction of multiple clock domains
     *
     * @param predecessorNodes The list of predecessor nodes from which to add order constraint arcs to
     * @param new_nodes
     * @param deleted_nodes
     * @param new_arcs
     * @param deleted_arcs
     */
    virtual void orderConstrainZeroInputNodes(std::vector<std::shared_ptr<Node>> predecessorNodes,
                                              std::vector<std::shared_ptr<Node>> &new_nodes,
                                              std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                              std::vector<std::shared_ptr<Arc>> &new_arcs,
                                              std::vector<std::shared_ptr<Arc>> &deleted_arcs);
};

/*! @} */

#endif //VITIS_SUBSYSTEM_H
