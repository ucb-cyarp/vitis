//
// Created by Christopher Yarp on 8/22/18.
//

#ifndef VITIS_HIGHLEVELNODE_H
#define VITIS_HIGHLEVELNODE_H

#include "GraphCore/Node.h"

/**
 * \addtogroup HighLevelNodes High Level Nodes
 *
 * @brief Expandable to primitives and may have multiple implementation possibilities
 *
 * A Convenience For Referring to a Common Structure
 */
/*@{*/

/**
 * @brief Represents a High Level Node which is expandable to primitives and may have multiple implementation possibilities
 *
 */
class HighLevelNode : public Node {
protected:
    /**
     * @brief Constructs a default high level node
     */
    HighLevelNode();

    /**
     * @brief Constructs a default high level node with a given parent.
     *
     * @param parent parent node
     */
    explicit HighLevelNode(std::shared_ptr<SubSystem> parent);

public:
    //High level nodes can be expanded
    bool canExpand() override;
};

/*@}*/

#endif //VITIS_HIGHLEVELNODE_H
