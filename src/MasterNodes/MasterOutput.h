//
// Created by Christopher Yarp on 6/27/18.
//

#ifndef VITIS_MASTEROUTPUT_H
#define VITIS_MASTEROUTPUT_H

#include "MasterNode.h"
#include <string>

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

public:
    /**
     * @brief Get the C name for the specified output
     *
     * The C name takes the form portName_portNum
     *
     * The port from which the name is taken is the corresponding input port to the Output Master
     *
     * @param portNum the port to get the C name for
     * @return a string with the C name for the specified output port
     */
    std::string getCOutputName(int portNum);
};

/*@}*/

#endif //VITIS_MASTEROUTPUT_H
