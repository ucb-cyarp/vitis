//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_MASTERINPUT_H
#define VITIS_MASTERINPUT_H

#include "MasterNode.h"

/**
 * \addtogroup MasterNodes Master Nodes
 */
/*@{*/

/**
 * @brief Represents the inputs to the data flow graph
 *
 * \ingroup MasterNodes
 */
class MasterInput : public MasterNode {

friend class NodeFactory;

protected:
    /**
     * @brief Default constructor.  Calls default constructor of supercass.
     */
    MasterInput();

public:
    /**
     * @brief Get the C name for the specified input
     *
     * The C name takes the form portName_portNum
     *
     * The port from which the name is taken is the corresponding output port to the Input Master
     *
     * @param portNum the port to get the C name for
     * @return a string with the C name for the specified output port
     */
    std::string getCInputName(int portNum);

};

/*@}*/

#endif //VITIS_MASTERINPUT_H
