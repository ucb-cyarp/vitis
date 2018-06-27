//
// Created by Christopher Yarp on 6/25/18.
//

#ifndef VITIS_PORT_H
#define VITIS_PORT_H

#include <memory>
#include <vector>
//#include "Node.h"
//#include "Arc.h"

//Forward Decls (Breaking Circular Dep)
class Arc;
class Node;

//This Class

/**
 * @brief The representation of a node's port
 *
 * Primarily serves as a container for pointers to arcs connected to the given port.
 */
class Port {
public:

    /**
     * @brief An enum representing the different Port types
     */
    enum PortType
    {
        INPUT, ///< An input port
        OUTPUT, ///< An output port
        ENABLE ///< An enable port
    };

private:
    std::shared_ptr<Node> parent; ///< The node this port belongs to
    int portNum; ///< The number of this port
    Port::PortType type; ///< The type of port (Input, Output, Enable)
    std::vector<std::shared_ptr<Arc>> arcs; ///< A vector containg pointers to arcs connected to this port

public:
//==== Constructors ====
    /**
     * @brief Construct an empty port object
     *
     * The port object has number 0, type INPUT, no parent, and no arcs.
     */
    Port();

    /**
     * @brief Construct a port object with the specified parent, type, and port numbers.
     *
     * The port object is initialized with an empty arc list
     *
     * @param parent The node this port belongs to
     * @param type The type of this port (Input, Output, Enable)
     * @param portNum The port number
     */
    Port(std::shared_ptr<Node> parent, Port::PortType type, int portNum);

//==== Methods ====
    /**
     * @brief Add an arc to this port
     *
     * Specifically adds this arc to the arcs vector.
     *
     * @note This function does not update any previous node/port object if this is the reassignment of the arc.
     * It also does not update entries in the Arc object.
     *
     * @param arc Arc to add
     */
    void addArc(std::shared_ptr<Arc> arc);

    /**
     * @brief Removes the given arc from this port.
     *
     * If no such arc exists in this port, no action is taken
     *
     * @note This function does not update any previous node/port object if this is the reassignment of the arc.
     * It also does not update entries in the Arc object.
     *
     * @param arc The arc to remove
     */
    void removeArc(std::shared_ptr<Arc> arc);

};


#endif //VITIS_PORT_H
