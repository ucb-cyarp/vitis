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

#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/PlatformUtils.hpp>

//Forward Declaration of Classes
class MasterInput;
class MasterOutput;
class MasterUnconnected;
class Node;
class Arc;
class ContextRoot;
class BlackBox;

//This Class

/**
* \addtogroup GraphCore Graph Core
*
* @brief Core Classes for Representing a Data Flow Graph
*/
/*@{*/

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
     *
     * @return the number of nodes pruned (if prune is true)
     */
    unsigned long scheduleTopologicalStort(TopologicalSortParameters params, bool prune, bool rewireContexts, std::string designName, std::string dir, bool printNodeSched);

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
     * @brief Verify that the graph has topological ordering
     */
    void verifyTopologicalOrder();

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
     * @return a vector of input variables ordered by the input port number
     */
    std::vector<Variable> getCInputVariables();

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
     * @brief Get the structure definition for the Input ports
     *
     * The struture definition takes the form of
     *
     * typedef struct Input{
     *     type1 var1;
     *     type2 var2;
     *     ...
     * }
     *
     * This is used by the driver generator.
     *
     * @param blockSize the block size (in samples).  The width is multiplied by this number
     *
     * @return
     */
    std::string getCInputPortStructDefn(int blockSize = 1);

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
     * @param nodesWithState nodes with state in the design
     * @param blockSize the size of the block (in samples) that are processed in each call to the function
     * @param indVarName the variable that specifies the index in the block that is being computed
     */
    void emitSingleThreadedOpsSchedStateUpdateContext(std::ofstream &cFile, SchedParams::SchedType schedType, int blockSize = 1, std::string indVarName = "");

    /**
     * @brief Emits the design as a single threaded C function (using the bottom-up method)
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
     * @brief Discover and mark contexts for nodes in the design.
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
     * Since contexts stop at enabled subsystems, the process begins again with any enabled subsystem within (after muxess
     * have been handled)
     *
     * Also updates the topLevelContextRoots for discovered context nodes
     *
     */
    void discoverAndMarkContexts();

    /**
     * @brief Encapsulate contexts inside ContextContainers and ContextFamilyContainers for the purpose of scheduling
     *
     * Nodes should exist in the lowest level context they are a member of.
     *
     * Also moves the context roots for the ContextFamilyContainers inside the ContextFamilyContainer
     */
    void encapsulateContexts();

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
     */
    void createStateUpdateNodes();

    /**
     * @brief Discovers contextRoots in the design and creates ContextVariableUpdate update nodes for them.
     *
     * The state update nodes have a ordering dependency with all of the nodes that are connected via out arcs from the
     * nodes with state elements.  They are also dependent on the primary node being scheduled first (this is typically
     * when the next state update variable is assigned).
     *
     */
    void createContextVariableUpdateNodes();

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

    //TODO: Validate that mux contexts do not contain state elements

    /**
     * @brief OrderConstraint Nodes with Zero Inputs in EnabledSubsystems (and nested within) in Design
     *
     * @note Apply this after expanding enabled sybsystem contexts
     */
    void orderConstrainZeroInputNodes();

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
     * Arcs that are the drivers of the contextRoot are also "rewired".  Note that there may be multiple driver arcs for
     * a context (ex. enabled subsystems have an arc to each EnabledInput and EnabledOutput).  In this case, there may
     * be a single rewired arc for several original arcs.  In this case, the same ptr to the rewired arc will be returned
     * for each of the corresponding original arcs.
     *
     * @note This function does not actually rewire existing arcs but creates a new arcs representing how the given
     * arcs should be rewired.  The two returned vectors have a 1-1 relationship between an arc to be rewired and an arc
     * representing the result of that rewiring.  To rewire, simply disconnect the arcs from the origArcs vector.  The
     * arcs returned in contextArcs are already added to the design.
     *
     * Alternativly, the origArcs can be kept and used in the scheduler.  This may be helpful when deciding on whether
     * or not a context should be scheduled in the next iteration of the scheduler
     *
     */
    void rewireArcsToContexts(std::vector<std::shared_ptr<Arc>> &origArcs, std::vector<std::shared_ptr<Arc>> &contextArcs);
};

/*@}*/

#endif //VITIS_DESIGN_H
