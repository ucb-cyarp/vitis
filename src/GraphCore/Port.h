//
// Created by Christopher Yarp on 6/25/18.
//

#ifndef VITIS_PORT_H
#define VITIS_PORT_H

#include <memory>
#include <set>
//#include "Node.h"
//#include "Arc.h"

//Forward Decls (Breaking Circular Dep)
class Arc;
class Node;

//This Class
/**
* \addtogroup GraphCore Graph Core
*/
/*@{*/

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
    enum class PortType
    {
        INPUT, ///< An input port
        OUTPUT, ///< An output port
        ENABLE ///< An enable port
    };

private:
    Node* parent; ///< The node this port belongs to (not a shared ptr because the port is a component of the node - there would always be a shared ptr to a node and would therefore never be deleted)
    int portNum; ///< The number of this port
    Port::PortType type; ///< The type of port (Input, Output, Enable)
    std::set<std::shared_ptr<Arc>> arcs; ///< A vector containg pointers to arcs connected to this port

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
    Port(Node* parent, Port::PortType type, int portNum);

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

    /**
     * @brief Get the current set of arcs connected to this port
     * @return A copy of the set of arcs connected to this port
     */
    std::set<std::shared_ptr<Arc>> getArcs();

    /**
     * @brief Replaces the set of arc assigned to this port
     * @param arcs The new set of arcs for this port
     */
    void setArcs(std::set<std::shared_ptr<Arc>> arcs);

    /**
     * @brief Get an iterator to the first arc
     * @return iterator to the first arc in the arc set
     */
    std::set<std::shared_ptr<Arc>>::iterator arcsBegin();

    /**
     * @brief Get an iterator to the last arc
     * @return iterator to the first arc in the arc set
     */
    std::set<std::shared_ptr<Arc>>::iterator arcsEnd();

    /**
     * @brief Get a shared pointer to the port.  Aliased with the parent node object as the stored pointer.
     *
     * This allows the parent node to be deleted when all shared pointers to ports and the node are deleted.
     * Stops the node from being deleted when all pointers to the node are deleted but pointers from arcs exist.
     * Stops the node from never being deleted if shared pointers to the node are contained in each port.
     *
     * @return an aliased shared pointer to the port
     */
    std::shared_ptr<Port> getSharedPointer(); //NOTE: should never return a bare pointer or an unaliased shared pointer to a port

    //==== Getters/Setters ====

    //Do not provide setter to parent as the port is contained within the parent.  Needs to be initialized at the constructor.
    //Provide a getter for a shared pointer to the parent.  The parent should not change during the lifetime of the port.
    //NEVER return a bare pointer
    std::shared_ptr<Node> getParent();

    int getPortNum() const;
    void setPortNum(int portNum);
    PortType getType() const;
    void setType(PortType type);

};

/*@}*/

#endif //VITIS_PORT_H
