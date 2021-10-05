//
// Created by Christopher Yarp on 6/27/18.
//

#ifndef VITIS_MASTEROUTPUT_H
#define VITIS_MASTEROUTPUT_H

#include "MasterNode.h"
#include <string>

/**
 * \addtogroup MasterNodes Master Nodes
 * @{
*/

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
    MasterOutput(std::shared_ptr<SubSystem> parent, MasterNode* orig);

public:
    /**
     * @brief Get the C name for the specified output
     *
     * The C name takes the form portName_outPort<portNum>
     *
     * The port from which the name is taken is the corresponding input port to the Output Master
     *
     * @param portNum the port to get the C name for
     * @return a string with the C name for the specified output port
     */
    std::string getCOutputName(int portNum);

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    std::string typeNameStr() override;

    /**
     * @brief Sets the port block sizes based on the associated clock domains.
     *
     * @warning Port clock domains must be set first
     *
     * @note Compensates for ports operating in the base clock domain which go into BlockingBoundary nodes
     *       and are already scaled by the block size.  The block size for these nodes is set to 1.
     *
     * @param baseBlockSize the block size of items operating at the base rate
     */
    void setPortBlockSizesBasedOnClockDomain(int baseBlockSize);
};

/*! @} */

#endif //VITIS_MASTEROUTPUT_H
