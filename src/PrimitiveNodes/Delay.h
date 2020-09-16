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
 * @brief Represents a Delay (z^-n) Block
 *
 * @warning earliestFirst is not currently supported for standard Delays (not TappedDelays) due to the current implementation
 *          of delay absorption.  All Delays should be initialized to set earliestFirst to be false for now.
 */
class Delay : public PrimitiveNode{
    friend NodeFactory;
public:
    enum class BufferType{
        AUTO, ///<Selects Shift Register or Circular Buffer Implementation Automatically Based on Configuration
        SHIFT_REGISTER, ///<Selects Shift Register Implementation
        CIRCULAR_BUFFER ///<Selects Circular Buffer Implementation
    };

protected:
    int delayValue; ///<The amount of delay in this node
    std::vector<NumericValue> initCondition; ///<The Initial condition of this delay.  Number of elements must match the delay value times the size of each element.  The initial condition that will be presented first is at index 0
    Variable cStateVar; ///<The C variable storing the state of the delay
    Variable cStateInputVar; ///<the C temporary variable holding the input to state before the update
    Variable circularBufferOffsetVar; ///<The C variable shoring the current head index of the circular buffer (if a circular buffer implementation is being used)
    bool roundCircularBufferToPowerOf2; ///<If true and using circular buffers, the buffer size is rounded up to the nearest power of 2
    //TODO: Re-evaluate if numeric value should be stored as double/complex double (like Matlab does).  An advantage to providing a class that can contain both is that there is less risk of an int being store improperly and full 64 bit integers can be represented.
    //TODO: Advantage of storing std::complex is that each element is smaller (does not need to allocate both a double and int version)
    BufferType bufferImplementation; ///<The type of buffer to implement.  This is ignored for delays of 0 or 1

    bool earliestFirst; ///<If true, the new values are stored at the start of the array.  If false, new values are stored at the end of the array.  The default is false.
    bool allocateExtraSpace; ///<If true, an extra space is allocated in the array according to earliestFirst.  The extra space is allocated at the end of the array where new values are inserted.  This has no effect when delay == 0.  The default is false

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
    BufferType getBufferImplementation() const;
    void setBufferImplementation(BufferType bufferImplementation);

    //For tappedDelay
    bool isEarliestFirst() const;
    void setEarliestFirst(bool earliestFirst);
    bool isAllocateExtraSpace() const;
    void setAllocateExtraSpace(bool allocateExtraSpace);

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

    /**
     * @brief Populates the delay properties from graphML into a delay node.  Declared seperatly from creatFromGraphML so that TappedDelay
     * can use the same logic and later extend it.
     * @param node the node to import properties into
     * @param simulinkDelayLenName the simulink parameter name for delay length (different between delay and tapped delay)
     * @param simulinkInitCondName the simulink parameter name for initial conditions (different between delay and tapped delay)
     * @param id the ID number of the node
     * @param name the human readable name of a node
     * @param dataKeyValueMap A map of property keys and values extracted from the data nodes in the GraphML
     * @param parent The parent of this node in the hierarchy
     * @param dialect The dialect of the GraphML file being imported
     */
    static void populatePropertiesFromGraphML(std::shared_ptr<Delay> node, std::string simulinkInitCondName,
                                              std::string simulinkDelayLenName,
                                              int id, std::string name,
                                              std::map<std::string, std::string> dataKeyValueMap,
                                              std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    /**
     * @brief Used to determine how to broadcast scalar initial conditions.  Knowledge of the dimensions of the input/output
     * are required for this
     */
    void propagateProperties() override;

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type) override ;

    /**
     * @brief Emits the core parameters for the delay block.  Separated from emitGraphML so that TappedDelay can use it
     *
     * Undoes initial conditions added for an extra element (used by tapped delay) or for circular buffer with power of 2 allocation
     *
     * @param doc
     * @param xmlNode
     */
    void emitGraphMLDelayParams(xercesc::DOMDocument *doc, xercesc::DOMElement* xmlNode);

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

    /**
     * @brief Check if a circular buffer implementation is used for this block.
     *
     * If bufferImplementation is AUTO, a circular buffer is implemented if:
     *  - Delay is ==2 and the input is a vector
     *  - Delay is >2
     *
     *  Otherwise, a shift register is used.
     *
     * @return
     */
    bool usesCircularBuffer();

    /**
     * @brief This function returns initial conditions without modifications for circular buffers or extra element, or reversing
     * @return
     */
    virtual std::vector<NumericValue> getExportableInitConds();

protected:
    void decrementAndWrapCircularBufferOffset(std::vector<std::string> &cStatementQueue);
    void incrementAndWrapCircularBufferOffset(std::vector<std::string> &cStatementQueue);
    int getBufferLength();

    void assignInputToBuffer(std::string insertPosition, std::vector<std::string> &cStatementQueue);
    void assignInputToBuffer(CExpr src, std::string insertPosition, bool imag, std::vector<std::string> &cStatementQueue);

    std::vector<NumericValue> getExportableInitCondsHelper();

    /**
     * @brief Split from propagateProperties so the same logic can be used in both the Delay and TappedDelay nodes.
     * Delays need to reverse initial conditions if earlyFirst is selected but TappedDelays do not
     */
    void propagatePropertiesHelper();
};

/*! @} */


#endif //VITIS_DELAY_H
