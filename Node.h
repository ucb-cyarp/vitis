//
// Created by Christopher Yarp on 6/25/18.
//

#ifndef VITIS_NODE_H
#define VITIS_NODE_H

#include <vector>
#include <string>
#include <memory>
#include "Port.h"
#include "Arc.h"
#include "SubSystem.h"

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
     * Constructs an empty node
     */
    Node();

    /**
     * Constructs an empty node with a given parent.  Adds the new node to the children list of the parent.
     * @param parent parent node
     */
    explicit Node(std::shared_ptr<SubSystem> parent);

    //==== Functions ====
    /**
     * Adds an input arc to this node.  Removes the arc from the orig port if not NULL.
     * Updates the given arc so that the destination port is set to the specified input
     * port of this node.
     * @param portNum Input port number to add the arc to
     * @param arc The arc to add
     */
    void addInArcUpdatePrevUpdateArc(int portNum, std::shared_ptr<Arc> arc);

    /**
     * Adds an output arc to this node.  Removes the arc from the orig port if not NULL.
     * Updates the given arc so that the source port is set to the specified output
     * port of this node.
     * @param portNum Output port number to add arc to
     * @param arc The arc to add
     */
    void addOutArcUpdatePrevUpdateArc(int portNum, std::shared_ptr<Arc> arc);

    /**
     * Removes the given input arc from the node.  Will search all input ports to find the given arc
     * @param arc Arc to remove
     */
    void removeInArc(std::shared_ptr<Arc> arc);

    /**
     * Removes the given output arc from the node.  Will search all output ports to find the given arc
     * @param arc Arc to remove
     */
    void removeOutArc(std::shared_ptr<Arc> arc);

    /**
     * Get the full hierarchical path of this node in GraphML format (ex. "n1::n2::n3")
     * @return Full hierarchical path of the given node as a std::string
     */
    std::string getFullGraphMLPath();

    /**
     * Emit GraphML for the given node (and its descendants if it is a Subsystem)
     * @return GraphML description of the given node (and its descendants) as a std::string
     */
    virtual std::string emitGraphML();

    virtual


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
