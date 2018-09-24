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

#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/PlatformUtils.hpp>

//Forward Declaration of Classes
class MasterInput;
class MasterOutput;
class MasterUnconnected;
class Node;
class Arc;

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
     * @param includeTerminatorsAndVis If true, will add the Vis master to the set of nodes that are not considered when calculating output degree.
     *
     * @return number of nodes removed from the graph
     */
    unsigned long prune(bool includeVisMaster = true);

    /**
     * @brief Schedule the nodes using topological sort.
     */
    void schedualTopologicalStort(); //TODO: copy the design, prune the design, remove master inputs & constants, remove outgoing arc from delays, call topological order, back propagate schedule

    /**
     * @brief Topological sort the current graph.
     *
     * @warning This destroys the graph by removing arcs from the nodes.
     * It is reccomended to run on a copy of the graph and to back propagate the results
     *
     * @return A vector of nodes arranged in topological order
     */
    std::vector<std::shared_ptr<Node>> topologicalSortDestructive();

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
     * @return argument portion of the C function prototype for this design
     */
    std::string getCFunctionArgPrototype();

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
     * This is used by the driver generator if arrays need to be defined and the .
     *
     * @return
     */
    std::string getCInputPortStructDefn();

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
     * @return
     */
    std::string getCOutputStructDefn();

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
     */
    void emitSingleThreadedC(std::string path, std::string fileName, std::string designName);

    /**
     * @brief Emits the benchmarking drivers for the design
     *
     * @note Design expansion and validation should be run before calling this function
     *
     * @param path path to where the output files will be generated
     * @param fileName base name of the output files (.h and a .c file will be created)
     * @param designName The name of the design (used as the function name)
     */
    void emitSingleThreadedCBenchmarkingDrivers(std::string path, std::string fileName, std::string designName);

    /**
     * @brief Emits the benchmarking drivers (constant arguments) for the design
     *
     * @note Design expansion and validation should be run before calling this function
     *
     * @param path path to where the output files will be generated
     * @param fileName base name of the output files (.h and a .c file will be created)
     * @param designName The name of the design (used as the function name)
     */
    void emitSingleThreadedCBenchmarkingDriverConst(std::string path, std::string fileName, std::string designName);

    /**
     * @brief Emits the benchmarking drivers (memory arguments) for the design
     *
     * @note Design expansion and validation should be run before calling this function
     *
     * @param path path to where the output files will be generated
     * @param fileName base name of the output files (.h and a .c file will be created)
     * @param designName The name of the design (used as the function name)
     */
    void emitSingleThreadedCBenchmarkingDriverMem(std::string path, std::string fileName, std::string designName);
};

/*@}*/

#endif //VITIS_DESIGN_H
