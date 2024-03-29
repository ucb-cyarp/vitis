//
// Created by Christopher Yarp on 9/3/19.
//

#ifndef VITIS_THREADCROSSINGFIFO_H
#define VITIS_THREADCROSSINGFIFO_H

#include <vector>
#include <memory>
#include <map>
#include <string>
#include <complex>

#include "GraphCore/NumericValue.h"
#include "GraphMLTools/GraphMLDialect.h"
#include "GraphCore/StateUpdate.h"
#include "MultiRate/ClockDomain.h"
#include "ThreadCrossingFIFOParameters.h"

/**
 * \addtogroup MultiThread Multi-Thread Support
 * @brief Nodes and helper classes which support Multi-threaded Code Generator
 * @{
 */

/**
 * @brief Represents a thread crossing FIFO in multi-threaded export.  Note that the read and write checks occur in the core scheduler.
 * The actual read and write operation may also occur in the scheduler and are handled via function arguments and return values
 *
 * Note, the FIFO should reside in the partition that is writing into the FIFO.  This is because the emitters treat nodes with state differently.
 * When scheduled/emitted, the emitter the emitCExprNextState function instead of the emitCExpr function.  The emitCExprNextState function is
 * where the value to be written into the FIFO is stored into output variable to be handled by the core scheduler.  Since the actual data
 * movement of the FIFO is handled by the core schedulers, the emitCStateUpdate function does nothing.  As such, this block does not actually
 * need to create a state update node (like the delay or EnabledOutput nodes do).
 * Nodes with state (and no combinational path) also have their output arcs removed before scheduling begins.  This prevents cycles
 * from forming in the schedule and allows different pipeline stages to be scheduled independently.
 */
class ThreadCrossingFIFO : public Node {
    friend NodeFactory;
protected:
    int fifoLength; ///<The length of this FIFO (in blocks)
    std::vector<std::vector<NumericValue>> initConditions; ///<Initial values for this FIFO (in elements).  The FIFO will be initialized to contain these values.  Size must be a multiple of the block size.  The outer vector is across input/output port pairs.  The inner vector contains the initial conditions for the given port pair.

    //TODO: Need to set the block size according to the block domains and clock domains.  Note that, if the lowest level clock domain is not using vector mode, the block size is defined by the relative rate of this domain.  The sub-blocking size is set to 1 in this case.
    std::vector<int> blockSizes; ///<The block sizes (in items) of transactions to each input of the FIFO.  It is possible for different ports to have different block sizes due to clock domains and blocking domains
    //Note, input and output should have the same block size.  However, they may have different sub-block sizes
    std::vector<int> subBlockSizesIn; ///<The sub-block sizes (in items) of transactions.  This is how many elements are accessed at a time.  This should be the size of the outer dimension of the input and output type.  If the subBlockSizes are 1, the I/O dimension will have 1 fewer dimension
    std::vector<int> baseSubBlockSizesIn; ///<The sub-block sizes of the nodes at the input of the FIFO

    std::vector<int> subBlockSizesOut; ///<The sub-block sizes (in items) of transactions.  This is how many elements are accessed at a time.  This should be the size of the outer dimension of the input and output type.  If the subBlockSizes are 1, the I/O dimension will have 1 fewer dimension
    std::vector<int> baseSubBlockSizesOut; ///<The sub-block sizes of the nodes at the outputs of the FIFO.  All outputs of a FIFO port must have the same base sub-blocking length

    std::vector<Variable> cStateVars; ///<The C variables from which values are read.  There is one per input/output port pair
    std::vector<Variable> cStateInputVars; ///<the C temporary variables holding data to be writen.  There is one per input/output pair

    std::vector<std::string> cBlockIndexExprsInput; ///<The C expression used in the compute loop for indexing into a block when the FIFO is used as an input.  Must be set before emit
    std::vector<std::string> cBlockIndexExprsOutput; ///<The C expression used in the compute loop for indexing into a block when the FIFO is used as an output.  Must be set before emit

    std::vector<bool> cStateVarsInitialized;
    std::vector<bool> cStateInputVarsInitialized;

    std::vector<std::shared_ptr<ClockDomain>> clockDomains; ///<The clock domains that this FIFO is associated with.  Each port can be in a different clock domain.  Because the FIFO exists in the context of the source, FIFOs connected to the MasterInput will not be in the context of the clock domain they belong to

    ThreadCrossingFIFOParameters::CopyMode copyMode;

    //==== Constructors ====
    /**
     * @brief Constructs an empty ThreadCrossing FIFO node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    ThreadCrossingFIFO();

    /**
     * @brief Constructs an empty ThreadCrossing FIFO node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit ThreadCrossingFIFO(std::shared_ptr<SubSystem> parent);

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
    ThreadCrossingFIFO(std::shared_ptr<SubSystem> parent, ThreadCrossingFIFO *orig);

    //====Helpers====
    //This version sets datatypes and includes the port number in the name
    static void initializeVarIfNotAlready(std::shared_ptr<Node> node, std::vector<Variable> &vars, std::vector<bool> &inits, int port, std::string suffix);

    //This version does not set datatypes and does not include the port number in the name
    static void initializeVarIfNotAlready(std::shared_ptr<Node> node, Variable &var, bool &init, std::string suffix);

public:
    enum class Role{
        NONE, //No defined role (does not use cached values)
        PRODUCER, //Uses cached producer state
        CONSUMER, //Uses cached consumer state
        PRODUCER_FULLCACHE, //Uses cached producer state, use cashed consumer state when possible
        CONSUMER_FULLCACHE //Uses cached consumer state, use cached producer state when possible
    };

    //====Getters/Setters====
    int getFifoLength() const;
    void setFifoLength(int fifoLength);

    std::vector<std::shared_ptr<ClockDomain>> getClockDomains() const;
    void setClockDomains(const std::vector<std::shared_ptr<ClockDomain>> &clockDomain);

    std::shared_ptr<ClockDomain> getClockDomainCreateIfNot(int portNum);
    void setClockDomain(int portNum, std::shared_ptr<ClockDomain> clockDomain);

    std::vector<std::vector<NumericValue>> getInitConditions() const;
    void setInitConditions(const std::vector<std::vector<NumericValue>> &initConditions);

    ThreadCrossingFIFOParameters::CopyMode getCopyMode() const;

    void setCopyMode(ThreadCrossingFIFOParameters::CopyMode copyMode);

    /**
     * @brief Returns true if threads operation on this FIFO use the data in the FIFO in place or copy to/from a local
     * buffer.  If used in place, checks need to be made to both input and output FIFOs before use.  State is only updated
     * after uses.
     *
     * For FIFOs not used in place, only the checks on the input FIFOs need to be conducted before computation.
     * The state of the FIFOs can be updated immediatly after the FIFO is read.  The output FIFOs only need to be checked
     * after computation and are updated after the write
     * @return
     */
    virtual bool isInPlace() = 0;

    /**
     * @brief
     * @param port
     * @return
     *
     * @note If the initConditions vector does not contain an entry for the specified port, entries will be inserted into the vector until an entry has been allocated for the port
     */
    std::vector<NumericValue> getInitConditionsCreateIfNot(int port);
    void setInitConditionsCreateIfNot(int port, const std::vector<NumericValue> &initConditions);

    std::vector<std::string> getCBlockIndexExprsInput() const;
    void setCBlockIndexExprsInput(const std::vector<std::string> &cBlockIndexExprInput);
    std::string getCBlockIndexExprInputCreateIfNot(int portNum);
    void setCBlockIndexExprInput(int portNum, const std::string &cBlockIndexExprInput);

    std::vector<std::string> getCBlockIndexExprsOutput() const;
    void setCBlockIndexExprsOutput(const std::vector<std::string> &cBlockIndexExprsOutput);
    std::string getCBlockIndexExprOutputCreateIfNot(int portNum);
    void setCBlockIndexExprOutput(int portNum, const std::string &cBlockIndexVarName);

    /**
     * @brief Gets the cStateVar for this FIFO.  If it has not yet been initialized, it will be initialized at this point
     * @return the initialized cStateVar for this FIFO
     *
     * @warning The returned state variable is not expanded for the block size.  To expand the varible for the block
     * size of the port, call getCStateVarExpandedForBlockSize
     */
    Variable getCStateVar(int port);
    void setCStateVar(int port, const Variable &cStateVar);

    /**
     * @brief Gets the cStateVar for this FIFO and expands it by the block size of the port.
     * If it has not yet been initialized, it will be initialized at this point
     *
     * @param port the port to get the cStateVar for
     * @return
     */
    Variable getCStateVarExpandedForBlockSize(int port);

    /**
     * @brief Gets the cStateInputVar for this FIFO.  If it has not yet been initialized, it will be initialized at this point
     * @return the initialized cStateInputVar for this FIFO
     */
    Variable getCStateInputVar(int port);
    void setCStateInputVar(int port, const Variable &cStateInputVar);

    Variable getCStateInputVarExpandedForBlockSize(int port);

    std::vector<int> getBlockSizes() const;
    int getBlockSizeCreateIfNot(int portNum);

    std::vector<int> getSubBlockSizesIn() const;
    int getSubBlockSizeInCreateIfNot(int portNum);
    std::vector<int> getSubBlockSizesOut() const;
    int getSubBlockSizeOutCreateIfNot(int portNum);

    std::vector<int> getBaseSubBlockSizesIn() const;
    int getBaseSubBlockSizeInCreateIfNot(int portNum);
    std::vector<int> getBaseSubBlockSizesOut() const;
    int getBaseSubBlockSizeOutCreateIfNot(int portNum);

    void setBlockSizes(const std::vector<int> &blockSizes);
    void setBlockSize(int portNum, int blockSize);

    void setSubBlockSizesIn(const std::vector<int> &subBlockSizes);
    void setSubBlockSizeIn(int portNum, int subBlockSize);
    void setSubBlockSizesOut(const std::vector<int> &subBlockSizes);
    void setSubBlockSizeOut(int portNum, int subBlockSize);

    void setBaseSubBlockSizesIn(const std::vector<int> &baseSubBlockSizes);
    void setBaseSubBlockSizeIn(int portNum, int baseSubBlockSize);
    void setBaseSubBlockSizesOut(const std::vector<int> &baseSubBlockSizes);
    void setBaseSubBlockSizeOut(int portNum, int baseSubBlockSize);

    /**
     * @brief Gets the number of elements in a block across all ports
     * @return
     */
    int getTotalBlockSizeAllPorts();

    //====Factories====
    //createFromGraphML needs to be implemented in non-abstract decendents of this class
    //putting functionality into a seperate class function
    void populatePropertiesFromGraphML(std::map<std::string, std::string> dataKeyValueMap,
                                       std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    //A seperate class function will write properties to graphml
    /**
     * @brief Emit the graphML data entries for this abstract class.  Can be called by the emitGraphML method in subclasses
     * @param doc the XML DOM Document
     * @param graphNode the DOM Element for the GraphNode
     */
    void emitPropertiesToGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode);

    std::string typeNameStr() override;

    /**
     * @brief This function only returns details in a string since this is an abstract class
     * @return
     */
    std::string labelStr() override;

    void validate() override;

    /**
     * @brief This returns the name of the function argument.  The FIFO read operation was conducted by the core scheduler
     * This function also initializes cStateVar if it is not already
     * @param cStatementQueue
     * @param schedType
     * @param outputPortNum
     * @param imag
     * @return name of the function argument
     */
    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                    bool imag) override;

    /**
     * @brief Specifies that the node has state.  This has implications on the topological sort algorithm and the emitter
     * Causes emitCExprNextState instead of emitCExpr to be emitted when the node is scheduled
     * @return true
     */
    bool hasState() override;

    /**
     * @brief Specifies that this does not have a combinational path. This has implications on the topological sort algorithm.
     * @Causes output arcs (except to state ouput nodes) to be removed for scheduling purposes
     * @return false
     */
    bool hasCombinationalPath() override;

    /**
     * @brief Returns nothing, state is allocated separately during thread creation
     * @returns nothing
     */
    std::vector<Variable> getCStateVars() override;

    /**
     * @brief This sets the output variable that corresponds to writing to the FIFO.  The actual write operation will occur in the core scheduler
     * This function also initializes cStateInputVar if it is not already
     * @param cStatementQueue
     * @param schedType
     */
    void emitCExprNextState(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType) override;

    /**
     * @brief This does not do anything.  The read/write operations are handled by the core schedulers
     * @param cStatementQueue
     * @param schedType
     */
    void emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, std::shared_ptr<StateUpdate> stateUpdateSrc) override;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override = 0;

    /**
     * @brief Does nothing since the state update is handled externally in the core scheduler
     *
     * @returns false
     */
    bool createStateUpdateNode(std::vector<std::shared_ptr<Node>> &new_nodes,
                               std::vector<std::shared_ptr<Node>> &deleted_nodes,
                               std::vector<std::shared_ptr<Arc>> &new_arcs,
                               std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                               bool includeContext) override;

    bool canExpand() override;

    //==== Thread Crossing FIFO Specific Functions ====

    /**
     * @brief Emits a C statement to check if the FIFO is not empty (has 1 or more blocks ready)
     *
     * To avoid reading a volatile pointer more than once per check, the pointers are de-referenced and stored in local
     * variables.  These statements are pushed onto the cStatementQueue and are referenced in the returned string.
     *
     * Scoping the call to this function may be desirable to limit the scope of these local variables
     *
     * Cached values will be used unless the role is specified as NONE
     *
     * @param cStatementQueue the current C statement queue.  Will be added to by the call to this function
     * @param role the role of the actor calling this function
     * @returns a C statements that evaluates to true if the FIFO is not empty and to false if the FIFO is empty
     */
    virtual std::string emitCIsNotEmpty(std::vector<std::string> &cStatementQueue, Role role) = 0;

    /**
     * @brief Emits a C statement to check if the FIFO is not full (has space for 1 or more blocks)
     *
     * To avoid reading a volatile pointer more than once per check, the pointers are de-referenced and stored in local
     * variables.  These statements are pushed onto the cStatementQueue and are referenced in the returned string.
     *
     * Scoping the call to this function may be desirable to limit the scope of these local variables
     *
     * Cached values will be used unless the role is specified as NONE
     *
     * @param cStatementQueue the current C statement queue.  Will be added to by the call to this function
     * @param role the role of the actor calling this function
     * @returns a C statements that evaluates to true if the FIFO is not full and to false if the FIFO is full
     */
    virtual std::string emitCIsNotFull(std::vector<std::string> &cStatementQueue, Role role) = 0;

    /**
     * @brief Emits a C statement to get the number of blocks available to read from the FIFO
     *
     * To avoid reading a volatile pointer more than once per check, the pointers are de-referenced and stored in local
     * variables.  These statements are pushed onto the cStatementQueue and are referenced in the returned string.
     *
     * Scoping the call to this function may be desirable to limit the scope of these local variables
     *
     * Cached values will be used unless the role is specified as NONE
     *
     * @param cStatementQueue the current C statement queue.  Will be added to by the call to this function
     * @param role the role of the actor calling this function
     * @returns a C statement to get the number of blocks available to read from the FIFO
     */
    virtual std::string emitCNumBlocksAvailToRead(std::vector<std::string> &cStatementQueue, Role role) = 0;

    /**
     * @brief Emits a C statement to get the number of blocks that can be written into the FIFO at this time
     *
     * To avoid reading a volatile pointer more than once per check, the pointers are de-referenced and stored in local
     * variables.  These statements are pushed onto the cStatementQueue and are referenced in the returned string.
     *
     * Scoping the call to this function may be desirable to limit the scope of these local variables
     *
     * Cached values will be used unless the role is specified as NONE
     *
     * @param cStatementQueue the current C statement queue.  Will be added to by the call to this function
     * @param role the role of the actor calling this function
     * @returns a C statement to get the number of blocks that can be written into the FIFO at this time
     */
    virtual std::string emitCNumBlocksAvailToWrite(std::vector<std::string> &cStatementQueue, Role role) = 0;

    /**
     * @brief Emits C statements which write data into the FIFO (if not in place)
     *
     * If FIFO operates in place this function returns a pointer to the block to be written to.
     * If the FIFO operates not in place, nothing is returned
     *
     * Cached values will be used unless the role is specified as NONE
     *
     * @param cStatementQueue The C statements are written into this queue
     * @param src The src for data to be written into the FIFO (for non-in-place FIFOs) or the name of the ptr to the location to write into [created by this function] (for in-place FIFOs)
     * @param numBlocks The number of blocks of data to write into the FIFO
     * @param role the role of the actor calling this function
     * @param pushStateAfter if true, the changes to the producer or consumer state are pushed immediately after the write operation
     * @param forceNotInPlace if true, forces the FIFO to copy to/from local buffers even if it is designed to work in place.  This is used primarily for I/O where non-blocking access to the FIFOs is used along with multiple local buffers.
     *
     * @returns pointer to block in buffer to be written to (in-place FIFO only)
     */
    virtual std::string emitCWriteToFIFO(std::vector<std::string> &cStatementQueue, std::string src, int numBlocks, Role role, bool pushStateAfter = true, bool forceNotInPlace = false) = 0;

    /**
     * @brief Emits C statements which read data from the FIFO (if not in place)
     *
     * Cached values will be used unless the role is specified as NONE
     *
     * If FIFO operates in place this function returns a pointer to the block to be read from.
     * If the FIFO operates not in place, nothing is returned
     *
     * @param cStatementQueue The C statements are written into this queue
     * @param dst The destination for data which will be read from the FIFO (for non-in-place FIFOs) or the name of the ptr to the location to read from [created by this function] (for in-place FIFOs)
     * @param numBlocks The number of blocks to read (used by both in place and out of place FIFOs to determine the state to push to the FIFO next)
     * @param role the role of the actor calling this function
     * @param pushStateAfter if true, the changes to the producer or consumer state are pushed immediately after the write operation
     * @param forceNotInPlace if true, forces the FIFO to copy to/from local buffers even if it is designed to work in place.  This is used primarily for I/O where non-blocking access to the FIFOs is used along with multiple local buffers.
     *
     * @returns pointer to block in buffer to be read from (in place FIFO only)
     */
    virtual std::string emitCReadFromFIFO(std::vector<std::string> &cStatementQueue, std::string dst, int numBlocks, Role role, bool pushStateAfter, bool forceNotInPlace = false) = 0;

    /**
     * @brief Get a list of shared variables for the given FIFO.  These variables should be passed to the threads which
     * communicate via this FIFO
     * @return a list of shared variables.  If the paired string is not empty, the variable uses a structure whose name is provided in the string.
     */
    virtual std::vector<std::pair<Variable, std::string>> getFIFOSharedVariables() = 0;

    /**
     * @brief Creates a structure which is the format for entries in the FIFO.  Each structure contains a block of samples
     */
    virtual std::string createFIFOStruct();

    /**
     * @brief Returns the type name of the FIFO structure.  Structures are used to store blocks in the FIFO
     * @return
     */
    virtual std::string getFIFOStructTypeName();

    /**
     * @brief Emits C statements that declare and allocate the shared variables for the FIFO
     *
     * This function should be called by the function creating threads.  These variables should be passed to threads.
     *
     * @param cStatementQueue The C statements will be written to this queue
     * @param core The core on which the memory should be allocated.  If -1, it is the core the current thread is running on (aligned_alloc).  Otherwise, it uses the vitis helper function
     */
    virtual void createSharedVariables(std::vector<std::string> &cStatementQueue, int core = -1) = 0;

    /**
     * @brief Emits C statements that initialize allocated variables
     *
     * This function should be called by the function creating threads.
     *
     * @param cStatementQueue The C statements will be written to this queue
     */
    virtual void initializeSharedVariables(std::vector<std::string> &cStatementQueue) = 0;


    /**
     * @brief Emits C statements that cleanup (de-allocate) shared variables for the FIFO
     *
     * @param cStatementQueue The C statements will be written to this queue
     */
    virtual void cleanupSharedVariables(std::vector<std::string> &cStatementQueue) = 0;


    /**
     * @brief Creates local variables used by the FIFO access functions
     *
     * @param cStatementQueue The C statements will be written to this queue
     */
    virtual void createLocalVars(std::vector<std::string> &cStatementQueue) = 0;

    /**
     * @brief Initializes the local variables according to the role specified
     *
     * If the role is consumer, the consumer variables will be initialized
     * If the role is producer, the producer variables will be initialized
     * If the role is none, both the consumer and producer variables will be initialized
     *
     * @param cStatementQueue The C statements will be written to this queue
     * @param role the role of the actor calling this function
     */
    virtual void initLocalVars(std::vector<std::string> &cStatementQueue, Role role) = 0;

    /**
     * @brief Loads the local variables based on the role
     *
     * If the role is producer, the consumer variables are pulled
     * If the role is consumer, the producer variables are pulled
     * If the role is none, both the producer and consumer variables are pulled
     *
     * @param cStatementQueue The C statements will be written to this queue
     * @param role the role of the actor calling this function
     */
    virtual void pullLocalVars(std::vector<std::string> &cStatementQueue, Role role) = 0;

    /**
     * @brief Writes the local variables based on the role
     *
     * If the role is producer, the producer variables are pushed
     * If the role is consumer, the consumer variables are pushed
     * If the role is none, both the producer and consumer variables are pushed
     *
     * @param cStatementQueue The C statements will be written to this queue
     * @param role the role of the actor calling this function
     */
    virtual void pushLocalVars(std::vector<std::string> &cStatementQueue, Role role) = 0;
};


/*! @} */

#endif //VITIS_THREADCROSSINGFIFO_H
