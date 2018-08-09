//
// Created by Christopher Yarp on 6/27/18.
//

#include "MasterOutput.h"

MasterOutput::MasterOutput() {

}

std::string MasterOutput::getCOutputName(int portNum) {
    return getInputPort(portNum)->getName() + "_" + std::to_string(portNum);
}
