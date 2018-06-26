//
// Created by Christopher Yarp on 6/25/18.
//

#ifndef VITIS_NODE_H
#define VITIS_NODE_H

#include <vector>
#include <set>
#include <string>
#include <memory>
#include "Port.h"
#include "Arc.h"
#include "SubSystem.h"
#include "GraphMLParameter.h"

/**
 * @brief Generic Node in Data Flow Graph
 *
 * This class represents a generic node in the data flow graph.  It is an abstract class which provides some basic
 * utility functions as well as stub functions which should be overwritten by subclasses.
 */
class Node {
protected:

    int id; ///<Node ID number used when reading/writing GraphML files
    std::shared_ptr<SubSystem> parent; ///<Parent of this node
    std::vector<Port> inputPorts; ///<The input ports of this node
    std::vector<Port> outputPorts; ///<The output ports of this node
    int partitionNum; ///<The partition set this node is contained within.  Used for multicore output
    std::vector<bool> cOutEmitted; ///<A vector of bools indicating if a particular output has been emitted in C++
    std::vector<std::string> cOutVarName; ///<A vector of strings which hold the variable names in C++ if the output was stored in a C++ var

public:
    //==== Constructors ====
    /**
     * @brief Constructs an empty node
     */
    Node();

    /**
     * @brief Constructs an empty node with a given parent.  Adds the new node to the children list of the parent.
     * @param parent parent node
     */
    explicit Node(std::shared_ptr<SubSystem> parent);

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
     * @brief Removes the given input arc from the node.
     *
     * Will search all input ports to find the given arc
     * @param arc Arc to remove
     */
    void removeInArc(std::shared_ptr<Arc> arc);

    /**
     * @brief Removes the given output arc from the node.
     *
     * Will search all output ports to find the given arc
     * @param arc Arc to remove
     */
    void removeOutArc(std::shared_ptr<Arc> arc);

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
    virtual std::string emitGraphML();

    /**
     * @brief Identifies which GraphML parameters this node (and its descendants) can emit
     * @return set of GraphML parameters which this node (and its descendants) can emit.
     *
     * @note The set may contain multiple parameters with the same key but different types.
     * Type conflicts must be resolved before emitting GraphML.
     */
    virtual std::set<GraphMLParameter> graphMLParameters();

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
     * @param nodes The nodes present in the design.  This will be updated if nodes are added or deleted.
     * @param arcs The arcs present in the design.  This will be updated if arcs are added or deleted.
     * @return true if expansion occurred, false if it did not
     */
    virtual bool expand(std::vector<std::shared_ptr<Node>> nodes, std::vector<std::shared_ptr<Arc>> arcs);

    /**
     * @brief Emit C++ code to calculate the value of an output port.
     *
     * If the output is dependant on the result of other operations, those dependant operations will be emitted
     * and included in the returned string before the code for the given operation.
     *
     * @param outputPort the output port for which the C++ code should be generated for
     * @return a string containing the C++ code for calculating the result of the output port
     */
    virtual std::string emitCpp(int outputPort);

    //==== Getters/Setters ====
    //++++ Getters/Setters With Added Functionality ++++
    /**
     * Sets the parent of the node.  Before setting, removes from the children list of the old parent (if not NULL).
     * Adds to the children list of the new parent (if not NULL).
     * @param parent new parent
     */
    void updateParent(SubSystem *parent);


};


#endif //VITIS_NODE_H
