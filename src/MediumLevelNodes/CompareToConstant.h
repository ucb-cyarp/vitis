//
// Created by Christopher Yarp on 7/23/18.
//

#ifndef VITIS_COMPARETOCONSTANT_H
#define VITIS_COMPARETOCONSTANT_H

#include "MediumLevelNode.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/NumericValue.h"
#include "PrimitiveNodes/Compare.h"
#include "MasterNodes/MasterUnconnected.h"

#include <vector>

/**
 * \addtogroup MediumLevelNodes Medium Level Nodes
 * @{
*/

/**
 * @brief Represents a compare to constant block.
 *
 * Is expanded to a constant node and a compare node.
 *
 * Compares the signal at port 0 to the constant.  Port 0 is the left hand operator, the constant is the right:
 *
 * (Port 0) CompareOP (Constant)
 *
 * ex. (Port 0) < (Constant)
 */
class CompareToConstant : public MediumLevelNode {
    friend NodeFactory;

private:
    std::vector<NumericValue> compareConst; ///<The constant to compare the signal against.
    Compare::CompareOp compareOp; ///<The comparison operator

    /**
     * @brief Constructs a Compare to Constant node with operator LT and an empty constant
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    CompareToConstant();

    /**
     * @brief Constructs a Compare to Constant node with operator LT, an empty constant, and a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit CompareToConstant(std::shared_ptr<SubSystem> parent);

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
    CompareToConstant(std::shared_ptr<SubSystem> parent, CompareToConstant* orig);

public:
    //==== Getters/Setters ====
    std::vector<NumericValue> getCompareConst() const;
    void setCompareConst(const std::vector<NumericValue> &compareConst);
    Compare::CompareOp getCompareOp() const;
    void setCompareOp(Compare::CompareOp compareOp);

    //==== Factories ====
    /**
     * @brief Creates a CompareToConstant node from a GraphML Description
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
    static std::shared_ptr<CompareToConstant> createFromGraphML(int id, std::string name,
                                                   std::map<std::string, std::string> dataKeyValueMap,
                                                   std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Expand ====
    /**
     * @brief Expands the CompareToConstant block into a compare block and a constant block.
     *
     * Validates before expansion to check assumptions are fulfilled.
     *
     * The type of the constant is determined in the following manner:
     *   - If input type is a floating point type, the constant takes on the same type as the input
     *   - If the input is an integer type, the constant takes the smallest integer type which accommodates the constant.
     *   - If the input is a fixed point type, the constant takes on a fixed point type with the integer portion is the smallest that supports the given constant
     *     the number of fractional bits are the fractional bits in 1st input
     *
     *   Complexity and width are taken from the constant NumericValue
     */
    std::shared_ptr<ExpandedNode> expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                             std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                             std::shared_ptr<MasterUnconnected> &unconnected_master) override ;

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    void validate() override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

};

/*! @} */

#endif //VITIS_COMPARETOCONSTANT_H
