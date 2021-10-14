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
 *
 * @warning limited configuration options are currently implemented for transactionBlockSize > 1
 *          Circular Buffering with Double Buffer Len
 */
class Delay : public PrimitiveNode{
    friend NodeFactory;
public:
    enum class BufferType{
        AUTO, ///<Selects Shift Register or Circular Buffer Implementation Automatically Based on Configuration
        SHIFT_REGISTER, ///<Selects Shift Register Implementation
        CIRCULAR_BUFFER ///<Selects Circular Buffer Implementation
    };

    enum class CircularBufferType{
        NO_EXTRA_LEN, ///<No extra length is allocated to allow returning stride 1 arrays in TappedDelay.  A circular buffer is returned to dependant blocks, requiring additional indexing operations.
        DOUBLE_LEN, ///<Allocate double the length of the buffer to allow for returning stride 1 arrays in TappedDelay result.  A non-circular array is returned to dependant blocks.
        PLUS_DELAY_LEN_M1 ///<Allocates an additional DelayLen-1 to the buffer to allow for returning.  A non-circular array is returned to dependant blocks.
    };

    enum class CopyMethod{
        FOR_LOOPS, ///<Copy inputs (>1 item) into the buffer using for loops
        MEMCPY, ///<Copy inputs (>1 item) into the buffer using standard memcpy
        CLANG_MEMCPY_INLINE, ///<Copy inputs (>1 item) into the buffer using Clang's __builtin_memcpy_inline
        FAST_COPY_UNALIGNED ///<Copy inputs (>1 item) into the buffer using my memcpy implementation using vector intrinsics
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

    bool earliestFirst; ///<(Perhaps more accurately, most recent first) If true, the new values are stored at the start of the array.  If false, new values are stored at the end of the array.  The default is false.
    bool allocateExtraSpace; ///<If true, an extra space is allocated in the array according to earliestFirst.  The extra space is allocated at the end of the array where new values are inserted.  This has no effect when delay == 0.  Has no effect for standard delay when block size >1.  The default is false
    CircularBufferType circularBufferType; ///< If usesCircularBuffer() is true, determines the style of circular buffer.  This primarily controls the allocation of extra space and double writing new values so that a stride-1 buffer can be passed to dependent nodes, avoiding the additional indexing logic required when reading circular buffers
    int transactionBlockSize; ///< When specializing for sub-blocking, defines how many incoming samples are processed at once.  Defaults to 1.  If the delay is a multiple of the sub-blocking size and was transformed to a vector delay, this is kept as 1

    //Deferal variables.  Only used durring delay blocking specialization deferral
    //TODO: Refactor and create FIFO merging pass?
    bool blockingSpecializationDeferred; ///<Indicates that this node went through the logic of specialization but did not make changes to the delay node itself.  The arcs coming into and out of the delay were scaled appropriatly.  This is specifically to accomodate FIFO delay absorption whose logic could require delays to be re-merged
    int deferredBlockSize;
    int deferredSubBlockSize;

    CopyMethod copyMethod; ///<The Copy method used when ingesting >1 items

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

    /**
     * @brief Reverses init conditions, primarily used when handling earliest first.
     *
     * Is aware of non-scalar types and
     *
     * @param Initial conditions to reverse (does not use the member variable to allow other modifications to initial conditions for allocation additional space)
     * @return Returns intitial conditions reversed
     */
    std::vector<NumericValue> reverseInitConds(std::vector<NumericValue> &initConds);

    /**
     * @brief There are several different options for the delay including earliestFirst, allocate extra space, round up allocation, ...
     * Each of these options requires initial conditions to be re-shaped when
     *
     * @warning if called after specialization deferred, ensure that all arcs have been expanded beforehand
     * @return
     */
    std::vector<NumericValue> getInitConditionsReshapedForConfig();

    void specializeForBlockingWOptions(bool processArcs,
                                       int localBlockingLength,
                                       int localSubBlockingLength,
                                       std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                       std::vector<std::shared_ptr<Node>> &nodesToRemove,
                                       std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                       std::vector<std::shared_ptr<Arc>> &arcsToRemove,
                                       std::vector<std::shared_ptr<Node>> &nodesToRemoveFromTopLevel,
                                       std::map<std::shared_ptr<Arc>, int> &arcsWithDeferredBlockingExpansion);

    void specializeForBlockingArcExpandOnly(int localBlockingLength,
                                            std::map<std::shared_ptr<Arc>, int> &arcsWithDeferredBlockingExpansion);

    /**
     * @brief A helper function for copying from an input expression to the delay buffer.  It respects the copyMethod
     *        function
     * @param src
     * @param insertPositionIn
     * @param imag
     * @param cStatementQueue
     */
    void copyToBuffer(CExpr src, std::string insertPositionIn, bool imag, std::vector<std::string> &cStatementQueue);

public:
    //====Getters/Setters====
    int getDelayValue() const;
    void setDelayValue(int delayValue);
    std::vector<NumericValue> getInitCondition() const;
    void setInitCondition(const std::vector<NumericValue> &initCondition);
    BufferType getBufferImplementation() const;
    void setBufferImplementation(BufferType bufferImplementation);
    CircularBufferType getCircularBufferType() const;
    void setCircularBufferType(CircularBufferType circularBufferType);
    int getTransactionBlockSize() const;
    void setTransactionBlockSize(int transactionBlockSize);

    //For tappedDelay
    bool isEarliestFirst() const;
    void setEarliestFirst(bool earliestFirst);
    bool isAllocateExtraSpace() const;
    void setAllocateExtraSpace(bool allocateExtraSpace);

    bool isBlockingSpecializationDeferred() const;
    void setBlockingSpecializationDeferred(bool blockingSpecializationDeferred);
    int getDeferredBlockSize() const;
    void setDeferredBlockSize(int deferredBlockSize);
    int getDeferredSubBlockSize() const;
    void setDeferredSubBlockSize(int deferredSubBlockSize);

    CopyMethod getCopyMethod() const;
    void setCopyMethod(CopyMethod copyMethod);

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
    static void populatePropertiesFromGraphML(std::shared_ptr<Delay> node, std::string simulinkDelayLenName,
                                              std::string simulinkInitCondName,
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

    std::set<std::string> getExternalIncludes() override;

    /**
     * @brief Get the initial circular buffer index location
     * @return
     */
    virtual int getCircBufferInitialIdx();

    //When include current value (allocateExtraSpace in Delay) is true (or delay < block size,  block size > 1),
    //getting the next state and updating the state are partially superfluous because the current value has already
    // been copied into the array by cEmitExpr (so long as it was called before emitCExprNextState and emitCStateUpdate.
    // If this node has an output arc at the time of emit, we will assume this is true and the added logic is not required.
    virtual bool requiresStandaloneCExprNextState();
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
     * @brief The delay can break a sub-blocking dependency if the delay is >= the sub-blocking length
     *
     * If the delay is > sub-blocking length & is not a multiple of the sub-blocking length, it is split into 2 delays
     *
     * @param localSubBlockingLength
     * @return
     */
    bool canBreakBlockingDependency(int localSubBlockingLength) override;

    void specializeForBlocking(int localBlockingLength,
                               int localSubBlockingLength,
                               std::vector<std::shared_ptr<Node>> &nodesToAdd,
                               std::vector<std::shared_ptr<Node>> &nodesToRemove,
                               std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                               std::vector<std::shared_ptr<Arc>> &arcsToRemove,
                               std::vector<std::shared_ptr<Node>> &nodesToRemoveFromTopLevel,
                               std::map<std::shared_ptr<Arc>, int> &arcsWithDeferredBlockingExpansion) override;

    bool specializesForBlocking() override;

    /**
     * @brief Performs the same basic actions of specializeForBlocking except that the actions of splitting the delay
     *        or configuring the delay node for blocking.  However, arcs which
     *
     *        Sets a boolean indicating that specialization was deferred but that
     * @param localBlockingLength
     * @param localSubBlockingLength
     * @param nodesToAdd
     * @param nodesToRemove
     * @param arcsToAdd
     * @param arcsToRemove
     * @param nodesToRemoveFromTopLevel
     * @param arcsWithDeferredBlockingExpansion
     */
     void specializeForBlockingDeferDelayReconfigReshape(int localBlockingLength,
                               int localSubBlockingLength,
                               std::vector<std::shared_ptr<Node>> &nodesToAdd,
                               std::vector<std::shared_ptr<Node>> &nodesToRemove,
                               std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                               std::vector<std::shared_ptr<Arc>> &arcsToRemove,
                               std::vector<std::shared_ptr<Node>> &nodesToRemoveFromTopLevel,
                               std::map<std::shared_ptr<Arc>, int> &arcsWithDeferredBlockingExpansion);

protected:
    void decrementAndWrapCircularBufferOffset(std::vector<std::string> &cStatementQueue);
    virtual void incrementAndWrapCircularBufferOffset(std::vector<std::string> &cStatementQueue);

    /**
     * @brief Returns the buffer length (or buffer_length/2 when double length allocation is used).
     * It accounts for extra elements if required and if the buffer allocation is rounded up to a power of 2.
     * This is the length which is used when determining wraparound of the write pointer.
     *
     * When blocking, it is forced to be in units of the block size.
     *
     * @warning it does no account for extra space added for alternate implementations of the circular buffer (DOUBLE_LEN, PLUS_DELAY_LEN_M1) which allow stride 1 arrays to be returned by TappedDelay
     */
    int getBufferLength();

    /**
     * @brief Returns the actual allocated buffer length including extra elements required by alternate implementations of the circular buffer (DOUBLE_LEN, PLUS_DELAY_LEN_M1) which allow stride 1 arrays to be returned by TappedDelay
     */
    int getBufferAllocatedLen();

    void assignInputToBuffer(std::string insertPosition, std::vector<std::string> &cStatementQueue);
    void assignInputToBuffer(CExpr src, std::string insertPosition, bool imag, std::vector<std::string> &cStatementQueue);

    /**
     * @brief Split from propagateProperties so the same logic can be used in both the Delay and TappedDelay nodes.
     * Delays need to reverse initial conditions if earlyFirst is selected but TappedDelays do not
     */
    void propagatePropertiesHelper();

    /**
     * @brief For circular buffers with additional elements for returning unit stride arrays, get the index of the second copy of the element at the given index
     *
     * @warning result is invalid if circular buffer is not used and implementation of circular buffers with extra elements is not used
     *
     * @param firstIndex The index of the first element
     * @return
     */
    std::string getSecondIndex(std::string firstIndex);

    /**
     * @brief For circular buffers of the CircularBufferType::PLUS_DELAY_LEN_M1, returns the check to determine if a second write is necessary
     *
     * @param index the index of the first copy of the element in the circular buffer
     * @return
     */
    std::string getSecondWriteCheck(std::string firstIndex);

    /**
     * @brief Splits a delay into 2 nodes.
     *
     * The given node is adjusted to the targetDelayLength.  A new delay node is created with the remaining delay.
     *
     * New delay is created before the current delay
     *
     * @param nodesToAdd
     * @param arcsToAdd
     * @param targetDelayLength
     * @return a pointer to the new delay which was created.  nullptr if no new delay was created
     */
    virtual std::shared_ptr<Delay> splitDelay(std::vector<std::shared_ptr<Node>> &nodesToAdd, std::vector<std::shared_ptr<Arc>> &arcsToAdd, int targetDelayLength);

    /**
     * @brief Only passes through when Delay==0.  In that case, this node has no state.
     * @return
     */
    bool passesThroughInputs() override;
};

/*! @} */


#endif //VITIS_DELAY_H
