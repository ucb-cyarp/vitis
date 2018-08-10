//
// Created by Christopher Yarp on 6/25/18.
//

#ifndef VITIS_NODE_H
#define VITIS_NODE_H

//Forward Decls (Breaking Circular Dep)
//class Arc;
//class Port;
class NodeFactory;

class SubSystem;
class GraphMLParameter;

#include <vector>
#include <set>
#include <string>
#include <memory>
#include <map>

#include "Port.h"
#include "InputPort.h"
#include "OutputPort.h"
#include "Arc.h"
#include "GraphMLParameter.h"
#include "Variable.h"
#include "CExpr.h"

#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/PlatformUtils.hpp>

//#include "SubSystem.h"
//#include "GraphMLParameter.h"

//This Class

/**
* \addtogroup GraphCore Graph Core
*/
/*@{*/

/**
 * @brief Generic Node in a data flow graph DSP design
 *
 * This class represents a generic node in the data flow graph.  It is an abstract class which provides some basic
 * utility functions as well as stub functions which should be overwritten by subclasses.
 *
 * ## Ownership Semantics:
 *   - Nodes are owned buy the design, parent nodes (if applicable), and arcs
 *   - Arcs are owned by the design.
 *   - Ports are owned by nodes
 *   - Ports have weak pointers to arcs
 *
 * If the design removes an arc from its list, it is destroyed.  It removes it's weak ptrs from ports it is connected to to avoid stale pointer access from the port.
 * The arc relinquishes its partial ownership of the nodes it was connected to.
 * If the design relinquishes its partial ownership of the nodes as well, the only ownership is from parent nodes.
 * A cascading destruction will occur
 */
class Node : public std::enable_shared_from_this<Node> {
    friend class NodeFactory;

protected:
    std::string name; ///< An optional human readable name for the node
    int id; ///<Node ID number used when reading/writing GraphML files
    std::shared_ptr<SubSystem> parent; ///<Parent of this node
    //Vectors to unique pointers because we actually need to pass pointers to these ports around (esp to arcs).
    //Since vectors can re-allocate memory as items are added or deleted, the pointer location is not guaranteed to remain
    //in the same location.  Putting the ports in the heap and having the nodes have a sole ownership pointer to them which is
    //contained in the vector allows the location of the object to remain the same (making passing this valid) even if more
    //entries are added to the vector.  When the node is destroyed, the ports are destroyed as well due to the sole ownership.
    //shared_ptrs to the ports can be generated but these are aliased pointers that account for the shared ownership of the
    //nodes, they alias to the port.
    std::vector<std::unique_ptr<InputPort>> inputPorts; ///<The input ports of this node
    std::vector<std::unique_ptr<OutputPort>> outputPorts; ///<The output ports of this node

    int tmpCount; ///<Used to track how many temporary variables have been created for this node

    int partitionNum; ///<The partition set this node is contained within.  Used for multicore output

    //==== Constructors (Protected to force use of factory - required to handle  ====

    /**
     * @brief Constructs an empty node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @warning Because pointer (this) is passed to ports, nodes must be allocated on the heap and not moved.  All interaction should be via pointers.
     */
    Node();

    /**
     * @brief Constructs an empty node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @warning Because pointer (this) is passed to ports, nodes must be allocated on the heap and not moved.  All interaction should be via pointers.
     *
     * @param parent parent node
     */
    explicit Node(std::shared_ptr<SubSystem> parent);

    /**
     * @brief Initializes parts of the given object that rely on a shared_ptr to the object itself
     *
     * This is due to how enable_shared_from_this operates.  It requires a shared_ptr to exist before any calls to shared_from_this().
     */
    virtual void init();

public:
    //==== Functions ====
     /**
     * @brief Add an input arc to the given node, updating referenced objects in the process
     *
     * Adds an input arc to this node.  Removes the arc from the orig port if not NULL.
     * Updates the given arc so that the destination port is set to the specified input
     * port of this node.  If the port object for the given number is not yet created, it
     * will be created during the call.
     * @param portNum Input port number to add the arc to
     * @param arc The arc to add
     */
     void addInArcUpdatePrevUpdateArc(int portNum, std::shared_ptr<Arc> arc);

    /**
     * @brief Add an output arc to the given node, updating referenced objects in the process
     *
     * Adds an output arc to this node.  Removes the arc from the orig port if not NULL.
     * Updates the given arc so that the source port is set to the specified output
     * port of this node.  If the port object for the given number is not yet created, it
     * will be created during the call.
     * @param portNum Output port number to add arc to
     * @param arc The arc to add
     */
    void addOutArcUpdatePrevUpdateArc(int portNum, std::shared_ptr<Arc> arc);

    /**
     * @brief Removes the given input arc from the node. Does not change the dstPort entry in the arc.
     *
     * Will search all input ports to find the given arc
     * @param arc Arc to remove
     */
    void removeInArc(std::shared_ptr<Arc> arc);

    /**
     * @brief Removes the given output arc from the node.  Does not change the srcPort entry in the arc.
     *
     * Will search all output ports to find the given arc
     * @param arc Arc to remove
     */
    void removeOutArc(std::shared_ptr<Arc> arc);

    /**
     * @brief Get an aliased shared pointer to the specified input port of this node.  If no such port exists a null pointer is returned.
     *
     * The pointer is aliased with this node as the stored pointer.
     *
     * @param portNum the input port number
     * @return aliased shared pointer to input port or nullptr
     */
    std::shared_ptr<InputPort> getInputPort(int portNum);

    /**
     * @brief Get an aliased shared pointer to the specified output port of this node.  If no such port exists a null pointer is returned.
     *
     * The pointer is aliased with this node as the stored pointer.
     *
     * @param portNum the output port number
     * @return aliased shared pointer to output port or nullptr
     */
    std::shared_ptr<OutputPort> getOutputPort(int portNum);

    /**
     * @brief Get an aliased shared pointer to the specified input port of this node.  If no such port exists, create one
     *
     * The pointer is aliased with this node as the stored pointer.
     *
     * @param portNum the input port number
     * @return aliased shared pointer to input port
     */
    std::shared_ptr<InputPort> getInputPortCreateIfNot(int portNum);

    /**
     * @brief Get an aliased shared pointer to the specified output port of this node.  If no such port exists, create one
     *
     * The pointer is aliased with this node as the stored pointer.
     *
     * @param portNum the output port number
     * @return aliased shared pointer to output port
     */
    std::shared_ptr<OutputPort> getOutputPortCreateIfNot(int portNum);

    /**
     * @brief Get a vector of pointers to the current input ports of the node
     *
     * @note Ports may be added to the node later which will not be reflected in the returned array
     *
     * @return vector of aliased pointers to the current input ports of the node
     */
    std::vector<std::shared_ptr<InputPort>> getInputPorts();

    /**
     * @brief Get a vector of pointers to the current output ports of the node
     *
     * @note Ports may be added to the node later which will not be reflected in the returned array
     *
     * @return vector of aliased pointers to the current output ports of the node
     */
    std::vector<std::shared_ptr<OutputPort>> getOutputPorts();

    /**
     * @brief Get the full hierarchical path of this node in GraphML format
     *
     * A typical GraphML formatted hierarchy would be "n1::n2::n3"
     * @return Full hierarchical path of the given node as a std::string
     */
    std::string getFullGraphMLPath();

    /**
     * @brief Get the parent of this node
     * @return the parent of the node or nullptr if no parent exists
     */
    std::shared_ptr<SubSystem> getParent();

    /**
     * @brief Get the fully qualified human readable name of the node
     *
     * A typical fully qualified name would be "subsysName/nodeName"
     * @return Fully qualified human readable name of the node as a std::string
     */
    std::string getFullyQualifiedName();

    /**
     * @brief Get the node ID from a full GraphML ID path
     * @param fullPath the full GraphML ID path
     * @return local ID number
     */
    static int getIDFromGraphMLFullPath(std::string fullPath);

    /**
     * @brief Gets a shared pointer to this node
     *
     * Used in port objects to get a shared pointer to the parent which they then use to return an aliased shared pointer to themselves.
     * This is because the port objects are contained in the node.  When the node is destructed, they are too.
     * The nodes would not be deleted if ports had a shared object to the
     *
     * @return shared pointer to this node
     */
    std::shared_ptr<Node> getSharedPointer();

    /**
     * @brief Identifies which GraphML parameters this node (and its descendants) can emit
     * @return set of GraphML parameters which this node (and its descendants) can emit.
     *
     * @note The set may contain multiple parameters with the same key but different types.
     * Type conflicts must be resolved before emitting GraphML.
     */
    virtual std::set<GraphMLParameter> graphMLParameters();

    /**
     * @brief Emits the current node as GraphML
     *
     * Include Data node entries for parameters
     *
     * @param doc the XML document containing graphNode
     * @param graphNode the node representing the \<graph\> XMLNode that this node object is contained within
     * @param include_block_node_type if true, the data entry "block_node_type" is included in the output, if false it is not (used when the type should be superceeded - ex expanded node)
     *
     * @return pointer to this node's associated DOMNode
     */
    virtual xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) = 0;

    /**
     * @brief Emits the current node as GraphML node without inserting data entries that change depending on the node type
     *
     * creates the \<node id="n2"\> entry but nothing below that
     *
     * @param doc the XML document containing graphNode
     * @param graphNode the node representing the \<graph\> XMLNode that this node object is contained within
     *
     * @return pointer to this node's associated DOMNode
     */
    virtual xercesc::DOMElement* emitGraphMLBasics(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode);

    /**
     * @brief Expand this node if applicable
     *
     * Typical reasons for expansion include:
     * - High Level Blocks to Primitive Operations
     *     - FIR Filter
     *     - Tapped Delay Line
     *     - FFT
     * - Medium Level Blocks to Primitive Operations
     *     - Gain
     *
     * @param new_nodes A vector which will be filled with the new nodes created during expansion
     * @param deleted_nodes A vector which will be filled with the nodes deleted during expansion
     * @param new_arcs A vector which will be filled with the new arcs created during expansion
     * @param deleted_arcs A vector which will be filled with the arcs deleted during expansion
     * @return true if expansion occurred, false if it did not
     */
    virtual bool expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes, std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs);

    /**
     * @brief Identifies if the given input port experiences internal fanout in the node.
     *
     * Internal fanout occurs if an input is used more than once (either within the calculation of a single output or
     * or within the calculation of multiple outputs).
     *
     * @param inputPort the input port being checked for internal fanout
     * @param imag if false, checks the real component of the input. if true, checks the imag component of the input
     * @return true if the port experiences internal fanout, false otherwise
     */
    virtual bool hasInternalFanout(int inputPort, bool imag = false);

    /**
     * @brief Emit C code to calculate the value of an output port.
     *
     * If the output is dependant on the result of other operations, those dependant operations will be emitted
     * and enqueued on the cStatementQueue before the code for the given operation is returned.
     *
     * This function can also check if the output port is fanned out (by checking the number of out
     * ports or checking for internal fanout).  This check occurs by default but can be disabled.
     *
     * Fanout can also be forced which results in temporary variable creation
     *
     * @param cStatementQueue a reference to the queue containing C statements for the function being emitted.  Additional statements may be enqueued by this function
     * @param outputPortNum the output port for which the C++ code should be generated for
     * @param imag if false, emits the real component of the output. if true, emits the imag component of the output
     * @param checkFanout if true, checks if the output is used in a fanout.  If false, no check is performed
     * @param forceFanout if true, a temporary variable is always create, if false the value of checkFanout dictates whether or not a temporary variable is created
     * @return a string containing the C code for calculating the result of the output port
     */
    virtual std::string emitC(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag = false, bool checkFanout = true, bool forceFanout = false);

protected:
    //TODO: Make this virtual.  Giving a default implementation now which throws an error message if not overwritten
    /**
     * @brief Emits a C expression to calculate the value of an output port
     *
     * @note @ref emitC should be called from outside the class instead of this function.  @ref emitC automatically handles fanout while this function does not
     *
     * @param cStatementQueue a reference to the queue containing C statements for the function being emitted.  Additional statements may be enqueued by this function
     * @param outputPortNum the output port for which the C++ code should be generated for
     * @param imag if false, emits the real component of the output. if true, emits the imag component of the output
     * @return a string containing the C expression for calculating the result of the output port
     */
    virtual CExpr emitCExpr(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag = false);

    /**
     * @brief Get a new temporary variable with the specified DataType
     *
     * @note @ref DataType::getCPUStorageType is called on the dataType to get a standard CPU type
     *
     * The C variable declaration of the new temporary variable is enqueued on the cStatementQueue
     *
     * The variable name has the form nodeName_n<id>_tmp<num>
     *
     * @param cStatementQueue a reference to the queue containing C statements for the function being emitted.  The temporary variable declaration is pushed onto this queue
     * @param dataType the datatype of the new temporary variable
     * @return a new Variable object
     */
    Variable getNewTempVar(std::vector<std::string> &cStatementQueue, DataType dataType);

public:
    /**
     * @brief Identifies if the node contains state elements
     *
     * @return true if the node contains state elements, false if it does not contain state elements
     */
    virtual bool hasState();

    /**
     * @brief Get a list of state variables used by this node which must be declared by the emitter as thread local
     *
     * If the node has multiple output ports, this function checks which ports are connected to the Unconnected Master
     * node and does not return state variables that are solely used by those outputs.
     *
     * @note Graph pruning should take place before this function is called to avoid emitting thread local variables for
     * pruned nodes.
     *
     * @return a list of state variables used by this node
     */
    virtual std::vector<Variable> getCStateVars();

    /**
     * @brief Emits the C code to update the state varibles of this node.
     *
     * This code should be called at the end of the emit for the given function.
     *
     * @param cStatementQueue a reference to the queue containing C statements for the function being emitted.  The state update statements for this node are enqueued onto this queue
     */
    virtual void emitCStateUpdate(std::vector<std::string> &cStatementQueue);

    /**
     * @brief Get a human readable description of the node
     * @return human readable description of node
     */
    virtual std::string labelStr();

    /**
     * @brief Validate if the node has a valid configuration.
     * This may involve looking at arcs or nodes connected to this node.
     *
     * If an invalid configuration is detected, the function will throw an exception
     */
    virtual void validate();

    /**
     * @brief Propagates properties from Arcs (and potentially connected nodes) into this node.  Should only be called after nodes and arcs are imported
     */
    virtual void propagateProperties();

    /**
     * @brief Check if the node can be expanded (to primitive nodes).
     *
     * @note Does not check for vector expansion.
     *
     * The @ref expand method can still be called even if this function returns false, however, no change to the design will be made
     * @return true if the node can be expanded, false otherwise
     */
    virtual bool canExpand() = 0;

    //==== Getters/Setters ====
    /**
     * @brief Sets the parent of the node without updating the child set of the parent to include this node.
     *
     * Consider using @ref updateParent which updates the parents (old and new) along with changing the parent of this node.
     *
     * @param parent the new parent of this node
     */
    void setParent(std::shared_ptr<SubSystem> parent);

    int getId() const;

    void setId(int id);

    const std::string &getName() const;

    void setName(const std::string &name);

    //++++ Getters/Setters With Added Functionality ++++
    /**
     * @brief Sets the parent of the node.
     *
     * Before setting, removes from the children list of the old parent (if not NULL).
     * Adds to the children list of the new parent (if not NULL).
     * @param parent new parent
     */
    void updateParent(std::shared_ptr<SubSystem> parent);

};

/*@}*/

#endif //VITIS_NODE_H
