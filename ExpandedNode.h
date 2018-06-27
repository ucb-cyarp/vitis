//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_EXPANDEDNODE_H
#define VITIS_EXPANDEDNODE_H

#include <memory>
#include "SubSystem.h"
#include "Node.h"

/**
 * @brief Represents a node that has been expanded.
 *
 * An expansion may be conducted by an external tool or by a pass over the data flow graph.  Expanded nodes become
 * Sub-systems which encapsulate the logic which implements them.  To facilitate the re-expansion or modification of
 * the expanded node, a reference to the original node (containing the original parameters used for expansion) is kept.
 *
 */
class ExpandedNode : SubSystem{
protected:
    std::shared_ptr<Node> origNode; ///< A pointer to the original node before expansion
};


#endif //VITIS_EXPANDEDNODE_H
