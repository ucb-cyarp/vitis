//
// Created by Christopher Yarp on 7/16/18.
//

#ifndef VITIS_GAIN_H
#define VITIS_GAIN_H

#include "MediumLevelNode.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/SubSystem.h"
#include "GraphCore/NumericValue.h"
#include "GraphMLTools/GraphMLDialect.h"

#include <vector>
#include <map>
#include <string>

/**
 * \addtogroup MediumLevelNodes Medium Level Nodes
 */
/*@{*/

/**
 * @brief Represents a gain (multiply by constant) block
 */
class Gain : public MediumLevelNode{
    friend NodeFactory;

private:
    std::vector<NumericValue> gain; ///<The gain to multiply the signal by (may be a vector)

    //==== Constructors ====
    /**
     * @brief Constructs a gain node with no value
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    Gain();

    /**
     * @brief Constructs a gain node with no value and a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit Gain(std::shared_ptr<SubSystem> parent);

public:
    //==== Getters/Setters ====
    std::vector<NumericValue> getGain() const;
    void setGain(const std::vector<NumericValue> &gain);

    //==== Factories ====
    /**
     * @brief Creates a gain node from a GraphML Description
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
    static std::shared_ptr<Gain> createFromGraphML(int id, std::string name,
                                                   std::map<std::string, std::string> dataKeyValueMap,
                                                   std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Expand ====
    /**
     * @brief Expands the gain block into a multiply block and a constant block.
     *
     * Validates before expansion to check assumptions are fulfilled.
     *
     * The type of the constant is determined in the following manner:
     *   - If output type is a floating point type, the constant takes on the same type as the output
     *   - If the output is an integer type, the constant takes the smallest integer type which accommodates the constant.
     *   - If the output is a fixed point type, the constant takes on a fixed point type with the integer portion is the smallest that supports the given constant
     *     the number of fractional bits are the max(fractional bits in output, fractional bits in 1st input)
     *
     *   Complexity and width are taken from the gain NumericValue
     */
    bool expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes, std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs) override ;

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    void validate() override ;

};

/*@}*/

#endif //VITIS_GAIN_H
