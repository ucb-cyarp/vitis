//
// Created by Christopher Yarp on 2019-01-28.
//

#ifndef VITIS_SIMULINKCODERFSM_H
#define VITIS_SIMULINKCODERFSM_H

#include "BlackBox.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 */
/*@{*/

/**
 * @brief This handles the special case of importing Simulink Coder Generated Stateflow FSMs
 *
 * @note Stateflow does not create a seperate state update function - it is all contained inside of a single function.
 * To handle this, we declare this blackbox as stateful but put in an empty string for the stateUpdate name.
 * We also modify the stateUpdate wiring to not create state update nodes for this block.
 */
class SimulinkCoderFSM : public BlackBox {
friend NodeFactory;

private:
    //In Stateflow's C generation, the initialize function appears to be the same as the reset
    std::string inputsStructName; ///<The current method of Simulink Coder export contains inputs to the function in an external structure.  This is the name of that structure.
    std::string outputsStructName; ///<The current method of Simulink Coder export contains outputs of the function in an external structure.  This is the name of that structure.
    std::string stateStructName; ///<The current method of Simulink Coder export contains the state of the function in an external structure.  This is the name of that structure.

    //==== Constructors ====
    /**
     * @brief Constructs an empty SimulinkCoderFSM node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    SimulinkCoderFSM();

    /**
     * @brief Constructs an empty SimulinkCoderFSM node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit SimulinkCoderFSM(std::shared_ptr<SubSystem> parent);

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
    SimulinkCoderFSM(std::shared_ptr<SubSystem> parent, BlackBox* orig);

public:
    std::string getInputsStructName() const;
    void setInputsStructName(const std::string &inputsStructName);

    std::string getOutputsStructName() const;
    void setOutputsStructName(const std::string &outputsStructName);

    std::string getStateStructName() const;
    void setStateStructName(const std::string &stateStructName);

    //====Factories====
    /**
     * @brief Creates a Simulink Coder Stateflow node from a GraphML Description
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
    static std::shared_ptr<BlackBox> createFromGraphML(int id, std::string name,
                                                       std::map<std::string, std::string> dataKeyValueMap,
                                                       std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::set<GraphMLParameter> graphMLParameters() override;



};

/*@}*/


#endif //VITIS_SIMULINKCODERFSM_H
