//
// Created by Christopher Yarp on 2019-02-28.
//

#ifndef VITIS_DIGITALDEMODULATOR_H
#define VITIS_DIGITALDEMODULATOR_H

#include "MediumLevelNode.h"
#include "GraphCore/NodeFactory.h"

/**
 * \addtogroup MediumLevelNodes Medium Level Nodes
 */
/*@{*/

/**
 * @brief Represents a Digital Demodulator
 *
 * Is capable of demodulator BPSK, QPSK/4QAM, 16QAM
 *
 * Is capable of fixed distance between points (x & y dimensions) or avg power normalization
 */
class DigitalDemodulator {
friend NodeFactory;

};

/*@}*/

#endif //VITIS_DIGITALDEMODULATOR_H
