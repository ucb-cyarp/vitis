//
// Created by Christopher Yarp on 7/2/18.
//

#ifndef VITIS_NODEFACTORY_H
#define VITIS_NODEFACTORY_H

#include <memory>
#include <MasterNodes/MasterOutput.h>
#include <MasterNodes/MasterInput.h>
#include <MultiRate/RateChange.h>
#include <MultiRate/Upsample.h>
#include <MultiRate/Repeat.h>
#include <MultiRate/Downsample.h>
#include <MultiRate/ClockDomain.h>
#include <MasterNodes/MasterNode.h>

#include "Node.h"
#include "SubSystem.h"
#include "EnableNode.h"
#include "GraphMLTools/GraphMLHelper.h"
#include "EnabledSubSystem.h"
#include "EnabledExpandedNode.h"
#include "EnableInput.h"
#include "EnableOutput.h"

/**
* \addtogroup GraphCore Graph Core
* @{
*/

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
    template <typename T>
    static std::shared_ptr<T> createNode(std::shared_ptr<SubSystem> parent)
    {
        std::shared_ptr<T> node = std::shared_ptr<T>(new T(parent));

        node->init(); //There is now a shared_ptr to the class, can now init things that require pointers to "this" inside the node constructor.

        if(parent != nullptr)
        {
            parent->addChild(node);
            std::vector<std::string> origLoc = parent->getOrigLocation();
            origLoc.push_back(parent->getName());
            node->setOrigLocation(origLoc);
        }

        return node;
    }

    /**
     * @brief Create a shallow clone of a given node.  The new node will have a parent set to given parent.  Add new node to the parent's children list (if parent is not null).
     *
     * @note If cloning the graph, the parent should be the clone of the parent
     * @param parent Parent of the new node
     * @param cloneFrom the node from which the shallow clone is being made
     * @return pointer to the new node
     */
    template <typename T>
    static std::shared_ptr<T> shallowCloneNode(std::shared_ptr<SubSystem> parent, T* cloneFrom)
    {
        std::shared_ptr<T> node = std::shared_ptr<T>(new T(parent, cloneFrom));

        node->init(); //There is now a shared_ptr to the class, can now init things that require pointers to "this" inside the node constructor.

        if(parent != nullptr)
        {
            parent->addChild(node);
        }

        return node;
    }

    /**
     * @brief Create (expanded) node with parent set to given parent and orig node set to given orig.  Add new node to the parent's children list (if parent is not null).
     * @param parent Parent of the new node
     * @param orig Origional node of new
     * @return pointer to the new node
     */
    template <typename T>
    static std::shared_ptr<T> createNode(std::shared_ptr<SubSystem> parent, std::shared_ptr<Node> orig)
    {
        std::shared_ptr<T> node = std::shared_ptr<T>(new T(parent));

        node->init(); //There is now a shared_ptr to the class, can now init things that require pointers to "this" inside the node constructor.
        node->setOrigNode(orig);
        node->setName(orig->getName() + "_expanded");
        node->setOrigLocation(orig->getOrigLocation());

        if(parent != nullptr)
        {
            parent->addChild(node);
        }

        return node;
    }

    /**
     * @brief Create (enabled expanded) node with parent set to given parent and orig node set to given orig.  Add new node to the parent's children list (if parent is not null).
     * @param parent Parent of the new node
     * @param orig Origional node of new
     * @return pointer to the new node
     */
    template <typename T>
    static std::shared_ptr<T> createNodeEnableSubsystem(std::shared_ptr<SubSystem> parent, std::shared_ptr<EnabledSubSystem> orig)
    {
        std::shared_ptr<T> node = std::shared_ptr<T>(new T(parent));

        node->init(); //There is now a shared_ptr to the class, can now init things that require pointers to "this" inside the node constructor.
        node->setOrigNode(orig);

        if(parent != nullptr)
        {
            parent->addChild(node);
        }

        return node;
    }

    /**
    * @brief Create node with no parent
    * @return pointer to the new node
    */
    template <typename T>
    static std::shared_ptr<T> createNode()
    {
        std::shared_ptr<T> node = std::shared_ptr<T>(new T());

        node->init(); //There is now a shared_ptr to the class, can now init things that require pointers to "this" inside the node constructor.

        return node;
    }

};

/*! @} */

#endif //VITIS_NODEFACTORY_H
