//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_MASTERINPUT_H
#define VITIS_MASTERINPUT_H

#include "MasterNode.h"

/**
 * \addtogroup MasterNodes Master Nodes
 * @{
*/

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
    MasterInput(std::shared_ptr<SubSystem> parent, MasterNode* orig);

public:
    /**
     * @brief Get the C name for the specified input
     *
     * The C name takes the form portName_inPort<portNum>
     *
     * The port from which the name is taken is the corresponding output port to the Input Master
     *
     * @param portNum the port to get the C name for
     * @return a string with the C name for the specified output port
     */
    std::string getCInputName(int portNum);

//    /**
//     * @brief For the MasterInput, fanout logic is not handled because the function simply returns the name of the function arg
//     *
//     */
//    std::string emitC(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag = false, bool checkFanout = true, bool forceFanout = false) override;

    /**
     * @brief Emits the name of the input argument for the given input port
     */
    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                    bool imag = false) override;


    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    std::string typeNameStr() override;

    /**
     * @brief Sets the port block sizes based on the associated clock domains.
     *
     * @warning Port clock domains must be set first
     *
     * @param baseBlockSize the block size of items operating at the base rate
     */
    void setPortBlockSizesBasedOnClockDomain(int baseBlockSize);

};

/*! @} */

#endif //VITIS_MASTERINPUT_H
