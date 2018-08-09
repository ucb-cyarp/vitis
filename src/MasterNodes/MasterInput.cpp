//
// Created by Christopher Yarp on 6/26/18.
//

#include "MasterInput.h"

MasterInput::MasterInput() {

}


std::string MasterInput::getCInputName(int portNum) {
    return getOutputPort(portNum)->getName() + "_" + std::to_string(portNum);
}