//
// Created by Christopher Yarp on 6/25/18.
//

#ifndef VITIS_SUBSYSTEM_H
#define VITIS_SUBSYSTEM_H

#include <vector>
#include <memory>
#include "Node.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Represents a Sub-system within the flow graph.
 *
 * While sub-systems are Nodes, they also can contain children.  Typically arcs will not terminate at a subsystem unless
 * there is a specific emit function for that subsystem.
 */
class SubSystem : public Node{
protected:
    std::vector<std::shared_ptr<Node>> children; ///< Nodes contained within this sub-system

public:
    /**
     * @brief Adds a child of this node to the children list
     * @param child The child to add
     */
    void addChild(std::shared_ptr<Node> child);

    /**
     * @brief Removes a child from this node's list of children
     * @param child The child to remove
     */
    void removeChild(std::shared_ptr<Node> child);

};

/*@}*/

#endif //VITIS_SUBSYSTEM_H
