//
// Created by Christopher Yarp on 7/16/18.
//

#ifndef VITIS_MUX_H
#define VITIS_MUX_H

#include "PrimitiveNode.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/SelectPort.h"
#include "GraphCore/NumericValue.h"
#include "GraphMLTools/GraphMLDialect.h"
#include "GraphCore/ContextRoot.h"

#include <map>

/**
 * \addtogroup PrimitiveNodes Primitives
 */
/*@{*/

/**
 * @brief Represents a mux block.
 *
 * The selector port is a special port in this block
 * Ports 0-(n-1) are the input lines which are selected from.  The use of a seperate selector port allows the user to use
 * the standard @ref getInputPort method without re-mapping indexes of ports.
 *
 * The selector port's value directly maps to the input port index which is passed through.
 *
 * The default port is the last port (index n-1)
 *
 * This switch object can act as a multiport switch
 *
 * @note This mux assumes the selector port input is the index of the input to forward.  If a switch with a threshold is desired,
 * use @ref ThresholdSwitch.
 *
 * @note The Simulink Multiport Switch maps to this excpet that the index may be off by 1 (depending on the indexing scheme used).
 * To prevent confusion, a seperate @ref SimulinkMultiPortSwitch class is provided.  The @ref SimulinkMultiPortSwitch expands to a
 * mux and a -1 on the select line.
 *
 */
class Mux : public PrimitiveNode, public ContextRoot{
    friend NodeFactory;

private:
    std::unique_ptr<SelectPort> selectorPort;///< The port which determines the mux selection
    bool booleanSelect; ///< If true, the mux uses the simulink convention of the top (first) port being true and the bottom port being false
    bool useSwitch; ///< If true, the mux uses a switch statement rather than an if/else statement (when tthe selector is not a bool).  This forces contexts to be contiguous
    Variable muxContextOutputVar; ///< The context output variable

    //==== Constructors ====
    /**
     * @brief Constructs a mux without a parent specified.  Creates an empty selector port.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    Mux();

    /**
     * @brief Constructs a mux with a given parent.  Creates an empty selector port.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit Mux(std::shared_ptr<SubSystem> parent);

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
    Mux(std::shared_ptr<SubSystem> parent, Mux* orig);

public:
    //==== Getters/Setters ====
    /**
     * @brief Get an aliased shared pointer to the selector port.
     *
     * The pointer is aliased with this node as the stored pointer.
     *
     * @return aliased shared pointer to the selector port
     */
    std::shared_ptr<SelectPort> getSelectorPort() const;

    /**
    * @brief Add a select arc to the given node, updating referenced objects in the process
    *
    * Adds an select arc to this node.  Removes the arc from the orig port if not NULL.
    * Updates the given arc so that the destination port is set to the specified input
    * port of this node.  If the port object for the given number is not yet created, it
    * will be created during the call.
    * @param arc The arc to add
    */
    void addSelectArcUpdatePrevUpdateArc(std::shared_ptr<Arc> arc);

    bool isBooleanSelect() const;

    void setBooleanSelect(bool booleanSelect);

    bool isUseSwitch() const;

    void setUseSwitch(bool useSwitch);

    Variable getMuxContextOutputVar() const;

    void setMuxContextOutputVar(const Variable &muxContextOutputVar);

    //==== Factories ====
    /**
     * @brief Creates a mux node from a GraphML Description
     *
     * @note Supports VITIS dialect only for import
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
    static std::shared_ptr<Mux> createFromGraphML(int id, std::string name,
                                                  std::map<std::string, std::string> dataKeyValueMap,
                                                  std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    /**
     * @brief Performs check of @ref Node in addition to checking the select port
     */
    void validate() override;

    std::string labelStr() override ;

    //TODO: When syntehsizing C/C++ check that boolean is handled (false = 0, true = 1)
    //Boolean will produce an if/else statement

    /**
     * @brief Specifies that the MUX has internal fanout on each input.  This prevents logic from being ingested into
     * the if/else/switch statement when using @ref emitCExpr
     *
     * @note It is often desierable for logic to be brought into the conditional block but care must be take as to
     * what operations can safely be moved inside.  Emit with contexts & context wrappers (allong with context
     * identification) can pull operations into the conditional.
     */
    virtual bool hasInternalFanout(int inputPort, bool imag) override;

    /**
     * @brief Emits a C expression for the mux
     *
     * for boolean select lines, an if/else is created.  For integer types, a switch statement is created.
     *
     * The output of the function is a tmp variable of the form: \<muxName\>_n\<id\>_out
     *
     * @note This function emits the most basic implementation of a multiplexer where all paths leading to the
     * mux are fully computed but only one path's result is selected.  This function does not contain any logic other
     * than selection in the if/else/switch conditional statements.  To contain other logic in the conditional
     * statement, emit with contexts and context wrappers is required.  In this case, this function will not be called
     * as the output assignment will occur during the context emit.
     */
    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) override;

    /**
     * @brief Version of emitCExpr for the bottom up scheduler.
     *
     * Gets the statement of each input port and creates a if/else or switch block.
     *
     * This should be used when context emit is not used.
     *
     * @param cStatementQueue the queue of C statements (modified durring the call to this function
     * @param schedType the scheduler used (parameter may not be used unless the C emit for the given node is different depending on the scheduler used - ex. if the scheduler is context aware)
     * @param outputPortNum the output port for which the C expression is being generated
     * @param imag if true, generate the imagionary component of the output, otherwise generate the real component
     * @return the C expression for the output
     */
    CExpr emitCExprBottomUp(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag);

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    void cloneInputArcs(std::vector<std::shared_ptr<Arc>> &arcCopies, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode, std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &origToCopyArc, std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &copyToOrigArc) override;

    std::set<std::shared_ptr<Arc>> disconnectInputs() override;

    std::set<std::shared_ptr<Node>> getConnectedInputNodes() override;

    unsigned long inDegree() override;

    std::vector<std::shared_ptr<Arc>> connectUnconnectedPortsToNode(std::shared_ptr<Node> connectToSrc, std::shared_ptr<Node> connectToSink, int srcPortNum, int sinkPortNum) override;

    std::vector<std::shared_ptr<InputPort>> getInputPortsIncludingSpecial() override;

    /**
     * @brief Discover the context for nodes connexted to the mux
     *
     * @note This function does not set the context hierarchy for marked nodes or set the nodesInContext array for the mux
     *
     * @return A map of input ports to sets of nodes in the port's context
     */
    std::map<std::shared_ptr<InputPort>, std::set<std::shared_ptr<Node>>> discoverContexts();

    /**
     * @brief Discovers and marks Mux contexts within a specific context level
     *
     * For example, this function should be called on muxes within an enabled subsystem.
     *
     * This method discovers mux contexts, discovers the hierarchy of contexts, set the context hierarchy in nodes
     * within the mux contexts, and sets the context node lists in the muxes
     *
     * @param muxes muxes at the desired level of the design
     */
    static void discoverAndMarkMuxContextsAtLevel(std::vector<std::shared_ptr<Mux>> muxes);

    //TODO: Update Mux Emit for Scheduled vs Bottom Up

    bool createContextVariableUpdateNodes(std::vector<std::shared_ptr<Node>> &new_nodes,
                                          std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                          std::vector<std::shared_ptr<Arc>> &new_arcs,
                                          std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                          bool setContext) override;

    std::vector<Variable> getCContextVars() override;

    bool requiresContiguousContextEmits() override;

    Variable getCContextVar(int contextVarIndex) override;

    void emitCContextOpenFirst(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber) override;
    void emitCContextOpenMid(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber) override;
    void emitCContextOpenLast(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber) override;

    void emitCContextCloseFirst(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber) override;
    void emitCContextCloseMid(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber) override;
    void emitCContextCloseLast(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int subContextNumber) override;
};

/*@}*/

#endif //VITIS_MUX_H
