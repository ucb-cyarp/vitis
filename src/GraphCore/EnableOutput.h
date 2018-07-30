//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_ENABLEOUTPUT_H
#define VITIS_ENABLEOUTPUT_H

#include <memory>

#include "EnableNode.h"
#include "Node.h"
#include "SubSystem.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Represents a special output port at the interface of an enabled subsystem.
 *
 * If not enabled, the previous value of this node is fed to the downstream logic.
 * If enabled, the calculated value is fed to the downstream logic as usual.
 */
class EnableOutput : public EnableNode{
friend class NodeFactory;

protected:
    /**
     * @brief Default constructor
     */
    EnableOutput();

    /**
     * @brief Construct a node with a given parent
     * @param parent parent of new node
     */
    EnableOutput(std::shared_ptr<SubSystem> parent);

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;


    /**
     * @brief Validate if the connections to this port are correct.
     *
     *   - Verify that the input port is a boolean type (and width 1).  Also Verify 1 and only 1 arc is connected
     *   - Checks that the port is in the EnabledSubsystem EnabledOutput list
     *
     * If an invalid configuration is detected, the function will throw an exception
     */
    void validate() override;
};

/*@}*/

#endif //VITIS_ENABLEOUTPUT_H
