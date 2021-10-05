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
#include "Variable.h"
#include "MultiThread/ThreadCrossingFIFOParameters.h"
#include "MultiThread/PartitionParams.h"
#include "Emitter/MultiThreadEmit.h"

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
    void addTopLevelContextRoot(std::shared_ptr<ContextRoot> contextRoot);
    void removeTopLevelNode(std::shared_ptr<Node> node);
    void removeTopLevelContextRoot(std::shared_ptr<ContextRoot> contextRoot);

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

    //Note, STL types take 2 classes in their template, the class contained and an allocator.  Need to declare the template template type to have 2 classes
    //Need to include allocators for both nodes and arcs.  Unfortunately, sets, which are also used in the program, have 3 template parameters.  So,
    //the same template template will no work for both std::vector and std::set
    //For now, will just create 2 duplicate functions, one for vectors and one for sets

    /**
     * @brief This utility functions adds/removes nodes and arcs supplied in arrays
     *
     * New nodes are added before old nodes are deleted
     * New nodes with parent set to nullptr are added as top level nodes
     *
     * New arcs are added before old arcs are deleted
     *
     * @param new_nodes set of nodes to add to the design
     * @param deleted_nodes set of nodes to remove from the design
     * @param new_arcs set of arcs to add to the design
     * @param deleted_arcs set of arcs to remove from the design
     */
    void addRemoveNodesAndArcs(std::set<std::shared_ptr<Node>> &new_nodes,
                               std::set<std::shared_ptr<Node>> &deleted_nodes,
                               std::set<std::shared_ptr<Arc>> &new_arcs,
                               std::set<std::shared_ptr<Arc>> &deleted_arcs);

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
    std::set<std::shared_ptr<Node>> getMasterNodes() const;
    std::vector<std::shared_ptr<Node>> getTopLevelNodes() const;
    void setTopLevelNodes(const std::vector<std::shared_ptr<Node>> topLevelNodes);
    std::vector<std::shared_ptr<Node>> getNodes() const;
    void setNodes(const std::vector<std::shared_ptr<Node>> nodes);
    std::vector<std::shared_ptr<Arc>> getArcs() const;
    void setArcs(const std::vector<std::shared_ptr<Arc>> arcs);
    std::vector<std::shared_ptr<ContextRoot>> getTopLevelContextRoots() const;
    void setTopLevelContextRoots(const std::vector<std::shared_ptr<ContextRoot>> topLevelContextRoots);


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
     * @warning Assumes block sizes are set in the master nodes
     *
     * @return a vector of input variables ordered by the input port number.  Their types are scaled for blocking
     */
    std::vector<Variable> getCInputVariables();

    /**
     * @brief Get the output variables for this design
     *
     * The input names take the form: portName_portNum
     *
     * @warning Assumes the design has already been validated (ie. has at least one arc per port).
     * @warning Assumes block sizes are set in the master nodes
     *
     * @return a vector of input variables ordered by the input port number.  Their types are scaled for blocking
     */
    std::vector<Variable> getCOutputVariables();

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
    std::string getCOutputStructDefn();


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

    std::set<int> listPresentPartitions();

    void validateNodes();

    /**
     * @brief Clears the discovered context info.  Includes clearing the top level context list, as well as running through the nodes in the design, clearing context sets and context stacks.
     */
    void clearContextDiscoveryInfo();
};

/*! @} */

#endif //VITIS_DESIGN_H
