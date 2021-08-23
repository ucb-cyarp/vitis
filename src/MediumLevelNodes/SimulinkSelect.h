//
// Created by Christopher Yarp on 8/2/20.
//

#ifndef VITIS_SIMULINKSELECT_H
#define VITIS_SIMULINKSELECT_H

#include "MediumLevelNode.h"
#include "GraphCore/NodeFactory.h"
#include "PrimitiveNodes/Select.h"
#include "GraphCore/NumericValue.h"

/**
 * \addtogroup MediumLevelNodes Medium Level Nodes
 * @{
*/

/**
 * @brief Represents the Simulink Selector block.  Is expanded to a vitis Select block and possibly Constant and Sum blocks
 *
 * @note Currently only supports vector inputs
 */
class SimulinkSelect : public MediumLevelNode{
    friend NodeFactory;
private:
    bool oneBased; ///<If true, indexes are 1 based instead of 0 based
    std::vector<Select::SelectMode> modes; ///<The indexing mode (either a vector of indexes or an offset and a vector length)
    std::vector<std::vector<NumericValue>> dialogIndexes; ///<The indexs (or offset) from the dialog.  Only relevant if index comes from dialog.
    std::vector<int> outputLens; ///<The output length when the mode is offset/length
    std::vector<bool> fromDialog;///<If true, the indexes come from the dialog parameter, otherwise, they come from the second input

    //outputSizeArray and inputPortWidth is also currently not needed as this information is stored in the connected arcs
    //Also, we are restricting to vectors at the moment

    //==== Constructors ====
    /**
     * @brief Constructs a SimulinkSelect node with no value
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    SimulinkSelect();

    /**
     * @brief Constructs a SimulinkSelect node with no value and a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit SimulinkSelect(std::shared_ptr<SubSystem> parent);

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
    SimulinkSelect(std::shared_ptr<SubSystem> parent, SimulinkSelect* orig);

public:
    //==== Getters/Setters ====
    bool isOneBased() const;
    void setOneBased(bool oneBased);
    std::vector<Select::SelectMode> getModes() const;
    void setModes(const std::vector<Select::SelectMode> &modes);
    std::vector<std::vector<NumericValue>> getDialogIndexes() const;
    void setDialogIndexes(const std::vector<std::vector<NumericValue>> &dialogIndexes);
    std::vector<int> getOutputLens() const;
    void setOutputLens(const std::vector<int> &outputLens);
    std::vector<bool> getFromDialog() const;
    void setFromDialog(const std::vector<bool> &fromDialog);

    //==== Factories ====
    /**
     * @brief Creates a SimulinkSelect node from a GraphML Description
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
    static std::shared_ptr<SimulinkSelect> createFromGraphML(int id, std::string name,
                                                   std::map<std::string, std::string> dataKeyValueMap,
                                                   std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Expand ====
    /**
     * @brief Expands the SimulinkSelect block into a multiply block and a constant block.
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

#endif //VITIS_SIMULINKSELECT_H
