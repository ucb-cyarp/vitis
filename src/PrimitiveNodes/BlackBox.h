//
// Created by Christopher Yarp on 2019-01-28.
//

#ifndef VITIS_BLACKBOX_H
#define VITIS_BLACKBOX_H

#include "GraphCore/NodeFactory.h"
#include "PrimitiveNode.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 */
/*@{*/

/**
 * @brief This represents a call to an external C/C++ function.  This function is treated like a black box from
 * the perspective of the the emitter and scheduler.
 *
 * The C/C++ function is allowed to have state.  If the function does include state, then the corresponding instance
 * varible in this class should be set to true.  For statefull black boxes, 3 functions should be supplied:
 *   * exeCombonational (arguments are input port expressions in order).  Input ports covered by external (global variables) are also OK and will be removed from the argument list for the function
 *   * stateUpdate (no arguments)
 *   * reset (no arguments)
 * exeCombonational is called when the BlackBox is scheduled.  The order of operands should match the input port numbering.
 * If an input port is complex, it must be split into 2 arguments with the real component coming first, followed by the
 * imagionary component. stateUpdate is called once all dependent nodes have been scheduled (similar to delay state
 * updates).  If the port is complex, both the real and imagionary components are manditory. reset is called to reset
 * the state of the BlackBox.
 *
 * A list of outputs that are directly from state can be specified.  Output arcs from these ports are disconnected for scheduling.
 */
class BlackBox : public PrimitiveNode {
    friend NodeFactory;

public:
    enum class ReturnMethod{
        NONE, ///< The function returns nothing
        SCALAR, ///< The function returns a single scalar value (output 0)
        OBJECT, ///< The function returns an object
        SCALAR_POINTER, ///< The function returns a pointer to a single scalar value (output 0)
        OBJECT_POINTER, ///< The function returns a pointer to an object
        EXT ///< The function modifies external variables (or globals)
    };

    enum class InputMethod{
        FUNCTION_ARG, ///< The input is fed directly to the function as an argument in the function call
        EXT ///< The input is in an external (global) variable and must be set before the function call
    };

    static ReturnMethod parseReturnMethodStr(std::string str);
    static std::string returnMethodToStr(ReturnMethod returnMethod);

    //TODO: ADD PARSER METHODS for ENUMS, ADD Generic Black Box Importer (exp for Port Access Names)

private:
    std::string exeCombinationalName; ///<The name of the exeCombinational function (called to process inputs and compute outputs
    std::string exeCombinationalRtnType; ///<The C++ datatype string for the return type of the exeCombinational function (can be an empty string)
    std::string stateUpdateName; ///<The name of the stateUpdate function
    std::string resetName; ///<The name of the reset function
    std::string cppHeaderContent; ///<The content of the C/C++ header file for this node.  The header should be properly wrapped with the #ifndef #def convention
    std::string cppBodyContent; ///<The content of the C/C++ file for this node
    bool stateful; ///<Denotes if this black box node is stateful or not
    std::string reSuffix; ///<If a port is complex, this represents the suffix appended to the signal name for the real component
    std::string imSuffix; ///<If a port is complex, this represents the suffix appended to the signal name for the imag component
    std::vector<InputMethod> inputMethods; ///<Denotes the method used to pass a given input to the function
    std::vector<std::string> inputAccess; ///<If InputMethod is EXT, specifies the name of the input port's external variable.  An entry in this array must be specified for each port if any port has an InputMethod of EXT
    std::vector<int> registeredOutputPorts; ///<If this node is stateful, identifies which outputs are registered
    ReturnMethod returnMethod; ///< The method by which the outputs of the BlackBox function calls are retrieved
    std::vector<std::string> outputAccess; ///<If RetrunMethod is OBJECT, OBJECT_POINTER, or EXT, specifies the name for each output in the returned data structure (if in an object) or ext variable name (real component)
    bool previouslyEmitted; ///<Used to check if the node has already been emitted before

    //==== Constructors ====
protected:
    /**
     * @brief Constructs an empty BlackBox node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    BlackBox();

    /**
     * @brief Constructs an empty BlackBox node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit BlackBox(std::shared_ptr<SubSystem> parent);

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
    BlackBox(std::shared_ptr<SubSystem> parent, BlackBox* orig);

public:
    //====Getters/Setters====
    std::string getExeCombinationalName() const;
    void setExeCombinationalName(const std::string &exeCombinationalName);

    std::string getExeCombinationalRtnType() const;
    void setExeCombinationalRtnType(const std::string &exeCombinationalRtnType);

    std::string getStateUpdateName() const;
    void setStateUpdateName(const std::string &stateUpdateName);

    std::string getResetName() const;
    void setResetName(const std::string &resetName);

    std::string getCppHeaderContent() const;
    void setCppHeaderContent(const std::string &cppHeaderContent);

    std::string getCppBodyContent() const;
    void setCppBodyContent(const std::string &cppBodyContent);

    bool isStateful() const;
    void setStateful(bool stateful);

    std::vector<int> getRegisteredOutputPorts() const;
    void setRegisteredOutputPorts(const std::vector<int> &registeredOutputPorts);

    ReturnMethod getReturnMethod() const;
    void setReturnMethod(ReturnMethod returnMethod);

    std::vector<std::string> getOutputAccess() const;
    void setOutputAccess(const std::vector<std::string> &outputAccess);

    bool isPreviouslyEmitted() const;
    void setPreviouslyEmitted(bool previouslyEmitted);

    std::vector<InputMethod> getInputMethods() const;
    void setInputMethods(const std::vector<InputMethod> &inputMethods);

    std::vector<std::string> getInputAccess() const;
    void setInputAccess(const std::vector<std::string> &inputAccess);

    std::string getReSuffix() const;
    void setReSuffix(const std::string &reSuffix);

    std::string getImSuffix() const;
    void setImSuffix(const std::string &imSuffix);

    //====Factories====
    /**
     * @brief Creates a BlackBox node from a GraphML Description
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

protected:
    // Moved to a seperate function so it can be called by subclass createFromGraphML function
    /**
     * @brief Populates the properties of this BlackBox node from GraphML
     *
     * @param id the ID number of the node
     * @param name the human readable name of a node
     * @param dataKeyValueMap A map of property keys and values extracted from the data nodes in the GraphML
     * @param parent The parent of this node in the hierarchy
     * @param dialect The dialect of the GraphML file being imported
     */
    void populatePropertiesFromGraphML(int id, std::string name,
                                       std::map<std::string, std::string> dataKeyValueMap,
                                       std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

public:

    //====Methods====
    bool hasState() override;

    bool hasCombinationalPath() override;

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    /**
     * @brief Get the GraphML Parameters that are common to BlackBox and its successors (namely StateFlow)
     */
    std::set<GraphMLParameter> graphMLParametersCommon();

    /**
     * @brief Emits the graphML that is common to BlackBox and its successors (namely StateFlow)
     * @param doc the xml document being emitted into
     * @param graphNode the xml node for this node
     */
    void emitGraphMLCommon(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode);

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    void validate() override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                    bool imag = false) override;


    //State update

    void emitCExprNextState(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType) override;

    void emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType) override;

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
                               std::vector<std::shared_ptr<Arc>> &deleted_arcs) override;

    //TOOD: State Update

    //TODO: If has state -> do state update node creation like delays
};

/*@}*/

#endif //VITIS_BLACKBOX_H
