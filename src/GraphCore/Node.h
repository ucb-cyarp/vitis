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
#include "Port.h"
#include "Arc.h"
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
 */
class Node : public std::enable_shared_from_this<Node> {
    friend class NodeFactory;

protected:
    int id; ///<Node ID number used when reading/writing GraphML files
    std::shared_ptr<SubSystem> parent; ///<Parent of this node
    std::vector<Port> inputPorts; ///<The input ports of this node
    std::vector<Port> outputPorts; ///<The output ports of this node
    int partitionNum; ///<The partition set this node is contained within.  Used for multicore output
    std::vector<bool> cOutEmitted; ///<A vector of bools indicating if a particular output has been emitted in C++
    std::vector<std::string> cOutVarName; ///<A vector of strings which hold the variable names in C++ if the output was stored in a C++ var

    //==== Constructors (Protected to force use of factory - required to handle  ====

    /**
     * @brief Constructs an empty node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    Node();

    /**
     * @brief Constructs an empty node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
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
    std::shared_ptr<Port> getInputPort(int portNum);

    /**
     * @brief Get an aliased shared pointer to the specified output port of this node.  If no such port exists a null pointer is returned.
     *
     * The pointer is aliased with this node as the stored pointer.
     *
     * @param portNum the output port number
     * @return aliased shared pointer to output port or nullptr
     */
    std::shared_ptr<Port> getOutputPort(int portNum);

    /**
     * @brief Get a vector of pointers to the current input ports of the node
     *
     * @note Ports may be added to the node later which will not be reflected in the returned array
     *
     * @return vector of aliased pointers to the current input ports of the node
     */
    std::vector<std::shared_ptr<Port>> getInputPorts();

    /**
     * @brief Get a vector of pointers to the current output ports of the node
     *
     * @note Ports may be added to the node later which will not be reflected in the returned array
     *
     * @return vector of aliased pointers to the current output ports of the node
     */
    std::vector<std::shared_ptr<Port>> getOutputPorts();

    /**
     * @brief Get the full hierarchical path of this node in GraphML format
     *
     * A typical GraphML formatted hierarchy would be "n1::n2::n3"
     * @return Full hierarchical path of the given node as a std::string
     */
    std::string getFullGraphMLPath();

    /**
     * @brief Emit GraphML for the given node (and its descendants if it is a Subsystem)
     * @return GraphML description of the given node (and its descendants) as a std::string
     */
//    virtual std::string emitGraphML();

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
//    virtual std::set<GraphMLParameter> graphMLParameters() = 0;

    /**
     * @brief Expand this node if applicable
     *
     * Typical reasons for expansion include:
     * - High Level Blocks to Primitive Operations
     *     - FIR Filter
     *     - Tapped Delay Line
     *     - FFT
     * - Vector Operation to Primitive Operations
     *
     * @param new_nodes A vector which will be filled with the new nodes created during expansion
     * @param deleted_nodes A vector which will be filled with the nodes deleted during expansion
     * @param new_arcs A vector which will be filled with the new arcs created during expansion
     * @param deleted_arcs A vector which will be filled with the arcs deleted during expansion
     * @return true if expansion occurred, false if it did not
     */
//    virtual bool expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes, std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs) = 0;

    /**
     * @brief Emit C++ code to calculate the value of an output port.
     *
     * If the output is dependant on the result of other operations, those dependant operations will be emitted
     * and included in the returned string before the code for the given operation.
     *
     * @param outputPort the output port for which the C++ code should be generated for
     * @return a string containing the C++ code for calculating the result of the output port
     */
//    virtual std::string emitCpp(int outputPort) = 0;

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
