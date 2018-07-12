//
// Created by Christopher Yarp on 6/27/18.
//

#ifndef VITIS_MASTEROUTPUT_H
#define VITIS_MASTEROUTPUT_H

#include "MasterNode.h"

/**
 * \addtogroup MasterNodes Master Nodes
 */
/*@{*/

/**
 * @brief Represents the outputs from the data flow graph
 */
class MasterOutput : public MasterNode{

friend class NodeFactory;

protected:
    /**
     * @brief Default constructor.  Calls default constructor of supercass.
     */
    MasterOutput();
};

/*@}*/

#endif //VITIS_MASTEROUTPUT_H
