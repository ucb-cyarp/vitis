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
class ExpandedNode;
class GraphMLParameter;
class StateUpdate;
class MasterUnconnected;

#include <vector>
#include <set>
#include <string>
#include <memory>
#include <map>

#include "Port.h"
#include "InputPort.h"
#include "OutputPort.h"
#include "OrderConstraintInputPort.h"
#include "OrderConstraintOutputPort.h"
#include "Arc.h"
#include "GraphMLParameter.h"
#include "Variable.h"
#include "CExpr.h"
#include "Context.h"
#include "SchedParams.h"

#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/PlatformUtils.hpp>

//#include "SubSystem.h"
//#include "GraphMLParameter.h"

/**
* \addtogroup GraphCore Graph Core
* @{
*/

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

public:
    /**
     * @brief Class for comparing std::shared_ptr<Node> pointers by their IDs rather than by the pointer address.  Is useful when order consistency between runs is important (ptr addresses will almost certainly be different between runs)
     *
     * @note If IDs are equal (for example if they have not yet been assigned), the ptr address is used as the tie breaker.
     */
    struct PtrID_Compare{
        bool operator() (const std::shared_ptr<Node>& lhs, const std::shared_ptr<Node>& rhs) const;
    };

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

    std::unique_ptr<OrderConstraintInputPort> orderConstraintInputPort; ///<Port which is the destination for arcs creating scheduling ordering constraints between nodes when the output is not directly consumed by this node
    std::unique_ptr<OrderConstraintOutputPort> orderConstraintOutputPort;


    //TODO: Add Setters/Getters (check if port exists first), Update in degree, Update connected nodes, Update disconnect

    int tmpCount; ///<Used to track how many temporary variables have been created for this node

    int partitionNum; ///<The partition set this node is contained within.  Used for multicore output
    int schedOrder; ///<Durring scheduled emit, nodes are emitted in decending schedOrder within a given partition.  Defaults to -1 (unscheduled)
    std::vector<Context> context; ///<A stack of contexts this node resides in.  The most specific context has the highest index.  Pushes onto the back of the stack and pops from the back of the stack.
    std::shared_ptr<StateUpdate> stateUpdateNode; ///<A reference to the state update node for this delay

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
    Node(std::shared_ptr<SubSystem> parent, Node* orig);

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
     * @brief Add a schedule ordering constraint arc to the given node, updating referenced objects in the process
     *
     * Adds an schedule ordering constraint arc to this node.  Removes the arc from the orig port if not NULL.
     * Updates the given arc so that the source port is set to the specified output
     * port of this node.  If the port object is not yet created, it will be created during the call.
     * @param arc The arc to add
     */
    void addOrderConstraintInArcUpdatePrevUpdateArc(std::shared_ptr<Arc> arc);

    /**
     * @brief Add a schedule ordering constraint arc to the given node, updating referenced objects in the process
     *
     * Adds an schedule ordering constraint arc to this node.  Removes the arc from the orig port if not NULL.
     * Updates the given arc so that the source port is set to the specified output
     * port of this node.  If the port object is not yet created, it will be created during the call.
     * @param arc The arc to add
     */
    void addOrderConstraintOutArcUpdatePrevUpdateArc(std::shared_ptr<Arc> arc);

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
     * @brief Removes the given OrderConstraintInArc arc from the node.  Does not change the dstPort entry in the arc.
     *
     * @param arc Arc to remove
     */
    void removeOrderConstraintInArc(std::shared_ptr<Arc> arc);

    /**
     * @brief Removes the given OrderConstraintOutArc arc from the node.  Does not change the srcPort entry in the arc.
     *
     * @param arc Arc to remove
     */
    void removeOrderConstraintOutArc(std::shared_ptr<Arc> arc);

    /**
     * @brief Gets a copy of the node's context stack
     * @return A copy of the node's context stack
     */
    std::vector<Context> getContext() const;

    /**
     * @brief Set the context stack of the node
     * @param context the context stack
     */
    void setContext(std::vector<Context> &context);

    /**
     * @brief Gets the context at index i in the stack
     * @param i the index into the context stack to fetch from
     * @return context[i]
     */
    Context getContext(unsigned long i) const;

    /**
     * @brief Sets the context at index i in the context stack
     * @param i index of the context stack to set
     * @param context context to set
     */
    void setContext(unsigned long i, Context &context);

    /**
     * @brief Pushes a new context onto the back of the context stack
     * @param context context to push onto the context stack
     */
    void pushBackContext(Context &context);

    /**
     * @brief Removes the given context from the context stack
     * @param context the context to remove
     */
    void removeContext(Context &context);

    /**
     * @brief Removes context[i] from the context stack
     * @param i the index to remove
     */
    void removeContext(unsigned long i);

    /**
     * @brief Disconnects the node from the graph.
     *
     * Disconnects all arcs from this node.  All arcs are disconnected from both the source and destination and should be safe to delete
     *
     * @return A list of arcs that have been disconnected from this node and the connected node.  These arcs should be safe to delete
     */
    std::set<std::shared_ptr<Arc>> disconnectNode();

    /**
     * @brief Disconnects the node from nodes it is (directly and virtually) dependent on.
     *
     * Disconnects all input arcs from this node.  All arcs are disconnected from both the source and destination and should be safe to delete
     *
     * @note This also disconnects from the sources of OrderConstraint arcs.
     *
     * @return A list of arcs that have been disconnected from this node and the connected node.  These arcs should be safe to delete
     */
    virtual std::set<std::shared_ptr<Arc>> disconnectInputs();

    /**
     * @brief Disconnects the node from nodes it is (directly) dependent on.
     *
     * Disconnects all input arcs from this node.  All arcs are disconnected from both the source and destination and should be safe to delete
     *
     * @note This also disconnects from the sources of OrderConstraint arcs.
     *
     * @return A list of arcs that have been disconnected from this node and the connected node.  These arcs should be safe to delete
     */
    virtual std::set<std::shared_ptr<Arc>> disconnectDirectInputs();

    /**
     * @brief Disconnects the node from nodes it is virtually dependent on.
     *
     * Disconnects all OrderConstraintIn arcs from this node.  All arcs are disconnected from both the source and destination and should be safe to delete
     *
     * @return A list of arcs that have been disconnected from this node and the connected node.  These arcs should be safe to delete
     */
    virtual std::set<std::shared_ptr<Arc>> disconnectOrderConstraintInArcs();

    /**
     * @brief Disconnects the node from the graph.
     *
     * Disconnects all output arcs from this node.  All arcs are disconnected from both the source and destination and should be safe to delete
     *
     * @note This also disconnects from the sources of OrderConstraint arcs.
     *
     * @return A list of arcs that have been disconnected from this node and the connected node.  These arcs should be safe to delete
     */
    std::set<std::shared_ptr<Arc>> disconnectOutputs();

    /**
     * @brief Disconnects the node from nodes it is (directly) dependent on by.
     *
     * Disconnects all output arcs from this node.  All arcs are disconnected from both the source and destination and should be safe to delete
     *
     * @return A list of arcs that have been disconnected from this node and the connected node.  These arcs should be safe to delete
     */
    virtual std::set<std::shared_ptr<Arc>> disconnectDirectOutputs();

    /**
     * @brief Disconnects the node from nodes it is virtually dependent on.
     *
     * Disconnects all OrderConstraintIn arcs from this node.  All arcs are disconnected from both the source and destination and should be safe to delete
     *
     * @return A list of arcs that have been disconnected from this node and the connected node.  These arcs should be safe to delete
     */
    virtual std::set<std::shared_ptr<Arc>> disconnectOrderConstraintOutArcs();

    /**
     * @brief Get the set of nodes connected to this node via arcs
     * @return set of nodes connected to this node via arcs
     */
    std::set<std::shared_ptr<Node>> getConnectedNodes();

    /**
     * @brief Get the set of nodes connected to the inputs of this node via arcs (direct and virtual)
     * @return set of nodes connected to this node via incoming arcs
     */
    virtual std::set<std::shared_ptr<Node>> getConnectedInputNodes();

    /**
     * @brief Get the set of nodes connected to the inputs of this node via arcs (direct and virtual)
     * @return set of nodes connected to this node via incoming arcs
     */
    virtual std::set<std::shared_ptr<Node>> getConnectedDirectInputNodes();

    /**
     * @brief Get the set of nodes connected to the inputs of this node via arcs (virtual)
     * @return set of nodes connected to this node via incoming arcs
     */
    virtual std::set<std::shared_ptr<Node>> getConnectedOrderConstraintInNodes();

    /**
     * @brief Get the set of nodes connected to the output of this node via arcs (virtual)
     * @return set of nodes connected to this node via incoming arcs
     */
    virtual std::set<std::shared_ptr<Node>> getConnectedOrderConstraintOutNodes();

    /**
     * @brief Get the set of nodes connected to the outputs of this node via arcs (direct)
     * @return set of nodes connected to this node via outgoing arcs
     */
    virtual std::set<std::shared_ptr<Node>> getConnectedDirectOutputNodes();

    /**
     * @brief Get the set of nodes connected to the outputs of this node via arcs (direct and virtual)
     * @return set of nodes connected to this node via outgoing arcs
     */
    virtual std::set<std::shared_ptr<Node>> getConnectedOutputNodes();

    /**
     * @brief Get the in-degree of this node (number of connected input arcs - direct and virtual)
     * @return in-degree of this node (number of connected input arcs)
     */
    virtual unsigned long inDegree();

    /**
     * @brief Get the in-degree of this node (number of connected input arcs - direct)
     * @return in-degree of this node (number of connected input arcs)
     */
    virtual unsigned long directInDegree();

    /**
     * @brief Get the in-degree of this node (number of connected input arcs - virtual)
     * @return in-degree of this node (number of connected input arcs)
     */
    virtual unsigned long orderConstraintInDegree();

    /**
     * @brief Get the out-degree of this node (number of connected output arcs - direct and virtual)
     * @return out-degree of this node (number of connected output arcs)
     */
    unsigned long outDegree();

    /**
     * @brief Get the out-degree of this node (number of connected output arcs - direct)
     * @return out-degree of this node (number of connected output arcs)
     */
    unsigned long directOutDegree();

    /**
     * @brief Get the out-degree of this node (number of connected input arcs - virtual)
     * @return out-degree of this node (number of connected output arcs)
     */
    virtual unsigned long orderConstraintOutDegree();

    /**
     * @brief Connects input and output ports with no arcs to the given node
     * @param connectToSrc the node to which unconnected input ports should be connected to
     * @param connectToSink the node to which unconnected output ports should be connected to
     * @param srcPortNum the port number of the connectToSrc to connect to
     * @param sinkPortNum the port number of the connectToSink to connect to
     * @return a vector of new arcs
     */
    virtual std::vector<std::shared_ptr<Arc>> connectUnconnectedPortsToNode(std::shared_ptr<Node> connectToSrc, std::shared_ptr<Node> connectToSink, int srcPortNum, int sinkPortNum);

    /**
     * @brief Get the out-degree of this node (number of connected output arcs - direct and vitual), ignoring arcs to nodes in the provided ignoreSet
     * @param ignoreSet a set of pointers to nodes.  Arcs to these nodes are not included in the out degree
     * @return out-degree of this node (number of connected output arcs), ignoring arcs to nodes in the provided ignoreSet
     */
    unsigned long outDegreeExclusingConnectionsTo(std::set<std::shared_ptr<Node>> ignoreSet);

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
     * @brief Get an aliased shared pointer to the specified OrderConstraint port of this node.  If no such port exists a null pointer is returned.
     *
     * The pointer is aliased with this node as the stored pointer.
     *
     * @return aliased shared pointer to input port or nullptr
     */
    std::shared_ptr<OrderConstraintInputPort> getOrderConstraintInputPort();

    /**
     * @brief Get an aliased shared pointer to the specified OrderConstraint port of this node.  If no such port exists a null pointer is returned.
     *
     * The pointer is aliased with this node as the stored pointer.
     *
     * @return aliased shared pointer to input port or nullptr
     */
    std::shared_ptr<OrderConstraintOutputPort> getOrderConstraintOutputPort();

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
     * @brief Get an aliased shared pointer to the specified OrderConstraint port of this node.  If no such port exists, create one
     *
     * The pointer is aliased with this node as the stored pointer.
     *
     * @return aliased shared pointer to input port
     */
    std::shared_ptr<OrderConstraintInputPort> getOrderConstraintInputPortCreateIfNot();

    /**
     * @brief Get an aliased shared pointer to the specified OrderConstraint port of this node.  If no such port exists, create one
     *
     * The pointer is aliased with this node as the stored pointer.
     *
     * @return aliased shared pointer to input port
     */
    std::shared_ptr<OrderConstraintOutputPort> getOrderConstraintOutputPortCreateIfNot();

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
     * @brief Get a vector of pointers to the current input ports of the node (including special inputs such as enables and selects)
     *
     * @warning Does not include the OrderConstraintPort
     *
     * @note Ports may be added to the node later which will not be reflected in the returned array
     *
     * @note The position in the vector does not nessisarily match the port number of the port.
     *
     * @return vector of aliased pointers to the current input ports of the node (including special inputs)
     */
    virtual std::vector<std::shared_ptr<InputPort>> getInputPortsIncludingSpecial();

    /**
     * @brief Get a vector of pointers to the current input ports of the node (including special inputs such as enables and selects).  Also includes the OrderConstraint port
     *
     * @note Ports may be added to the node later which will not be reflected in the returned array
     *
     * @note The position in the vector does not nessisarily match the port number of the port.
     *
     * @return vector of aliased pointers to the current input ports of the node (including special inputs)
     */
    std::vector<std::shared_ptr<InputPort>> getInputPortsIncludingSpecialAndOrderConstraint();

    /**
     * @brief Get a vector of pointers to the current output ports of the node
     *
     * @note Ports may be added to the node later which will not be reflected in the returned array
     *
     * @return vector of aliased pointers to the current output ports of the node
     */
    std::vector<std::shared_ptr<OutputPort>> getOutputPorts();

    /**
     * @brief Get a vector of pointers to the current output ports of the node (including the OrderConstraint port)
     *
     * @note Ports may be added to the node later which will not be reflected in the returned array
     *
     * @note The position in the vector does not nessisarily match the port number of the port.
     *
     * @return vector of aliased pointers to the current output ports of the node
     */
    std::vector<std::shared_ptr<OutputPort>> getOutputPortsIncludingOrderConstraint();

    /**
     * @brief Get a list of direct input arcs connected to this node (not order constraint arcs)
     * @return list of direct input arcs (not order constraint arcs)
     */
    std::set<std::shared_ptr<Arc>> getDirectInputArcs();

    /**
     * @brief Get a list of order constraint input arcs connected to this node
     * @return list of order constraint input arcs
     */
    std::set<std::shared_ptr<Arc>> getOrderConstraintInputArcs();

    /**
     * @brief Get a list of input arcs (direct and order constraint) connected to this node
     * @return list of direct input arcs
     */
    std::set<std::shared_ptr<Arc>> getInputArcs();

    /**
     * @brief Get a list of direct output arcs connected to this node (not order constraint arcs)
     * @return list of direct output arcs (not order constraint arcs)
     */
    std::set<std::shared_ptr<Arc>> getDirectOutputArcs();

    /**
     * @brief Get a list of order constraint output arcs connected to this node
     * @return list of order constraint output arcs
     */
    std::set<std::shared_ptr<Arc>> getOrderConstraintOutputArcs();

    /**
     * @brief Get a list of  output arcs (direct and order constraint) connected to this node
     * @return list of direct output arcs
     */
    std::set<std::shared_ptr<Arc>> getOutputArcs();

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
     *
     * @param sanitize If true, replaces newlines with spaces in returned string.  Otherwise, will not replace newlines in returned string.
     * @param delim the demimitor string (default '/')
     *
     * @return Fully qualified human readable name of the node as a std::string
     */
    std::string getFullyQualifiedName(bool sanitize = true, std::string delim = "/");

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
     * @param unconnected_master A pointer to the unconnected master node
     * @return pointer to expanded node if expansion occurred, nullptr if it did not
     */
    virtual std::shared_ptr<ExpandedNode> expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                     std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                     std::shared_ptr<MasterUnconnected> &unconnected_master);

    /**
     * @brief Identifies if the given input port experiences internal fanout in the node.
     *
     * Internal fanout occurs if an input is used more than once (either within the calculation of a single output or
     * or within the calculation of multiple outputs).
     *
     * @note Use the port's hasInternalFunction function as it will call the correct hasInternalFanout function in the node.
     * For example, select lines for muxes are inputs but are treated seperately and have their own internalFanout function.
     * This function is called by the hadInternalFanout function of the port for standard input ports.
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
     * @param schedType the scheduler used (parameter may not be used unless the C emit for the given node is different depending on the scheduler used - ex. if the scheduler is context aware)
     * @param outputPortNum the output port for which the C++ code should be generated for
     * @param imag if false, emits the real component of the output. if true, emits the imag component of the output
     * @param checkFanout if true, checks if the output is used in a fanout.  If false, no check is performed
     * @param forceFanout if true, a temporary variable is always create, if false the value of checkFanout dictates whether or not a temporary variable is created
     * @return a string containing the C code for calculating the result of the output port
     */
    virtual std::string
    emitC(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag = false,
              bool checkFanout = true, bool forceFanout = false);

protected:
    /**
     * @brief Emits a C expression to calculate the value of an output port
     *
     * @warning This function is separated from the @ref emitCExprNextState function in order to break emit cycles.  For any node with state,
     * the values of the new state elements should be computed in the @ref emitCExprNextState.
     *
     * @note @ref emitC should be called from outside the class instead of this function.  @ref emitC automatically handles fanout while this function does not
     *
     * @param cStatementQueue a reference to the queue containing C statements for the function being emitted.  Additional statements may be enqueued by this function
     * @param schedType the scheduler used (parameter may not be used unless the C emit for the given node is different depending on the scheduler used - ex. if the scheduler is context aware)
     * @param outputPortNum the output port for which the C++ code should be generated for
     * @param imag if false, emits the real component of the output. if true, emits the imag component of the output
     * @return a string containing the C expression for calculating the result of the output port
     */
    virtual CExpr emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                            bool imag = false);

public:

    /**
     * @brief Constructs a shallow copy of the given node.  Ports are not copied and neither is the parent reference.  The parent parameter is set and the cloned node is added as a child of the parent.
     *
     * @note If copying a graph, the parent should be one of the copies and not from the original graph.
     *
     * @param parent the parent node of the node
     * @return returns a shared pointer to the cloned node
     */
    virtual std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent);

    /**
     * @brief Constructs a shallow copy of the given node.  Ports are not copied and neither is the parent reference.  The parent parameter is set and the cloned node is added as a child of the parent.
     * Copies are made recursivly to children of this node
     *
     * @note If copying a graph, the parent should be one of the copies and not from the original graph.
     *
     * @param parent the parent node of the node
     * @param nodeCopies A vector into which pointers to the node copies are added
     * @param origToCopyNode A map of orig node pointers to copy pointers.  New node copies are entered into this map
     * @param copyToOrigNode A map of copy node pointers to orig pointers.  New node copies are entered into this map
     */
    virtual void shallowCloneWithChildren(std::shared_ptr<SubSystem> parent, std::vector<std::shared_ptr<Node>> &nodeCopies, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode);

    /**
     * @brief Clones input arcs (direct and virtual) for the given node
     *
     * The cloned arcs will connect the cloned versions of the nodes rather than redundantly connecting the origional nodes.
     *
     * @warning Nodes must already be cloned before this function is called.  It is assumed there is an entry in the origToCopyNode map.
     *
     * @note This function should be called on the origional node rather than a clone
     *
     * @param arcCopies A vector into which the cloned input arcs will be inserted into
     * @param origToCopyNode A map of origional node pointers to cloned node pointers
     * @param origToCopyArc A map of orig arc pointers to the cloned arc
     * @param copyToOrigArc A map of cloned arc pointers to orig arcs
     */
    virtual void cloneInputArcs(std::vector<std::shared_ptr<Arc>> &arcCopies, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode, std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &origToCopyArc, std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &copyToOrigArc);

    /**
     * @brief Identifies if the node contains state elements
     *
     * @return true if the node contains state elements, false if it does not contain state elements
     */
    virtual bool hasState();

    /**
     * @brief Identifies if the node contains a path that is combinational
     *
     * @note: Nodes can both contain state and have combinational paths (ex. Mealey style FSM)
     *
     * @return true if the node contains a combinational path, false if it does not contain a combinational path (likely a pure state element)
     */
    virtual bool hasCombinationalPath();

    /**
     * @brief If this node has state, get the corresponding StateUpdate node
     * @return A pointer to the StateUpdate node if this node has state, nullptr if this node does not have state or is a latch type (ex. EnableOutput)
     */
    std::shared_ptr<StateUpdate> getStateUpdateNode();

    /**
     * @brief Set the state update node (useful for cloneing)
     * @param stateUpdate StateUpdate node to set
     */
     void setStateUpdateNode(std::shared_ptr<StateUpdate> stateUpdate);

    /**
     * @brief If this node has state, create its corresponding StateUpdate node and insert it into the graph.  Also sets
     * the stateUpdate node entry in the node so that getStateUpdateNode will return the corresponding node.
     *
     * @note This function should typically be called after context discovery and context expansion.  This is because
     * the StateUpdate generally inherits
     *
     * @param new_nodes a vector of nodes added to the design.  This includes the new StateUpdate node
     * @param deleted_nodes a vector of nodes to be deleted in the design
     * @param new_arcs a vector of new arcs added to the design
     * @param deleted_arcs a vector of arcs to be deleted from the design
     *
     * @return true if the StateUpdate node was created and inserted into the graph, false if it was not
     */
    virtual bool createStateUpdateNode(std::vector<std::shared_ptr<Node>> &new_nodes,
                                       std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                       std::vector<std::shared_ptr<Arc>> &new_arcs,
                                       std::vector<std::shared_ptr<Arc>> &deleted_arcs);

    /**
     * @brief Identifies if the node requires a global declaration.
     *
     * Examples of requiring global declarations include declaring constant values or constant arrays
     *
     * This function is called by the emitter to determine if global declarations are required
     *
     * @return true if the node requires an global declaration, false if it does not
     */
    virtual bool hasGlobalDecl();

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
     * @brief Get the global declaration(s) required by the node
     *
     * Use @ref hasGlobalDecl to determine if a node requires global declaration(s)
     *
     * @return The global declaration(s) required by the node as a string
     */
    virtual std::string getGlobalDecl();

    /**
     * @brief Emits a C expressions to calculate the next value of the internal state elements.  These values will be used in @ref emitCStateUpdate
     *
     * @warning This function is separated from the @ref emitCExpr function in order to break emit cycles
     * @param cStatementQueue a reference to the queue containing C statements for the function being emitted.  Additional statements may be enqueued by this function
     * @param schedType the scheduler used (parameter may not be used unless the C emit for the given node is different depending on the scheduler used - ex. if the scheduler is context aware)
     */
    virtual void emitCExprNextState(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType);

    /**
     * @brief Emits the C code to update the state varibles of this node.
     *
     * This code should be called at the end of the emit for the given function.
     *
     * @param cStatementQueue a reference to the queue containing C statements for the function being emitted.  The state update statements for this node are enqueued onto this queue
     * @param schedType the scheduler used (parameter may not be used unless the C emit for the given node is different depending on the scheduler used - ex. if the scheduler is context aware)
     */
    virtual void emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType);

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
     * @param parent the new parent of this node
     */
    void setParent(std::shared_ptr<SubSystem> parent);

    int getId() const;

    void setId(int id);

    const std::string &getName() const;

    void setName(const std::string &name);

    int getPartitionNum() const;

    void setPartitionNum(int partitionNum);

    int getSchedOrder() const;

    void setSchedOrder(int schedOrder);

    /**
     * @brief Returns true if a->schedOrder < b->schedOrder
     * @param a
     * @param b
     * @return true if a->schedOrder < b->schedOrder
     */
    static bool lessThanSchedOrder(std::shared_ptr<Node> a, std::shared_ptr<Node> b);

    /**
     * @brief Copy port names from another node.  If the ports in copyFrom do not exist in this node, they are created
     * @param copyFrom The node to copy port names from
     */
    void copyPortNames(std::shared_ptr<Node> copyFrom);

    /**
     * @brief Returns a string that can be included at the end of error messages to provide context for the error
     * @return
     */
    std::string getErrorReportContextStr();

};

/*! @} */

#endif //VITIS_NODE_H
