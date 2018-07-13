//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_ENABLEDEXPANDEDNODE_H
#define VITIS_ENABLEDEXPANDEDNODE_H

#include "EnabledSubSystem.h"
#include "ExpandedNode.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Represents a node which, when expanded, became an enabled subsystem.
 *
 * Extends both @ref EnabledSubSystem and @ref ExpandedNode, both of which extend @ref SubSystem
 */
class EnabledExpandedNode : public EnabledSubSystem, public ExpandedNode {
friend class NodeFactory;

protected:
    /**
     * @brief Default constructor
     */
    EnabledExpandedNode();

    /**
     * @brief Construct a new node with a given parent
     * @param parent the parent for the new node
     */
    EnabledExpandedNode(std::shared_ptr<SubSystem> parent);

    /**
     * @brief Construct a node with a given parent and given orig node.
     * @param parent of new node
     * @param orig original node
     */
    EnabledExpandedNode(std::shared_ptr<SubSystem> parent, std::shared_ptr<Node> orig);

    //Init should be overwritten by EnabledSubSystem

    std::string labelStr() override ;

};

/*@}*/

#endif //VITIS_ENABLEDEXPANDEDNODE_H
