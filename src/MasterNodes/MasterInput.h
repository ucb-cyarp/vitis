//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_MASTERINPUT_H
#define VITIS_MASTERINPUT_H

#include "GraphCore/Node.h"

/**
 * \addtogroup MasterNodes Master Nodes
 */
/*@{*/

/**
 * @brief Represents the inputs to the data flow graph
 *
 * \ingroup MasterNodes
 */
class MasterInput : public Node {
public:
    /**
     * @brief Default constructor.  Calls default constructor of supercass.
     */
    MasterInput();
};

/*@}*/

#endif //VITIS_MASTERINPUT_H
