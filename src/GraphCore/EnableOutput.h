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
};

/*@}*/

#endif //VITIS_ENABLEOUTPUT_H
