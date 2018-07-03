//
// Created by Christopher Yarp on 6/27/18.
//

#ifndef VITIS_MASTERUNCONNECTED_H
#define VITIS_MASTERUNCONNECTED_H

#include "GraphCore/Node.h"

/**
 * \addtogroup MasterNodes Master Nodes
 */
/*@{*/

/**
 * @brief Represents unconnected ports in the dataflow graph
 */
class MasterUnconnected : public Node{

friend class NodeFactory;

protected:
    /**
     * @brief Default constructor.  Calls default constructor of supercass.
     */
    MasterUnconnected();

};

/*@}*/

#endif //VITIS_MASTERUNCONNECTED_H
