//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_ENABLEINPUT_H
#define VITIS_ENABLEINPUT_H

#include "EnableNode.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Represents a special input port at the interface of an enabled subsystem.
 *
 * If not enabled, the downstream logic from this node is not executed in the given clock cycle and the previous state
 * is held.  If enabled, the downstream logic from this node is allowed to execute as usual and state is updated.
 */
class EnableInput : public EnableNode {
friend class NodeFactory;

protected:
    /**
     * @brief Default constructor
     */
    EnableInput();

    /**
     * @brief Construct a node with a given parent
     * @param parent parent of new node
     */
    EnableInput(std::shared_ptr<SubSystem> parent);

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    /**
     * @brief Validate if the connections to this port are correct.
     *
     *   - Verify that the input port is a boolean type (and width 1).  Also Verify 1 and only 1 arc is connected
     *   - Checks that the port is in the EnabledSubsystem EnabledInput list
     *
     * If an invalid configuration is detected, the function will throw an exception
     */
    void validate() override;
};

/*@}*/

#endif //VITIS_ENABLEINPUT_H
