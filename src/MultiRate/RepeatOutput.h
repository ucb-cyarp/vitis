//
// Created by Christopher Yarp on 4/29/20.
//

#ifndef VITIS_REPEATOUTPUT_H
#define VITIS_REPEATOUTPUT_H

#include "Repeat.h"
#include "GraphCore/NodeFactory.h"

/**
 * \addtogroup MultiRate Multi-Rate Support Nodes
 * @{
*/

/**
 * @brief This represents a Repeat at the output of a DownsampleClockDomain
 *
 * It mirrors the general functionality of the EnableOutput in that it behaves like a latch
 */
class RepeatOutput : public Repeat {
friend class NodeFactory;

private:
    Variable stateVar; ///<The state variable for the latch
    std::vector<NumericValue> initCondition; ///<The Initial condition of this output port

protected:
    /**
     * @brief Default constructor
     */
    RepeatOutput();

    /**
     * @brief Construct a node with a given parent
     * @param parent parent of new node
     */
    explicit RepeatOutput(std::shared_ptr<SubSystem> parent);

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
    RepeatOutput(std::shared_ptr<SubSystem> parent, RepeatOutput* orig);

public:
    //==== Getters & Setters ====
    Variable getStateVar() const;
    void setStateVar(const Variable &stateVar);
    std::vector<NumericValue> getInitCondition() const;
    void setInitCondition(const std::vector<NumericValue> &initCondition);

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::set<GraphMLParameter> graphMLParameters() override;

    /**
     * @brief Creates a RepeatOutput node from a GraphML Description
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
    static std::shared_ptr<RepeatOutput> createFromGraphML(int id, std::string name,
                                                           std::map<std::string, std::string> dataKeyValueMap,
                                                           std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    std::string typeNameStr() override;

    std::string labelStr() override ;

    void validate() override;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    //Override hasState (it is like a transparent latch)
    bool hasState() override;

    //Override hasCombinationalPath (it is like a transparent latch)
    bool hasCombinationalPath() override;

    /**
     * @brief Creates a the StateUpdate node for the RepeatOutput
     *
     * Because the EnableOutput is a transparent latch, the update is placed between the input node to the RepeatOutput
     * and itself.  The update to the state occurs immediately when the input is calculated.  The emitCExprNextState
     * function does nothing for the EnableOutput.  The emitCExpr always returns the state element.
     *
     */
    bool createStateUpdateNode(std::vector<std::shared_ptr<Node>> &new_nodes,
                               std::vector<std::shared_ptr<Node>> &deleted_nodes,
                               std::vector<std::shared_ptr<Arc>> &new_arcs,
                               std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                               bool includeContext) override;

    /**
     * @brief Generate the C expression for the RepeatOutput
     *
     * This function simply returns the name of the state variable
     */
    virtual CExpr emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) override;

    /**
     * @brief Get a list of State Variables
     *
     * @note This function also calculate the parameter of the state vars.  It is done here because the node must already been connected and validated
     *
     * @return A list of State Variables
     */
    std::vector<Variable> getCStateVars() override;

    /**
     * @brief Sets the latch (state) from the calculated input
     */
    void emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, std::shared_ptr<StateUpdate> stateUpdateSrc) override;

    bool isSpecialized() override;

    /**
     * @brief Get the output variable (only used when operating in vector mode)
     * @return
     */
    Variable getVectorModeOutputVariable();

    std::vector<Variable> getVariablesToDeclareOutsideClockDomain() override;
};

/*! @} */

#endif //VITIS_REPEATOUTPUT_H
