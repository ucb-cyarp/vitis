//
// Created by Christopher Yarp on 7/17/18.
//

#ifndef VITIS_MEDIUMLEVELNODE_H
#define VITIS_MEDIUMLEVELNODE_H

#include "GraphCore/Node.h"

/**
 * \addtogroup MediumLevelNodes Medium Level Nodes
 *
 * @brief Expandable to Primitives But Typically has a Single Implementation Type
 *
 * A Convenience For Referring to a Common Structure
 */
/*@{*/

/**
 * @brief Represents a Medium Level Node which is Expandable to Primitives But Typically has a Single Implementation Type
 *
 * A Convenience For Referring to a Common Structure
 */
class MediumLevelNode : public Node {
protected:
    /**
     * @brief Constructs a default medium level node
     */
    MediumLevelNode();

    /**
     * @brief Constructs a default medium level node with a given parent.
     *
     * @param parent parent node
     */
    explicit MediumLevelNode(std::shared_ptr<SubSystem> parent);

public:
    //Medium level nodes can be expanded
    bool canExpand() override;
};

/*@}*/

#endif //VITIS_MEDIUMLEVELNODE_H
