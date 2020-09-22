//
// Created by Christopher Yarp on 6/2/20.
//

#ifndef VITIS_SIMULINKBITSHIFT_H
#define VITIS_SIMULINKBITSHIFT_H

#include "MediumLevelNode.h"
#include "GraphCore/NodeFactory.h"

/**
 * \addtogroup MediumLevelNodes Medium Level Nodes
 * @{
*/

/**
 * @brief This block represents a bit shift
 */
class SimulinkBitShift : public MediumLevelNode {
friend NodeFactory;

public:
    enum class ShiftMode{
        SHIFT_LEFT_LOGICAL, ///<Zero fills when shifting right whether signed or unsigned.  Basically, the equivalent of << in C
        SHIFT_RIGHT_LOGICAL, ///<Zero fills when shifting left whether signed or unsigned.  Basically forces a re-interpret cast to unsigned if the input is signed
        SHIFT_RIGHT_ARITHMETIC ///<For unsigned, behaves same as shift right logical. For signed, replicates the MSB.  Basically, the equivalent of >> in C which changes behavior depending on the type
    };

    /**
     * @brief Parses the shift mode from either the Vitis Dialect or Simulink
     */
    static ShiftMode parseShiftMode(std::string str);

    static std::string shiftModeToString(ShiftMode mode);

private:
    ShiftMode shiftMode; ///<The shift mode
    int shiftAmt; ///<The shift amount

    //==== Constructors ====
    /**
     * @brief Constructs a SimulinkBitShift node with no value
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    SimulinkBitShift();

    /**
     * @brief Constructs a SimulinkBitShift node with no value and a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit SimulinkBitShift(std::shared_ptr<SubSystem> parent);

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
    SimulinkBitShift(std::shared_ptr<SubSystem> parent, SimulinkBitShift* orig);

public:
    //==== Getters/Setters ====
    ShiftMode getShiftMode() const;
    void setShiftMode(ShiftMode shiftMode);
    int getShiftAmt() const;
    void setShiftAmt(int shiftAmt);

    //==== Factories ====
    /**
     * @brief Creates a SimulinkBitShift node from a GraphML Description
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
    static std::shared_ptr<SimulinkBitShift> createFromGraphML(int id, std::string name,
                                                   std::map<std::string, std::string> dataKeyValueMap,
                                                   std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Expand ====
    /**
     * @brief Expands the SimulinkBitShift block into BitwiseOperator blocks and possibly ReinterpretCast
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

#endif //VITIS_SIMULINKBITSHIFT_H
