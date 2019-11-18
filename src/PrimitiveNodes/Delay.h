//
// Created by Christopher Yarp on 7/6/18.
//

#ifndef VITIS_DELAY_H
#define VITIS_DELAY_H

#include <vector>
#include <memory>
#include <map>
#include <string>
#include <complex>

#include "PrimitiveNode.h"
#include "GraphCore/NumericValue.h"
#include "GraphMLTools/GraphMLDialect.h"
#include "GraphCore/StateUpdate.h"


/**
 * \addtogroup PrimitiveNodes Primitives
 * @{
*/

/**
 * @brief Represents a Delay (z^-1) Block
 */
class Delay : public PrimitiveNode{
    friend NodeFactory;
private:
    int delayValue; ///<The amount of delay in this node
    std::vector<NumericValue> initCondition; ///<The Initial condition of this delay.  Length must match the delay value.  The initial condition that will be presented first is at index 0
    Variable cStateVar; ///<The C variable storing the state of the dela
    Variable cStateInputVar; ///<the C temporary variable holding the input to state before the update
    //TODO: Re-evaluate if numeric value should be stored as double/complex double (like Matlab does).  An advantage to providing a class that can contain both is that there is less risk of an int being store improperly and full 64 bit integers can be represented.
    //TODO: Advantage of storing std::complex is that each element is smaller (does not need to allocate both a double and int version)

    //==== Constructors ====
    /**
     * @brief Constructs an empty delay node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    Delay();

    /**
     * @brief Constructs an empty delay node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit Delay(std::shared_ptr<SubSystem> parent);

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
    Delay(std::shared_ptr<SubSystem> parent, Delay* orig);

public:
    //====Getters/Setters====
    int getDelayValue() const;
    void setDelayValue(int delayValue);
    std::vector<NumericValue> getInitCondition() const;
    void setInitCondition(const std::vector<NumericValue> &initCondition);

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
    static std::shared_ptr<Delay> createFromGraphML(int id, std::string name,
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

    void emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType) override;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    /**
     * @brief Creates a the StateUpdate node for the Delay
     *
     * Creates the state update for the node.
     *
     * The StateUpdate Node is dependent on:
     *   - Next state calculated (calculated when Delay node scheduled) -> order constraint on delay node
     *      - The next state is stored in a temporary.  This would be redundant (the preceding operation should produce
     *        a single assignmnent to a temporary) except for the case when there is another state element.  In that case
     *        a temporary variable is not
     *   - Each node dependent on the output of the delay (order constraint only)
     *
     */
    bool createStateUpdateNode(std::vector<std::shared_ptr<Node>> &new_nodes,
                               std::vector<std::shared_ptr<Node>> &deleted_nodes,
                               std::vector<std::shared_ptr<Arc>> &new_arcs,
                               std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                               bool includeContext) override;

};

/*! @} */


#endif //VITIS_DELAY_H
