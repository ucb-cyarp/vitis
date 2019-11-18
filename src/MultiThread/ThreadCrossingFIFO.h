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

/**
 * \addtogroup MultiThread Multi-Thread Support
 * @brief Nodes and helper classes which support Multi-threaded Code Generator
 * @{
 */

/**
 * @brief Represents a thread crossing FIFO in multi-threaded export.  Note that the read and write checks occur in the core scheduler.
 * The actual read and write operation may also occur in the scheduler and are handled via function arguments and return values
 *
 * Note, the FIFO sould reside in the partition that is writing into the FIFO.  This is because the emitters treat nodes with state differently.
 * When scheduled/emitted, the emitter the emitCExprNextState function instead of the emitCExpr function.  The emitCExprNextState function is
 * where the value to be written into the FIFO is stored into output variable to be handled by the core scheduler.  Since the actual data
 * movement of the FIFO is handled by the core schedulers, the emitCStateUpdate function does nothing.  As such, this block does not actually
 * need to create a state update node (like the delay or EnabledOutput nodes do).
 * Nodes with state (and no combinational path) also have their output arcs removed before scheduling begins.  This prevents cycles
 * from forming in the schedule and allows different pipeline stages to be scheduled independtly.
 */
class ThreadCrossingFIFO : public Node {
    friend NodeFactory;
protected:
    int fifoLength; ///<The length of this FIFO (in blocks)
    std::vector<std::vector<NumericValue>> initConditions; ///<Initial values for this FIFO (in elements).  The FIFO will be initialized to contain these values.  Size must be a multiple of the block size.  The outer vector is across input/output port pairs.  The inner vector contains the initial conditions for the given port pair.
    int blockSize; ///<The block size (in elements) of transactions to/from FIFO
    std::vector<Variable> cStateVars; ///<The C variables from which values are read.  There is one per input/output port pair
    std::vector<Variable> cStateInputVars; ///<the C temporary variables holding data to be writen.  There is one per input/output pair

    //TODO: Possibly re-factor.  Could make part of the cEmit function.  However, only 2 types of nodes need to know about it: InputMaster and ThreadCrossingFIFOs
    //Special casing may be preferrable for now
    std::string cBlockIndexVarInputName; ///The C variable used in the compute loop for indexing into a block when the FIFO is used as an input.  Is set by the emitter
    std::string cBlockIndexVarOutputName; ///The C variable used in the compute loop for indexing into a block when the FIFO is used as an output.  Is set by the emitter

    std::vector<bool> cStateVarsInitialized;
    std::vector<bool> cStateInputVarsInitialized;

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
    //====Getters/Setters====
    int getFifoLength() const;
    void setFifoLength(int fifoLength);

    std::vector<std::vector<NumericValue>> getInitConditions() const;
    void setInitConditions(const std::vector<std::vector<NumericValue>> &initConditions);

    /**
     * @brief
     * @param port
     * @return
     *
     * @note If the initConditions vector does not contain an entry for the specified port, entries will be inserted into the vector until an entry has been allocated for the port
     */
    std::vector<NumericValue> getInitConditionsCreateIfNot(int port);
    void setInitConditionsCreateIfNot(int port, const std::vector<NumericValue> &initConditions);

    std::string getCBlockIndexVarInputName() const;
    void setCBlockIndexVarInputName(const std::string &cBlockIndexVarName);

    std::string getCBlockIndexVarOutputName() const;
    void setCBlockIndexVarOutputName(const std::string &cBlockIndexVarName);

    /**
     * @brief Gets the cStateVar for this FIFO.  If it has not yet been initialized, it will be initialized at this point
     * @return the initialized cStateVar for this FIFO
     */
    Variable getCStateVar(int port);
    void setCStateVar(int port, const Variable &cStateVar);

    /**
     * @brief Gets the cStateInputVar for this FIFO.  If it has not yet been initialized, it will be initialized at this point
     * @return the initialized cStateInputVar for this FIFO
     */
    Variable getCStateInputVar(int port);
    void setCStateInputVar(int port, const Variable &cStateInputVar);

    int getBlockSize() const;
    void setBlockSize(int blockSize);

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
     * @brief Returns nothing, state is allocated seperatly during thread creation
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
    void emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType) override;

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
     * @returns a C statements that evaluates to true if the FIFO is not empty and to false if the FIFO is empty
     */
    virtual std::string emitCIsNotEmpty() = 0;

    /**
     * @brief Emits a C statement to check if the FIFO is not full (has space for 1 or more blocks)
     * @returns a C statements that evaluates to true if the FIFO is not full and to false if the FIFO is full
     */
    virtual std::string emitCIsNotFull() = 0;

    /**
     * @brief Emits a C statement to get the number of blocks available to read from the FIFO
     * @returns a C statement to get the number of blocks available to read from the FIFO
     */
    virtual std::string emitCNumBlocksAvailToRead() = 0;

    /**
     * @brief Emits a C statement to get the number of blocks that can be written into the FIFO at this time
     * @returns a C statement to get the number of blocks that can be written into the FIFO at this time
     */
    virtual std::string emitCNumBlocksAvailToWrite() = 0;

    /**
     * @brief Emits C statements which write data into the FIFO.
     * @param cStatementQueue The C statements are written into this queue
     * @param src The src for data to be written into the FIFO
     * @param numBlocks The number of blocks of data to write into the FIFO
     */
    virtual void emitCWriteToFIFO(std::vector<std::string> &cStatementQueue, std::string src, int numBlocks) = 0;

    /**
     * @brief Emits C statements which read data from the FIFO
     * @param cStatementQueue The C statements are written into this queue
     * @param dst The destination for data which will be read from the FIFO
     * @param numBlocks The number of blocks to read
     */
    virtual void emitCReadFromFIFO(std::vector<std::string> &cStatementQueue, std::string dst, int numBlocks) = 0;

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
     */
    virtual void createSharedVariables(std::vector<std::string> &cStatementQueue) = 0;

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
};


/*! @} */

#endif //VITIS_THREADCROSSINGFIFO_H
