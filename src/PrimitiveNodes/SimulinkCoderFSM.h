//
// Created by Christopher Yarp on 2019-01-28.
//

#ifndef VITIS_SIMULINKCODERFSM_H
#define VITIS_SIMULINKCODERFSM_H

#include "BlackBox.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 */
/*@{*/

/**
 * @brief
 *
 * @note Stateflow does not create a seperate state update function - it is all contained inside of a single function.
 * To handle this, we declare this blackbox as stateful but put in an empty string for the stateUpdate name.
 * We also modify the stateUpdate wiring to not create state update nodes for this block.
 */
class SimulinkCoderFSM : public BlackBox {

};

/*@}*/


#endif //VITIS_SIMULINKCODERFSM_H
