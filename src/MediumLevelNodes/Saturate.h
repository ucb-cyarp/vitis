//
// Created by Christopher Yarp on 9/13/18.
//

#ifndef VITIS_SATURATE_H
#define VITIS_SATURATE_H

#include "MediumLevelNode.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/SubSystem.h"
#include "GraphCore/NumericValue.h"
#include "GraphMLTools/GraphMLDialect.h"
#include "MasterNodes/MasterUnconnected.h"

#include <vector>
#include <map>
#include <string>

/**
 * \addtogroup MediumLevelNodes Medium Level Nodes
 * @{
*/

/**
 * @brief Represents a saturate block
 *
 * Any input below the @ref lowerLimit is outputted as the lowerLimit.  Any input above the @ref upperLimit is outputted
 * as the upperLimit.  Any other input is simply passed.
 *
 * Seperate comparisons are made for the real and imag components if the input is complex
 *
 * This node is expanded to constants, comparisons, and switches
 */
class Saturate : public MediumLevelNode{
    friend NodeFactory;

private:
    NumericValue lowerLimit; ///< The lower limit of the input/output
    NumericValue upperLimit; ///< The uppper limit of the input/output

    //==== Constructors ====
    /**
     * @brief Constructs a Saturate node with no defined limits
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    Saturate();

    /**
     * @brief Constructs a Satrurate node with no defined limits and a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit Saturate(std::shared_ptr<SubSystem> parent);

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
    Saturate(std::shared_ptr<SubSystem> parent, Saturate* orig);

public:
    //==== Getters/Setters ====
    NumericValue getLowerLimit() const;
    void setLowerLimit(const NumericValue &lowerLimit);
    NumericValue getUpperLimit() const;
    void setUpperLimit(const NumericValue &upperLimit);

    //==== Factories ====
    /**
     * @brief Creates a saturate node from a GraphML Description
     *
     * @note This function does not add the node to the design or to the nodeID/pointer map
     *
     * @param id the ID number of the node
     * @param name the human readable name of a node
     * @param dataKeyValueMap A map of property keys and values extracted from the data nodes in the GraphML
     * @param parent The parent of this node in the hierarchy
     * @param dialect The dialect of the GraphML file being imported
     * @return a pointer to the new delay node
     */
    static std::shared_ptr<Saturate> createFromGraphML(int id, std::string name,
                                                   std::map<std::string, std::string> dataKeyValueMap,
                                                   std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Expand ====
    /**
     * @brief Expands the Saturate block into constant blocks, comparisons, and switches
     *
     * Seperate logic paths are defined for real and imagionary components
     *
     * Validates before expansion to check assumptions are fulfilled.
     */
    std::shared_ptr<ExpandedNode> expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                             std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                             std::shared_ptr<MasterUnconnected> &unconnected_master) override ;

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string typeNameStr() override;

    std::string labelStr() override ;

    void validate() override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;
};

/*! @} */

#endif //VITIS_SATURATE_H
