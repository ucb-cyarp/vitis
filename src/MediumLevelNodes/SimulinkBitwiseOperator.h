//
// Created by Christopher Yarp on 6/2/20.
//

#ifndef VITIS_SIMULINKBITWISEOPERATOR_H
#define VITIS_SIMULINKBITWISEOPERATOR_H

#include "MediumLevelNode.h"
#include "GraphCore/NodeFactory.h"
#include "PrimitiveNodes/BitwiseOperator.h"

/**
 * \addtogroup MediumLevelNodes Medium Level Nodes
 * @{
*/

/**
 * @brief Represents a bitwise operator from simulink
 */
class SimulinkBitwiseOperator : public MediumLevelNode{
friend NodeFactory;

private:
    BitwiseOperator::BitwiseOp bitwiseOp; ///<The bitwise operator to implement
    std::vector<NumericValue> mask; ///<The constant mask (if used)
    bool useMask; ///<If true, a constant mask is used, if false, a secondary input is used

    /**
     * @brief Constructs a SimulinkBitwiseOperator node with no value
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    SimulinkBitwiseOperator();

    /**
     * @brief Constructs a SimulinkBitwiseOperator node with no value and a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit SimulinkBitwiseOperator(std::shared_ptr<SubSystem> parent);

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
    SimulinkBitwiseOperator(std::shared_ptr<SubSystem> parent, SimulinkBitwiseOperator* orig);

public:
    //==== Getters/Setters ====
    BitwiseOperator::BitwiseOp getBitwiseOp() const;
    void setBitwiseOp(BitwiseOperator::BitwiseOp bitwiseOp);
    std::vector<NumericValue> getMask() const;
    void setMask(const std::vector<NumericValue> &mask);
    bool isUseMask() const;
    void setUseMask(bool useMask);

    //==== Factories ====
    /**
     * @brief Creates a SimulinkBitwiseOperator node from a GraphML Description
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
    static std::shared_ptr<SimulinkBitwiseOperator> createFromGraphML(int id, std::string name,
                                                   std::map<std::string, std::string> dataKeyValueMap,
                                                   std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Expand ====
    /**
     * @brief Expands the SimulinkBitwiseOperator block into a multiply block and a constant block.
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

#endif //VITIS_SIMULINKBITWISEOPERATOR_H
