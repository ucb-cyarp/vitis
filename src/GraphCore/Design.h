//
// Created by Christopher Yarp on 6/27/18.
//

#ifndef VITIS_DESIGN_H
#define VITIS_DESIGN_H

#include <memory>
#include <vector>
#include <set>
#include <map>

#include "GraphMLParameter.h"
#include "General/GeneralHelper.h"
#include "General/TopologicalSortParameters.h"
#include "Variable.h"
#include "SchedParams.h"
#include "MultiThread/ThreadCrossingFIFOParameters.h"
#include "MultiThread/PartitionParams.h"
#include "MultiThread/MultiThreadEmitterHelpers.h"

#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/PlatformUtils.hpp>

//Forward Declaration of Classes
class MasterInput;
class MasterOutput;
class MasterUnconnected;
class Node;
class SubSystem;
class Arc;
class ContextRoot;
class BlackBox;
class ContextFamilyContainer;
class ContextContainer;
class ThreadCrossingFIFO;
class ClockDomain;

//This Class

/**
* \addtogroup GraphCore Graph Core
*
* @brief Core Classes for Representing a Data Flow Graph
* @{
*/

/**
* @brief Represents a Streaming DSP Flow Graph Design
*
* This class acts as a container for a streaming DSP flow graph design.  It contains references to the various master
* nodes which represents the inputs, outputs, visualizers, termininators, and unconnected.  It also contains vectors
* of pointers to the nodes and arcs in the design.  This allows the design to contain nodes which are not reachable
* from the master nodes.  These unreachable nodes may be pruned or may be kept.
*/
class Design {
private:
    std::shared_ptr<MasterInput> inputMaster; ///< A pointer to the input master node of the design, representing the inputs to the design
    std::shared_ptr<MasterOutput> outputMaster; ///< A pointer to the output master node of the design, representing the outputs from the design
    std::shared_ptr<MasterOutput> visMaster; ///< A pointer to the visualization master node of the deign, representing signals to be visualized
    std::shared_ptr<MasterUnconnected> unconnectedMaster; ///< A pointer to the unconnected master node of the design, representing unconnected ports in the design
    std::shared_ptr<MasterOutput> terminatorMaster; ///< A pointer to the unconnected master node of the design, representing terminated ports

    std::vector<std::shared_ptr<Node>> topLevelNodes;///< A vector of nodes at the top level of the design (ie. not under a subsystem)
    std::vector<std::shared_ptr<Node>> nodes; ///< A vector of nodes in the design
    std::vector<std::shared_ptr<Arc>> arcs; ///< A vector of arcs in the design

    std::vector<std::shared_ptr<ContextRoot>> topLevelContextRoots; //A vector of the top level context roots for the design.  Top level includes subsystems that do not create contexts

public:
    /**
     * @brief Constructs a design object, creating master nodes in the process.
     *
     * The vectors are initialized using the default method (not explicitly initialized by the constructor).
     */
    Design();

    void addNode(std::shared_ptr<Node> node);
    void addTopLevelNode(std::shared_ptr<Node> node);
    void addArc(std::shared_ptr<Arc> arc);

    /**
     * @brief Re-number node IDs
     *
     * @note The node IDs may change in node that previously had valid IDs
     */
    void reNumberNodeIDs();

    /**
     * @brief Assigns node IDs to nodes which have an ID < 0
     *
     * Finds the maximum ID of all nodes in the design and begins numbering from there
     */
    void assignNodeIDs();

    /**
     * @brief Re-number arc IDs
     *
     * @note The arc IDs may change in node that previously had valid IDs
     */
    void reNumberArcIDs();

    /**
     * @brief Assigns arc IDs to nodes which have an ID < 0
     *
     * Finds the maximum ID of all arcs in the design and begins numbering from there
     */
    void assignArcIDs();

    /**
     * @brief Emits the given design as GraphML XML
     *
     * @note: @ref assignNodeIDs and @ref assignArcIDs should be called before this is called.  Otherwise, there may be name colissions.
     * This function does not call them to allow for either assignment or complete re-numbering to occur based on the situation.
     *
     * @param doc the XML document for the design to emit into
     * @param root root XML element of document
     */
    void emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* root);

    /**
     * @brief Get a set of GraphML parameters for this design by querying the nodes within it
     * @return set of GraphML parameters for this design
     */
    std::set<GraphMLParameter> graphMLParameters();

    /**
     * @brief This utility functions adds/removes nodes and arcs supplied in arrays
     *
     * New nodes are added before old nodes are deleted
     * New nodes with parent set to nullptr are added as top level nodes
     *
     * New arcs are added before old arcs are deleted
     *
     * @param new_nodes vector of nodes to add to the design
     * @param deleted_nodes vector of nodes to remove from the design
     * @param new_arcs vector of arcs to add to the design
     * @param deleted_arcs vector of arcs to remove from the design
     */
    void addRemoveNodesAndArcs(std::vector<std::shared_ptr<Node>> &new_nodes,
                               std::vector<std::shared_ptr<Node>> &deleted_nodes,
                               std::vector<std::shared_ptr<Arc>> &new_arcs,
                               std::vector<std::shared_ptr<Arc>> &deleted_arcs);

    /**
     * @brief Check if any of the nodes in the design can be expanded
     * @return true if any node in the design can be expanded, false if all nodes cannot be expanded
     */
    bool canExpand();

    /**
     * @brief Expand the design
     *
     * Traverse each node in the design and call the Node::expand function
     *
     * @note Expanded nodes may produce nodes that can further be expanded.  May need to call this function multiple times to get a full expansion.
     *
     * @return true if a node expanded, false if no node expanded
     */
    bool expand();

    /**
     * @brief Expand the design until it cannot be expanded further
     *
     * Calls expand() until it returns false
     *
     * @note Assumes that once expand() returns false, the design cannot be expanded further no matter how many times expand() is called
     *
     * @return true if an expansion was made, false if no node was expanded
     */
    bool expandToPrimitive();

    //==== Getters/Setters ====
    std::shared_ptr<MasterInput> getInputMaster() const;
    void setInputMaster(const std::shared_ptr<MasterInput> inputMaster);
    std::shared_ptr<MasterOutput> getOutputMaster() const;
    void setOutputMaster(const std::shared_ptr<MasterOutput> outputMaster);
    std::shared_ptr<MasterOutput> getVisMaster() const;
    void setVisMaster(const std::shared_ptr<MasterOutput> visMaster);
    std::shared_ptr<MasterUnconnected> getUnconnectedMaster() const;
    void setUnconnectedMaster(const std::shared_ptr<MasterUnconnected> unconnectedMaster);
    std::shared_ptr<MasterOutput> getTerminatorMaster() const;
    void setTerminatorMaster(const std::shared_ptr<MasterOutput> terminatorMaster);
    std::vector<std::shared_ptr<Node>> getTopLevelNodes() const;
    void setTopLevelNodes(const std::vector<std::shared_ptr<Node>> topLevelNodes);
    std::vector<std::shared_ptr<Node>> getNodes() const;
    void setNodes(const std::vector<std::shared_ptr<Node>> nodes);
    std::vector<std::shared_ptr<Arc>> getArcs() const;
    void setArcs(const std::vector<std::shared_ptr<Arc>> arcs);

    /**
     * @brief Copy a design
     * Copies the nodes and arcs of this design and provides maps linking the original nodes/arcs and the copies.
     *
     * @param origToCopyNode A map of original nodes to the copies, which is populated during the copy
     * @param copyToOrigNode A map of node copies to the originals, which is populated during the copy
     * @param origToCopyArc A map of original arcs to the copies, which is populated during the copy
     * @param copyToOrigArc A map of arc copies to the originals, nodes which is populated during the copy
     * @return A copy of the design
     */
    Design copyGraph(std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode, std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &origToCopyArc, std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &copyToOrigArc);

    /**
     * @brief Removes a node from the design
     *
     * Removes all arcs connected the node from the design as well
     *
     * @note This function is also capable of removing master nodes from the design.  If this occurs, the pointer to that node in the design is set to nullptr
     *
     * @param node node to remove from design
     */
    void removeNode(std::shared_ptr<Node> node);

    /**
     * @brief Prunes the design
     *
     * Removes unused nodes from the graph.
     *
     * Tracks nodes with 0 out degree (when connections to the Unconnected and Terminated masters are not counted)
     *
     * The unused and terminated masters are not removed during pruning.  However, nodes connected to them may be removed.
     *
     * Ports that are left unused are connected to the Unconnected master node
     *
     * @param includeVisMaster If true, will add the Vis master to the set of nodes that are not considered when calculating output degree.
     *
     * @return number of nodes removed from the graph
     */
    unsigned long prune(bool includeVisMaster = true);

    /**
     * @brief Schedule the nodes using topological sort.
     *
     * @param params the parameters used by the scheduler (ex. what heuristic to use, random seed (if applicable))
     * @param prune if true, prune the design before scheduling.  Pruned nodes will not be scheduled but will also not be removed from the origional graph.
     * @param rewireContexts if true, arcs between a node outside a context to a node inside a context are rewired to the context itself (for scheduling, the origional is left untouched).  If false, no rewiring operation is made for scheduling
     *
     * @param designName the name of the design (used when dumping a partially scheduled graph in the event an error is encountered)
     * @param dir the directory to dump any error report files into
     * @param printNodeSched if true, print the node schedule to the console
     * @param schedulePartitions if true, each partition in the design is scheduled seperatly
     *
     * @warning: UpsampleClockDomains currently rely on all of their nodes being scheduled together (ie not being split up).
     * This currently is provided by the hierarchical implementation of the scheduler.  However, if this were to be changed
     * later, a method for having vector intermediates would be required.
     *
     * @return the number of nodes pruned (if prune is true)
     */
    unsigned long scheduleTopologicalStort(TopologicalSortParameters params, bool prune, bool rewireContexts, std::string designName, std::string dir, bool printNodeSched, bool schedulePartitions);

    /**
     * @brief Topological sort the current graph.
     *
     * @warning This destroys the graph by removing arcs from the nodes.
     * It is reccomended to run on a copy of the graph and to back propagate the results
     *
     * @param designName name of the design used in the working graph export at part of the file name
     * @param dir location to export working graph in the event of a cycle or error
     * @param params the parameters used by the scheduler (ex. what heuristic to use, random seed (if applicable))
     *
     * @return A vector of nodes arranged in topological order
     */
    std::vector<std::shared_ptr<Node>> topologicalSortDestructive(std::string designName, std::string dir, TopologicalSortParameters params);

    /**
     * @brief Topological sort of a given partition in the the current graph.
     *
     * @warning This does not schedule the output node except for partition -2 (I/O)
     *
     * @warning This destroys the graph by removing arcs from the nodes.
     * It is reccomended to run on a copy of the graph and to back propagate the results
     *
     * @param designName name of the design used in the working graph export at part of the file name
     * @param dir location to export working graph in the event of a cycle or error
     * @param params the parameters used by the scheduler (ex. what heuristic to use, random seed (if applicable))
     * @param partition the partitions
     *
     * @return A vector of nodes arranged in topological order
     */
    std::vector<std::shared_ptr<Node>> topologicalSortDestructive(std::string designName, std::string dir, TopologicalSortParameters params, int partitionNum);

    /**
     * @brief Verify that the graph has topological ordering
     * @param checkOutputMaster if true, checks the schedule state of the output master
     */
    void verifyTopologicalOrder(bool checkOutputMaster, SchedParams::SchedType schedType);

    /**
     * @brief Get a node by its name path
     *
     * The name path would have the form {"subsys", "nested subsys", "nodeName"}
     *
     * The node list is not checked, rather the node hierarchy is traversed.
     *
     * @warning Assumes fully qualified node names are unique in the design.  If duplicates exist, only one of the 2 will be returned.
     *
     * @param namePath The name path of the node given as a vector from most general to most specific
     * @return a pointer to the node if it exists, nullptr if it cannot be found
     */
    std::shared_ptr<Node> getNodeByNamePath(std::vector<std::string> namePath);

    /**
     * @brief Get the input variables for this design
     *
     * The input names take the form: portName_portNum
     *
     * @warning Assumes the design has already been validated (ie. has at least one arc per port).
     *
     * @return a vector of input variables ordered by the input port number along with the rate of the input
     */
    std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> getCInputVariables();

    /**
     * @brief Get the output variables for this design
     *
     * The input names take the form: portName_portNum
     *
     * @warning Assumes the design has already been validated (ie. has at least one arc per port).
     *
     * @return a vector of input variables ordered by the input port number along with the rate of the input
     */
    std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> getCOutputVariables();

    /**
     * @brief Get the argument portion of the C function prototype for this design.
     *
     * For example, if there are 3 inputs to the system:
     *   - double realIn
     *   - double imagIn
     *   - bool pass
     *
     * The input names take the form: portName_portNum
     *
     * The function prototype would be designName(double In_0_re, double In_0_re, bool pass_1, OutputType *output, unsigned long *outputCount)
     *
     * This function returns "double In_0_re, double In_0_re, bool pass_1, OutputType *output, unsigned long *outputCount"
     *
     * Complex types are split into 2 arguments, each is prepended with _re or _im for real and imagionary component respectivly.
     *
     * The DataType is converted to the smallest standard CPU type that can contain the type
     *
     * @warning Assumes the design has already been validated (ie. has at least one arc per port).
     *
     * @param forceArray if true, forces the width of the type to be >1 which forces an array declaration
     *
     * @return argument portion of the C function prototype for this design
     */
    std::string getCFunctionArgPrototype(bool forceArray);

    /**
     * @brief Get the structure definition for the output type
     *
     * The struture definition takes the form of
     *
     * typedef struct OutputType{
     *     type1 var1;
     *     type2 var2;
     *     ...
     * }
     *
     * @param blockSize the block size (in samples).  The width is multiplied by this number
     * @return a string of the C structure definition
     */
    std::string getCOutputStructDefn(int blockSize = 1);

    /**
     * @brief Generates a Single Threaded Version of the Design as a C program
     *
     * Depending on the scheduler specified, this function will prune, schedule, and emit
     *
     * @param outputDir the output directory for the C files
     * @param designName the design name to be used as the C file names
     * @param schedType the type of scheduler to use when generating the C program
     * @param topoSortParams the parameters used by the topological scheduler if applicable (ex. what heuristic to use, random seed)
     * @param blockSize the size of the blocks (in samples) processed in each call to the generated C function
     * @param emitGraphMLSched if true, emit a GraphML file with the computed schedule as a node parameter
     * @param printNodeSched if true, print the node schedule to the console
     */
    void generateSingleThreadedC(std::string outputDir, std::string designName, SchedParams::SchedType schedType, TopologicalSortParameters topoSortParams, unsigned long blockSize, bool emitGraphMLSched, bool printNodeSched);

    /**
     * @brief Emits operators using the bottom up emitter
     * @param cFile the cFile to emit tp
     * @param nodesWithState nodes with state in the design
     * @param schedType the specific scheduler used
     */
    void emitSingleThreadedOpsBottomUp(std::ofstream &cFile, std::vector<std::shared_ptr<Node>> &nodesWithState, SchedParams::SchedType schedType);

    /**
     * @brief Emits operators using the schedule emitter
     * @param cFile the cFile to emit to
     * @param nodesWithState nodes with state in the design
     */
    void emitSingleThreadedOpsSched(std::ofstream &cFile, SchedParams::SchedType schedType);

    /**
     * @brief Emits operators using the schedule emitter.  This emitter is context aware and supports emitting scheduled state updates
     * @param cFile the cFile to emit to
     * @param blockSize the size of the block (in samples) that are processed in each call to the function
     * @param indVarName the variable that specifies the index in the block that is being computed
     */
    void emitSingleThreadedOpsSchedStateUpdateContext(std::ofstream &cFile, SchedParams::SchedType schedType, int blockSize = 1, std::string indVarName = "");

    /**
     * @brief Emits the design as a single threaded C function
     *
     * @note Design expansion and validation should be run before calling this function
     *
     * @note To avoid dead code being emitted, prune the design before calling this function
     *
     * @param path path to where the output files will be generated
     * @param fileName name of the output files (.h and a .c file will be created)
     * @param designName The name of the design (used as the function name)
     * @param schedType Schedule type
     * @param blockSize the size of the block (in samples) that is processed in each call to the emitted C function
     */
    void emitSingleThreadedC(std::string path, std::string fileName, std::string designName, SchedParams::SchedType schedType, unsigned long blockSize);

    /**
     * @brief Emits the design as a series of C functions.  Each (clock domain in) each partition is emitted as a C
     * function.  A function is created for each thread which includes the intra-thread scheduler.  This scheduler is
     * responsible for calling the partition functions as appropriate.  The scheduler also has access to the input and
     * output FIFOs for the thread and checks the inputs for the empty state and the outputs for the full state to decide
     * if functions should be executed.  A setup function will also be created which allocates the inter-thread
     * FIFOs, creates the threads, binds them to cores, and them starts execution.  Seperate function will be created
     * that enqueue data onto the input FIFOs (when not full) and dequeue data off the output FIFOs (when not empty).
     * Blocking and non-blocking variants of these functions will be created.  Benchmarking code should call the setup
     * function then interact with the input/output FIFO functions.  This will require the use of a core.
     *
     * @note (For framework devloper) The conditional statement about whether or not a context excutes needs to be made in each context it exists
     * in.
     *
     * @note Partitioning should have already occurred before calling this function.
     *
     * @note Design expansion and validation should be run before calling this function
     *
     * @note To avoid dead code being emitted, prune the design before calling this function
     *
     * @param path path to where the output files will be generated
     * @param fileName name of the output files (.h and a .c file will be created)
     * @param designName The name of the design (used as the function name)
     * @param schedType Schedule type
     * @param fifoLength the length of the FIFOs in blocks
     * @param blockSize the block size
     * @param propagatePartitionsFromSubsystems if true, propagates partition information from subsystems to children (from VITIS_PARTITION directives for example)
     * @param partitionMap a vector indicating the mapping of partitions to logical CPUs.  The first element is the I/O thread.  The subsequent entries are for partitions 0, 1, 2, .... If an empty array, I/O thread is placed on CPU0 and the other partitions are placed on the CPU that equals their partition number (ex. partition 1 is placed on CPU1)
     * @param threadDebugPrint if true, inserts print statements into the generated code which indicate the progress of the different threads as they execute
     * @param ioFifoSize the I/O FIFO size in blocks to allocate (only used for shared memory FIFO I/O)
     * @param printTelem if true, telemetry is printed
     * @param telemDumpPrefix if not empty, specifies a file prefix into which telemetry from each compute thread is dumped
     * @param telemLevel the level of telemetry collected
     * @param telemCheckBlockFreq the number of blocks between checking if the telemetry duration has elapsed.  Used to reduce how often the timer is checked
     * @param telemReportPeriodSec the (inexact) period in seconds between telemetry being reported.  Inexact because how often the reporting timer is checked is governed by telemCheckBlockFreq
     * @param memAlignment the aligment (in bytes) used for FIFO buffer allocation
     * @param useSCHEDFIFO if true, pthreads are created which will request to be run with the max RT priority under the linux SCHED_FIFO scheduler
     * @param fifoIndexCachingBehavior indicates when FIFOs check the head/tail pointers and when they rely on a priori information first
     * @param fifoDoubleBuffer indicates what FIFO double buffering behavior to use
     * @param pipeNameSuffix defines as a suffix to be appended to the names of POSIX Pipes or Shared Memory streams
     */
    void emitMultiThreadedC(std::string path, std::string fileName, std::string designName,
                            SchedParams::SchedType schedType, TopologicalSortParameters schedParams,
                            ThreadCrossingFIFOParameters::ThreadCrossingFIFOType fifoType,
                            bool emitGraphMLSched, bool printSched, int fifoLength, unsigned long blockSize,
                            bool propagatePartitionsFromSubsystems, std::vector<int> partitionMap,
                            bool threadDebugPrint, int ioFifoSize, bool printTelem, std::string telemDumpPrefix,
                            EmitterHelpers::TelemetryLevel telemLevel, int telemCheckBlockFreq, double telemReportPeriodSec,
                            unsigned long memAlignment, bool useSCHEDFIFO,
                            PartitionParams::FIFOIndexCachingBehavior fifoIndexCachingBehavior,
                            MultiThreadEmitterHelpers::ComputeIODoubleBufferType fifoDoubleBuffer,
                            std::string pipeNameSuffix);
    //TODO: update fifoLength to be set on a per FIFO basis

    /*
     * Discover Partitions
     *      The I/O in/out of the design needs to be handled in some way.  For now, we will probably want to keep them
     *      in one partition.  This will allow I/O to be handled in a single thread.  Therefore, the input and output
     *      ports should be assigned a partition.  It may be best for them to have their own partition.  OR perhaps, it
     *      would be better for them to be in the same partition as the blocks that directly use them.  However, this
     *      method would require I/O's impact on performance to be
     * Discover Partition Crossings
     * Discover Group-able Partition Crossings
     * Insert Inter-Partition FIFOs
     *      FIFO module will have a similar structure to a delay or latch (EnableOutput).  FIFOs are placed in the partition of the source
     *      because of the scheduling semantics for nodes with state.  They have no explicit state update emitter as this is handled by the
     *      core scheduler.  For now, all FIFOs
     *      are assumed to be at the boundaries
     * Simple re-timing
     *      We need to do proper re-timing but, as a first step, allow delays next to a FIFO to be re-timed into it if they are the
     *      only connection.  This works best if state update nodes are not created when the FIFOs are inserted.  TODO: check sequence.
     * Possible needed modifications:
     *     When creating context nodes (or expanding nodes), need to make sure they are in the same partition as their parent (except for FIFOs)
     *     This also applied to any nodes created during expansion.
     *          Partitioning will probably happen after expansion but it would be a good thing to include (ie. for the case when hand partitioning is performed)
     *          TODO: perhaps modify factory functions
     * Discover function prototypes for each partition (the I/O is the FIFO inputs)
     *     Can likley do this with
     * Identify which FIFOs are contained within contexts and what FIFO contains information about the context's execution mode
     *      Note: Only FIFOs entirely within a context may not need a value.
     *          Note: there is an optimization for inputs into a context where values do not need to be communicated to the next thread if the
     *          downstream context is is used in is not going to execute.
     *      For now, will assume that FIFOs to/from a context will need to be checked (with the possible exception of to enableOutput
     *      ports which, if not considered part of the
     * Emit the partition, replacing any FIFO output with a variable name.  Similar to Delays being split into a constant
     *      and a state update, FIFOs are separated into a receive and send portion.  The receive partition
     * As we had a choice in how to handle input/output FIFOs, there is also a choice in how to handle output FIFOs
     *      Could have the FIFO check occur as part of the function or outside of the function.
     *      For consistency with the inputs and for modularity, we will handle the output FIFOs outside of the function
     * Create scheduler for thread
     *      - Checks input FIFOs.  Context Dependency is respected in check
     *      - Execute function
     *      - Check output FIFOs for room, stall if necessary
     *      - Enqueue outputs (for outputs that were enabled)
     *      - Repeat
     * Create I/O thread
     *      - constant (benchmarking)
     *      - file (benchmarking)
     *      - initialized memory (benchmarking)
     *      - socket (get 10 GbE card and another system to act as playback)
     * Create global scheduler
     *      - create FIFOs
     *      - create and start threads (passing FIFOs to them)
     *      - create and start I/O threads
     *      - wait for I/O threads to finish or any errors
     *
     * TODO: Consider other FIFO access schemes:
     *      - Input FIFO as part of function.  Stall if empty.  Requires check dependency for contexts.
     *      - Output FIFO as part of function.  Stall if empty.
     *      - Output FIFO as part of function.  Check for space before function executes.  Allows scheduled write to occur interleaved with computation
     */

    /**
     * @brief Emits the benchmarking drivers for the design
     *
     * @note Design expansion and validation should be run before calling this function
     *
     * @param path path to where the output files will be generated
     * @param fileName base name of the output files (.h and a .c file will be created)
     * @param designName The name of the design (used as the function name)
     * @param blockSize the size of the block (in samples) that is processed in each call to the generated function
     */
    void emitSingleThreadedCBenchmarkingDrivers(std::string path, std::string fileName, std::string designName, int blockSize);

    /**
     * @brief Emits the benchmarking drivers (constant arguments) for the design
     *
     * @note Design expansion and validation should be run before calling this function
     *
     * @param path path to where the output files will be generated
     * @param fileName base name of the output files (.h and a .c file will be created)
     * @param designName The name of the design (used as the function name)
     * @param blockSize the size of the block (in samples) that is processed in each call to the generated function
     */
    void emitSingleThreadedCBenchmarkingDriverConst(std::string path, std::string fileName, std::string designName, int blockSize);

    /**
     * @brief Emits the benchmarking drivers (memory arguments) for the design
     *
     * @note Design expansion and validation should be run before calling this function
     *
     * @param path path to where the output files will be generated
     * @param fileName base name of the output files (.h and a .c file will be created)
     * @param designName The name of the design (used as the function name)
     * @param blockSize the size of the block (in samples) that is processed in each call to the generated function
     */
    void emitSingleThreadedCBenchmarkingDriverMem(std::string path, std::string fileName, std::string designName, int blockSize);

    /**
     * @brief Expands the enabled subsystems in the design to include combinational logic at the inputs and outputs of enabled subsystems
     *
     * This is an *optimization pass* that can be conducted before code emit.
     *
     * Note that expansion occurs hierarchically starting with the highest level and expanding any enabled subsystem within
     */
    void expandEnabledSubsystemContexts();

    /**
     * @brief It is possible that a design may want to visualize nodes inside of an enabled subsystem.  These nodes
     * do not typically have EnableOutput ports associated with them.  This function creates the appropriate EnableOutput
     * nodes for these visualizations.
     *
     * @note Only rewires standard arcs (not order constraint in arcs)
     *
     * @warning This function should be called after EnabledSubsystems are expanded but before state update nodes
     * are created as it does not check for the case where the visualization node becomes an ordered predecessor of a
     * state update node
     */
    void createEnabledOutputsForEnabledSubsysVisualization();

    /**
     * @brief Discover and mark contexts for nodes in the design (ie. sets the context stack of nodes).
     *
     * Propogates enabled subsystem contexts to its nodes (and recursively to nested enabled subsystems.
     *
     * Finds mux contexts under the top level and under each enabled subsystem
     *     Muxes are searched within the top level and within any subsystem.
     *
     *     Once all the Muxes within the top level (or within the level of the enabled subsystem are discovered)
     *     their contexts are found.  For each mux, we sum up the number of contexts which contain that given mux.
     *     Hierarchy is discovered by tracing decending layers of the hierarchy tree based on the number of contexts
     *     a mux is in.
     *
     * Since contexts stop at enabled subsystems, the process begins again with any enabled subsystem within (after muxes
     * have been handled)
     *
     * Also updates the topLevelContextRoots for discovered context nodes
     *
     */
    void discoverAndMarkContexts();

    /**
     * @brief Encapsulate contexts inside ContextContainers and ContextFamilyContainers for the purpose of scheduling
     * and inter-thread dependency handling for contexts
     *
     * Nodes should exist in the lowest level context they are a member of.
     *
     * Also moves the context roots for the ContextFamilyContainers inside the ContextFamilyContainer
     */
    void encapsulateContexts();

    void propagatePartitionsFromSubsystemsToChildren();

    /**
     * @brief Discovers nodes with state in the design and creates state update nodes for them.
     *
     * The state update nodes have a ordering dependency with all of the nodes that are connected via out arcs from the
     * nodes with state elements.  They are also dependent on the primary node being scheduled first (this is typically
     * when the next state update variable is assigned).
     *
     * @note This function updates the context of the the state update to mirror the context of the primary node.
     *
     * @note It appears that inserting these nodes before context discovery could result in some nodes being erroniously
     * left out of discoverd contexts.  For example, in mux context, nodes that are directly dependent on state will have
     * an order constraint arc to the StateUpdate node for the particular state element.  This StateUpdate is not reachable
     * from the mux and will therefore cause the node to not be included in the mux context even if it should be.  It
     * is therefore reccomended to insert the state update nodes after context discovery but before scheduling
     *
     * @param includeContext if true, the state update node is included in the same context (and partition) as the root node
     */
    void createStateUpdateNodes(bool includeContext);

    /**
     * @brief Discovers contextRoots in the design and creates ContextVariableUpdate update nodes for them.
     *
     * The state update nodes have a ordering dependency with all of the nodes that are connected via out arcs from the
     * nodes with state elements.  They are also dependent on the primary node being scheduled first (this is typically
     * when the next state update variable is assigned).
     *
     * @param includeContext if true, inserts the new contect varaible update nodes into the subcontext they reside in, otherwise does not (handled in a later step)
     */
    void createContextVariableUpdateNodes(bool includeContext = false);

    /**
     * @brief Find nodes with state in the design
     * @return a vector of nodes in the design with state
     */
    std::vector<std::shared_ptr<Node>> findNodesWithState();

    /**
     * @brief Find nodes with global declarations in the design
     * @return a vector of nodes in the design with global declarations
     */
    std::vector<std::shared_ptr<Node>> findNodesWithGlobalDecl();

    /**
     * @brief Find ContextRoots in the design
     * @return a vector of ContexRoots in the design
     */
    std::vector<std::shared_ptr<ContextRoot>> findContextRoots();

    /**
     * @brief Find BlackBox nodes in the design
     * @return a vector of BlackBox nodes in the design
     */
    std::vector<std::shared_ptr<BlackBox>> findBlackBoxes();

    /**
     * @brief Discovers partitions in the design and the nodes that are in each partition
     *
     * @note: Subsystems may not have a valid partition.  Partition -1 indicates that the node has not been placed in a
     * partition.
     *
     * @warning: Standard subsystems in partition -1 are not returned in the map
     *
     * @return A map of partitions to vectors of nodes in that partition
     */
    std::map<int, std::vector<std::shared_ptr<Node>>> findPartitions();

    /**
     * @brief Discovers directional crossings between nodes in different partitions
     *
     * This can be used to discover where inter-partition FIFOs are needed.
     *
     * @note: Not all inter-partition FIFOs need to be checked at all times.  For instance, if an enabled subsystem is
     * split between partitions, the FIFOs within the enabled subsystem may be empty for some cycles.  However, the
     * EnableOutput ports need to know that the subsystem was not enabled so that they can emit the last value.  One
     * method to alleviate this is to have a separate FIFO which caries information about whether or not the enabled
     * subsystem executed.  If it was, the scheduler needs to wait for data on the subsystem FIFOs.  If not, it can
     * proceed.
     *
     * Another special case to note is that a partitioned enabled subsystem may not have EnabledOutput ports in each
     * partition.  For instance, you can imagine an Enabled Subsystem being split into 3 partitions 1, 2, and 3 with the
     * inputs occurring in partition 1, and outputs occurring in partition 3, with paths running from 1->2 and 2->3.  Even
     * though the compute graph may not explicitally indicate the need for an arc from 1->2 indicating that the enabled
     * subsystem was active or not, one should be included anyway for the scheduler's benifit.  (The arc may exist in
     * the compute graph as a order constraint arc).
     *
     * The same issue exists for the mux contexts where a context may or may not execute.  The scheduler needs to know
     * about this.
     *
     * In general, the conditional statements for contexts need to be run in each partition they are present in.  This
     * may be inside the partition function itself an may not be expicitally handled by the scheduler.  The scheduler needs
     * to be aware of whether or not a context executes to decide if it needs to wait on data on particular FIFOs.  An
     * alternate to this is for the cross parition boundaries to be
     *
     * @warning: This function searches both direct and order constraint arcs.  This function may return crossings that do
     * not effectively exist because of these order constraints (ex. order constraint for state update).  It is advisable
     * to use this function before
     *
     * @param checkForToFromNoPartition if true, checks for arcs going to/from nodes in partition -1 and throws an error if this is encountered.  Arcs between 2 nodes in partition -1 will not cause an error
     *
     * @return A map of vectors which contain the arcs that go from the nodes in one partition to nodes in another partition.
     */
    std::map<std::pair<int, int>, std::vector<std::shared_ptr<Arc>>> getPartitionCrossings(bool checkForToFromNoPartition = true);

    /**
     * @brief Discovers partition crossing arcs that can be grouped together
     *
     * There may be cases where multiple arcs from a node in 1 partition go to several nodes inside of another
     * parition.  Instead of creating a FIFO for each arc, a single FIFO should be created with the fanout occuring within
     * the second parition.  This function finds arcs which can be combined into single FIFOs in this manner.
     *
     * @param checkForToFromNoPartition if true, checks for arcs going to/from nodes in partition -1 and throws an error if this is encountered.  Arcs between 2 nodes in partition -1 will not cause an error
     *
     * @return A map of vectors which contain groups arcs that go from the nodes in one partition to nodes in another partition.  Groups can contain one or several arcs
     */
    std::map<std::pair<int, int>, std::vector<std::vector<std::shared_ptr<Arc>>>> getGroupableCrossings(bool checkForToFromNoPartition = true);

    //TODO: Validate that mux contexts do not contain state elements

    /**
     * @brief OrderConstraint Nodes with Zero Inputs in Contexts
     *
     * This is to prevent nodes with 0 inputs in a context from being scheduled before the context drivers are computed
     *
     * @note Apply this after partitioning, replicating context drivers, and the creation of partition specific context drivers
     *
     * @note This is only needed if ContextFamilyNodes are not scheduled as a single unit.
     *
     */
    void orderConstrainZeroInputNodes(); //TODO: Use if scheduler changed to no longer schedule contexts as a single unit

    /**
     * @brief "Rewires" arcs that go between nodes in different contexts to be between contexts themselves (used primarily for scheduling)
     *
     * Goes through the array of the arcs in a design and rewires arcs that are between a nodes in different contexts.
     *
     * If the src node is at or above the dst node in the context hierarchy, then the src is not rewired. (Equiv to: dst node is at or below the src node -> src not rewired)
     * If the dst node is at or above the src node in the context hierarchy, then the dst is not rewired. (Equiv to: src node is at or below the dst node -> dst not rewired)
     *
     * Putting it another way
     * If the src node is either below the dst node node in the hierarchy or outside the dst node's hierarchy stack, then the src is rewired.
     * If the dst node is either below the src node node in the hierarchy or outside the dst node's hierarchy stack, then the dst is rewired.
     *
     * When rewiring the src, it is set to 1 context below the lowest common context of the src and dst on the src side.
     * When rewiring the dst, it is set to 1 context below the lowest common context of the src and dst on the dst side.
     *
     * @note: During encapsulation, which should occure before this is called, the context root driver arcs are
     * added as order constraint arcs for each ContextFamilyContainer created for a given ContextRoot.
     * For scheduling purposes, these order constraint arcs are what should be considered when scheduling rather
     * than the original context driver.  So, the origional context driver is disconnected by this function.
     *
     * @note This function does not actually rewire existing arcs but creates a new arcs representing how the given
     * arcs should be rewired.  The two returned vectors have a 1-many relationship between an arc to be rewired and the arcs
     * representing the result of that rewiring.  To rewire, simply disconnect the arcs from the origArcs vector.  The
     * arcs returned in contextArcs are already added to the design.
     *
     * Alternativly, the origArcs can be kept and used in the scheduler.  This may be helpful when deciding on whether
     * or not a context should be scheduled in the next iteration of the scheduler
     *
     */
    void rewireArcsToContexts(std::vector<std::shared_ptr<Arc>> &origArcs, std::vector<std::vector<std::shared_ptr<Arc>>> &contextArcs);

    /**
     * @brief Gets the ContextFamilyContainer for the provided context root if one exists (is in the ContextRoot's map of ContextFamilyContainers).
     *
     * If the ContextFamilyContainer does not exist for the given partition, one is created and is added to the ContextRoot's map of ContextFamilyContainers.
     * The existing ContextFamilyContainers for the given ContextRoot have their sibling maps updated.  The new ContextFamilyContainer has its sibling map set
     * to include all other ContextFamilyContainers belonging to the given ContextRoot
     *
     * When the ContextFamilyContainer is created, an order constraint arc from the context driver is created.  This ensures that the context driver
     * will be available in each partition that the context resides in (so long as FIFO insertion occurs after this point).
     *
     * @note In cases where a ContexRoot has multiple driver arcs that all come from the same source port (ex. EnabledSubsystems
     * where each EnableInput and EnableOutput has a driver arc), only a single order constraint port is created.
     *
     * @warning The created ContextFamilyContainer does not have its parent set.  This needs to be performed outside of this helper function
     *
     * @param contextRoot
     * @param partition
     * @return
     */
    std::shared_ptr<ContextFamilyContainer> getContextFamilyContainerCreateIfNotNoParent(std::shared_ptr<ContextRoot> contextRoot, int partition);

    std::set<int> listPresentPartitions();

    /**
     * @brief This searches the graph for subsystems which have no children (either because they were empty or all of their children had been moved) and can be removed.  Upon removal, it parent subsystem is also checked
     *
     * Note, subsystems that are context roots are not removed
     *
     * @return
     */
    void cleanupEmptyHierarchy(std::string reason);

    /**
     * @brief Replicates context root drivers if requested by the ContextRoot
     *
     * New driver arcs will be created as OrderConstraint arcs to the ContextRoot.
     *
     * The driver will have a duplicate copy created for the pariton it is in.  However,
     * a new OrderConstraint arc will be created for that same partition.
     *
     * @warning: This currently only support replicating a single node with no inputs
     * and a single output arc to the context root
     *
     * @warning: This should be done after context discovery but before encapsulation
     */
    void replicateContextRootDriversIfRequested();

    /**
     * @brief Specialize ClockDomains into UpsampleClockDomains or DownsampleClockDomains
     *
     * @returns a new vector of ClockDomain nodes since new nodes are created to replace each clock domain durring specialization.  Already specialized ClockDomains are returned unchanged
     */
    std::vector<std::shared_ptr<ClockDomain>> specializeClockDomains(std::vector<std::shared_ptr<ClockDomain>> clockDomains);

    /**
     * @brief Creates support nodes for ClockDomains
     */
    void createClockDomainSupportNodes(std::vector<std::shared_ptr<ClockDomain>> clockDomains, bool includeContext, bool includeOutputBridgingNodes);

    /**
     * @brief Find ClockDomains in design
     */
    std::vector<std::shared_ptr<ClockDomain>> findClockDomains();

    /**
     * @brief Resets the clock domain links in the design master nodes
     */
    void resetMasterNodeClockDomainLinks();

    /**
     * @brief Finds the clock domain rates for each partition in the design
     * @return
     */
    std::map<int, std::set<std::pair<int, int>>> findPartitionClockDomainRates();

    void validateNodes();

    /**
     * @brief Places EnableNodes that are not in any partition into a partition.
     * It will attempt to place EnableInput and EnableOutput nodes to be in the same partitions as the nodes they are
     * connected to within the enabled subsystem.  If EnableInputs go to multiple partitions, replicas will be created
     * for each destination partition
     *
     * This will potentially help avoid issues where partition cycles could
     * occur if the EnableInput and EnableOutput nodes took the partitions of the nodes outside of the enabled subsystem.
     * A case where this can occur is where partition A is used to compute the enable signal in context B and another
     * value from context A is passed to the EnabledSubsystem C.  In this case, there would be a cyclic dependency between
     * Partition A->B->A since the input port of the EnabledSubsystem would be placed in partition A.
     *
     * It is possible to be more intelligent solution with placement by checking for cycles when assigning EnableNodes to
     * partitions.
     * TODO: Implement a more intelligent scheme.
     *
     * The EnabledSubsystem node itself is checked to see if it is in a partition.  If not, it is placed in the same partition
     * as its first enable input.  If no enable inputs exist, it is placed in the partition of the first enable output.
     * Even though the EnabledSubsystem node itself is not used post encapsulation, it is not deleted because that node
     * contains the logic for generating the various context checks.  Placing it in a partition avoids a ContextContainer
     * being created for partition -1 (unassigned)
     */
    void placeEnableNodesInPartitions();

    /**
     * @brief Disconnect arcs to the unconnected, terminated, or vis masters
     * @param removeVisArcs if true, removes arcs connected to the vis masters.  If false, leaves these arcs alone
     */
    void pruneUnconnectedArcs(bool removeVisArcs);
};

/*! @} */

#endif //VITIS_DESIGN_H
