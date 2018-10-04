//
// Created by Christopher Yarp on 6/27/18.
//

#include "MasterOutput.h"
#include "General/GeneralHelper.h"

MasterOutput::MasterOutput() {

}

std::string MasterOutput::getCOutputName(int portNum) {
    return getInputPort(portNum)->getName() + "_outPort" + GeneralHelper::to_string(portNum);
}
