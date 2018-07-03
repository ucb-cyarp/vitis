//
// Created by Christopher Yarp on 7/2/18.
//

#ifndef VITIS_NODEFACTORY_H
#define VITIS_NODEFACTORY_H

#include <memory>

#include "Node.h"
#include "SubSystem.h"

/**
* \addtogroup GraphCore Graph Core
*/
/*@{*/

template <typename T>
/**
 * @brief Factory class for creating nodes.
 *
 * The main reason for this class is to ensure that shared pointers exist before parts of the node initialization which
 * rely on passing pointers to "this" execute.  This is an artifiact of "enable_shared_from_this" works as described in
 * https://stackoverflow.com/questions/31924396/why-shared-from-this-cant-be-used-in-constructor-from-technical-standpoint.
 *
 * It is also included to provide templated factories for the many subclasses of node without requiring code duplication.
 *
 * Not included in the constructor of nodes since any shared pointer would be out of sync with the shared pointer in the parent.
 * Not including as static function of Node class because of circular dependency with SubSystem class.
 */
class NodeFactory {
public:
    /**
     * @brief Create node with parent set to given parent.  Add new node to the parent's children list (if parent is not null).
     * @param parent Parent of the new node
     * @return pointer to the new node
     */
    static std::shared_ptr<T> createNode(std::shared_ptr<SubSystem> parent)
    {
        std::shared_ptr<T> node = std::shared_ptr<T>(new T(parent));

        std::shared_ptr<Node> nodeCast = node;

        nodeCast->init(); //There is now a shared_ptr to the class, can now init things that require pointers to "this" inside the node constructor.

        if(parent != nullptr)
        {
            parent->addChild(node);
        }
    }

    /**
     * @brief Create (expanded) node with parent set to given parent and orig node set to given orig.  Add new node to the parent's children list (if parent is not null).
     * @param parent Parent of the new node
     * @param orig Origional node of new
     * @return pointer to the new node
     */
    static std::shared_ptr<T> createNode(std::shared_ptr<SubSystem> parent, std::shared_ptr<Node> orig)
    {
        std::shared_ptr<T> node = std::shared_ptr<T>(new T(parent));

        std::shared_ptr<Node> nodeCast = node;

        nodeCast->init(); //There is now a shared_ptr to the class, can now init things that require pointers to "this" inside the node constructor.

        if(parent != nullptr)
        {
            parent->addChild(node);
        }
    }

    /**
    * @brief Create node with no parent
    * @return pointer to the new node
    */
    static std::shared_ptr<T> createNode()
    {
        std::shared_ptr<T> node = std::shared_ptr<T>(new T());

        std::shared_ptr<Node> nodeCast = node;

        nodeCast->init(); //There is now a shared_ptr to the class, can now init things that require pointers to "this" inside the node constructor.
    }

};

/*@}*/

#endif //VITIS_NODEFACTORY_H