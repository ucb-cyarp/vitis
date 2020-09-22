//
// Created by Christopher Yarp on 5/2/20.
//

#ifndef VITIS_WRAPPINGCOUNTER_H
#define VITIS_WRAPPINGCOUNTER_H

#include "PrimitiveNode.h"
#include "GraphCore/NodeFactory.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 * @{
*/

/**
 * @brief Represent an unconditional counter that counts up to a specified value then wraps back to 0
 *
 * @note This is currently a primitive node to support context driver replication for DownsampleClockDomains
 */
class WrappingCounter : public PrimitiveNode{
    friend NodeFactory;

private:
    int countTo; ///<The value to count up to.  This value is never reached and serves as the modulus for the counter
    int initCondition; ///<The initial state of the counter
    Variable cStateVar; ///<The C variable storing the state of the counter
    //There is no need to have an input variable since the counter increments unconditionally by 1

    //==== Constructors ====
    /**
     * @brief Constructs an empty WrappingCounter node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    WrappingCounter();

    /**
     * @brief Constructs an empty WrappingCounter node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit WrappingCounter(std::shared_ptr<SubSystem> parent);

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
    WrappingCounter(std::shared_ptr<SubSystem> parent, WrappingCounter* orig);

public:
    //====Getters/Setters====
    int getCountTo() const;
    void setCountTo(int countTo);
    int getInitCondition() const;
    void setInitCondition(int initCondition);
    Variable getCStateVar() const;
    void setCStateVar(const Variable &cStateVar);

    //====Factories====
    /**
     * @brief Creates a delay node from a GraphML Description
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
    static std::shared_ptr<WrappingCounter> createFromGraphML(int id, std::string name,
                                                    std::map<std::string, std::string> dataKeyValueMap,
                                                    std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type) override ;

    std::string typeNameStr() override;

    std::string labelStr() override ;

    void validate() override;

    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) override;

    bool hasState() override;

    bool hasCombinationalPath() override;

    std::vector<Variable> getCStateVars() override;

    void emitCExprNextState(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType) override;

    void emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, std::shared_ptr<StateUpdate> stateUpdateSrc) override;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    /**
     * @brief Creates a the StateUpdate node for the Delay
     *
     * Creates the state update for the WrappingCounter.
     *
     * The StateUpdate Node is dependent on:
     *   - Each node dependent on the output of the WrappingCounter (order constraint only)
     *
     */
    bool createStateUpdateNode(std::vector<std::shared_ptr<Node>> &new_nodes,
                               std::vector<std::shared_ptr<Node>> &deleted_nodes,
                               std::vector<std::shared_ptr<Arc>> &new_arcs,
                               std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                               bool includeContext) override;

};

/*! @} */

#endif //VITIS_WRAPPINGCOUNTER_H
