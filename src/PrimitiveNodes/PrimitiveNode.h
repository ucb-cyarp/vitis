//
// Created by Christopher Yarp on 7/17/18.
//

#ifndef VITIS_PRIMITIVENODE_H
#define VITIS_PRIMITIVENODE_H

#include "GraphCore/Node.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 *
 * @brief A set of primitive nodes in a Data Flow Graph
 */
/*@{*/

/**
 * @brief Represents a primitive node in the Date Flow Graph
 *
 * This is an abstract class.  It is not a friend of the NodeFactory class and the constructors are kept protected
 * as a result
 */
class PrimitiveNode : public Node{
protected:
    /**
     * @brief Constructs a default primitive node
     */
    PrimitiveNode();

    /**
     * @brief Constructs a default primitive node with a given parent.
     *
     * @param parent parent node
     */
    explicit PrimitiveNode(std::shared_ptr<SubSystem> parent);

public:
    //Primitive nodes cannot be expanded
    bool canExpand() override;
};

/*@}*/

#endif //VITIS_PRIMITIVENODE_H
