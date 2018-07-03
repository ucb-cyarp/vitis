//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_EXPANDEDNODE_H
#define VITIS_EXPANDEDNODE_H

#include <memory>
#include "SubSystem.h"
#include "Node.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Represents a node that has been expanded.
 *
 * An expansion may be conducted by an external tool or by a pass over the data flow graph.  Expanded nodes become
 * Sub-systems which encapsulate the logic which implements them.  To facilitate the re-expansion or modification of
 * the expanded node, a reference to the original node (containing the original parameters used for expansion) is kept.
 *
 */
class ExpandedNode : public SubSystem{
friend class NodeFactory;

protected:
    std::shared_ptr<Node> origNode; ///< A pointer to the original node before expansion

    /**
     * @brief Default Constructor.  Sets orig node to null.
     */
    ExpandedNode();

    /**
     * @brief Construct a node with a given parent.  Orig node is set to null
     * @param parent parent of new node
     */
    ExpandedNode(std::shared_ptr<SubSystem> parent);

    /**
     * @brief Construct a node with a given parent and given orig node.
     * @param parent of new node
     * @param orig original node
     */
    ExpandedNode(std::shared_ptr<SubSystem> parent, std::shared_ptr<Node> orig);

public:
    //==== Getters/Setters ====
    const std::shared_ptr<Node> getOrigNode() const;
    void setOrigNode(std::shared_ptr<Node> origNode);

};

/*@}*/

#endif //VITIS_EXPANDEDNODE_H
