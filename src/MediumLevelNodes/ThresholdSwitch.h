//
// Created by Christopher Yarp on 7/23/18.
//

#ifndef VITIS_THRESHOLDSWITCH_H
#define VITIS_THRESHOLDSWITCH_H

#include "MediumLevelNode.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/NumericValue.h"
#include "PrimitiveNodes/Compare.h"

/**
 * \addtogroup MediumLevelNodes Medium Level Nodes
 */
/*@{*/

/**
 * @brief Represents a Simulink style switch (2mux) where the select line is compared to a threshold
 *
 * Comparision take the form of (Select Line) compareOp (Threshold)
 * For example: (Select Line) > (Treshold)
 *
 * Is expanded to a mux and (optionally) a compare to constant block.
 */
class ThresholdSwitch : public MediumLevelNode{
    friend NodeFactory;

private:
    std::vector<NumericValue> threshold;
    Compare::CompareOp compareOp;

    //==== Constructors ====
    /**
     * @brief Constructs a threshold node with LT and no constant value
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    ThresholdSwitch();

    /**
     * @brief Constructs a gain node with LT, no constant value, and a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit ThresholdSwitch(std::shared_ptr<SubSystem> parent);

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
    ThresholdSwitch(std::shared_ptr<SubSystem> parent, ThresholdSwitch* orig);

public:
    //==== Getters/Setters ====
    std::vector<NumericValue> getThreshold() const;
    void setThreshold(const std::vector<NumericValue> &threshold);
    Compare::CompareOp getCompareOp() const;
    void setCompareOp(Compare::CompareOp compareOp);

    //==== Factories ====
    /**
     * @brief Creates a ThresholdSwitch node from a GraphML Description
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
    static std::shared_ptr<ThresholdSwitch> createFromGraphML(int id, std::string name,
                                                   std::map<std::string, std::string> dataKeyValueMap,
                                                   std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Expand ====
    /**
     * @brief Expands the ThresholdSwitch block into a mux block and, potentially, a CompareToConstant block.
     *
     * Validates before expansion to check assumptions are fulfilled.
     *
     * Expansion checks the type of the input.  If it is a integer, fixed, or floating point, a CompareToConstant
     * block is inserted (the threshold switch).  Otherwise, the select line is simply passed through.
     *
     * Simulink uses the following ports:
     *  - 0: passed when condition is true
     *  - 1: select line
     *  - 2: passed when condition is false
     *
     * @note In VITIS, it is assumed that 0 is false and any other number is true.  Typically, true is 1.
     *
     * @note The primitive mux node provided by vitis uses an index approach with 0 selecting the first input and
     * 1 selecting the second input.  To address this, and the convention that false is 0 and true is 1, the
     * order of the inputs are swapped in the expanded mux.
     */
    bool expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes, std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs) override ;

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    void validate() override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;
};


/*@}*/

#endif //VITIS_THRESHOLDSWITCH_H
