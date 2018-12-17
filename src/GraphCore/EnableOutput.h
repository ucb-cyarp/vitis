//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_ENABLEOUTPUT_H
#define VITIS_ENABLEOUTPUT_H

#include <memory>

#include "EnableNode.h"
#include "Node.h"
#include "SubSystem.h"
#include "Variable.h"
#include "GraphMLTools/GraphMLDialect.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Represents a special output port at the interface of an enabled subsystem.
 *
 * If not enabled, the previous value of this node is fed to the downstream logic.
 * If enabled, the calculated value is fed to the downstream logic as usual.
 */
class EnableOutput : public EnableNode{
friend class NodeFactory;

private:
    Variable stateVar; ///<The state variable for the latch
//    Variable nextStateVar; ///<The temporary variable for holding the next state for the latch
    std::vector<NumericValue> initCondition; ///<The Initial condition of this output port

protected:
    /**
     * @brief Default constructor
     */
    EnableOutput();

    /**
     * @brief Construct a node with a given parent
     * @param parent parent of new node
     */
    explicit EnableOutput(std::shared_ptr<SubSystem> parent);

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
    EnableOutput(std::shared_ptr<SubSystem> parent, EnableOutput* orig);

public:
    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::set<GraphMLParameter> graphMLParameters() override;

    /**
     * @brief Creates a EnableOutput node from a GraphML Description
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
    static std::shared_ptr<EnableOutput> createFromGraphML(int id, std::string name,
                                                    std::map<std::string, std::string> dataKeyValueMap,
                                                    std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    std::string labelStr() override ;


    /**
     * @brief Validate if the connections to this port are correct.
     *
     *   - Verify that the input port is a boolean type (and width 1).  Also Verify 1 and only 1 arc is connected
     *   - Checks that the port is in the EnabledSubsystem EnabledOutput list
     *
     * If an invalid configuration is detected, the function will throw an exception
     */
    void validate() override;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    //Override hasState (it is like a transparent latch)
    bool hasState() override;

    /**
     * @brief Creates a the StateUpdate node for the EnableOutput
     *
     * Because the EnableOutput is a transparent latch, the update is placed between the input node to the EnableOutput
     * and itself.  The update to the state occurres immediatly when the input is calculated.  The emitCExprNextState
     * function does nothing for the EnableOutput.  The emitCExpr always returns the state element.
     *
     */
    bool createStateUpdateNode(std::vector<std::shared_ptr<Node>> &new_nodes,
                               std::vector<std::shared_ptr<Node>> &deleted_nodes,
                               std::vector<std::shared_ptr<Arc>> &new_arcs,
                               std::vector<std::shared_ptr<Arc>> &deleted_arcs) override;

    /**
     * @brief Generate the C expression for the EnableOutput
     *
     * This function simply returns the name of the state variable
     */
    virtual CExpr emitCExpr(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag) override;

    /**
     * @brief Get a list of State Variables
     *
     * @note This function also calculate the parameter of the state vars.  It is done here because the node must already been connected and validated
     *
     * @return A list of State Variables
     */
    std::vector<Variable> getCStateVars() override;

//    /**
//     * @brief Calculates the next state for the latch
//     *
//     * Should be immediatly assigned
//     */
//    void emitCExprNextState(std::vector<std::string> &cStatementQueue) override;

    /**
     * @brief Sets the latch (state) from the calculated input
     */
    void emitCStateUpdate(std::vector<std::string> &cStatementQueue) override;

    //==== Getters & Setters ====
    Variable getStateVar() const;
    void setStateVar(const Variable &stateVar);
//    Variable getNextStateVar() const;
//    void setNextStateVar(const Variable &nextStateVar);
    std::vector<NumericValue> getInitCondition() const;
    void setInitCondition(const std::vector<NumericValue> &initCondition);
};

/*@}*/

#endif //VITIS_ENABLEOUTPUT_H
