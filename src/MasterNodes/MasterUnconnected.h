//
// Created by Christopher Yarp on 6/27/18.
//

#ifndef VITIS_MASTERUNCONNECTED_H
#define VITIS_MASTERUNCONNECTED_H

#include "MasterNode.h"

/**
 * \addtogroup MasterNodes Master Nodes
 * @{
*/

/**
 * @brief Represents unconnected ports in the dataflow graph
 */
class MasterUnconnected : public MasterNode{

friend class NodeFactory;

protected:
    /**
     * @brief Default constructor.  Calls default constructor of supercass.
     */
    MasterUnconnected();
    MasterUnconnected(std::shared_ptr<SubSystem> parent, MasterNode* orig);

public:
    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

};

/*! @} */

#endif //VITIS_MASTERUNCONNECTED_H
