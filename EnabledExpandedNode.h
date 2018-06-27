//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_ENABLEDEXPANDEDNODE_H
#define VITIS_ENABLEDEXPANDEDNODE_H

#include "EnabledSubSystem.h"
#include "ExpandedNode.h"

/**
 * @brief Represents a node which, when expanded, became an enabled subsystem.
 *
 * Extends both @ref EnabledSubSystem and @ref ExpandedNode, both of which extend @ref SubSystem
 */
class EnabledExpandedNode : EnabledSubSystem, ExpandedNode {

};


#endif //VITIS_ENABLEDEXPANDEDNODE_H
