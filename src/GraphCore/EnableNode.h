//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_ENABLENODE_H
#define VITIS_ENABLENODE_H

#include <memory>
#include "Node.h"
#include "SubSystem.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Represents a special port at the interface of an enabled subsystem
 *
 * @note This class is an abstract class with concrete classes being @ref EnableInput and @ref EnableOutput
 */
class EnableNode : public Node {
friend class NodeFactory;

protected:
    Port enablePort; ///< The enable port.  The input of this port determines if a new value is propagated or not

    void init() override;

public:
    /**
     * @brief Default constructor
     */
    EnableNode();

    /**
     * @brief Construct a node with a given parent
     * @param parent parent of the new node
     */
    EnableNode(std::shared_ptr<SubSystem> parent);
};

/*@}*/

#endif //VITIS_ENABLENODE_H
