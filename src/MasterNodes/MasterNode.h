//
// Created by Christopher Yarp on 7/11/18.
//

#ifndef VITIS_MASTERNODE_H
#define VITIS_MASTERNODE_H

#include "GraphCore/Node.h"

/**
 * \addtogroup MasterNodes Master Nodes
 */
/*@{*/

/**
 * @brief Represents a master node in the design
 *
 * Master nodes are artificial nodes which represent different boundaries of the design including inputs and outputs
 */
class MasterNode : public Node {
    friend class NodeFactory;

protected:
    MasterNode();

public:
    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

};

/*@}*/

#endif //VITIS_MASTERNODE_H