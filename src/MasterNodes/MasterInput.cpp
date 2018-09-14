//
// Created by Christopher Yarp on 6/26/18.
//

#include "MasterInput.h"

MasterInput::MasterInput() {

}


std::string MasterInput::getCInputName(int portNum) {
    return getOutputPort(portNum)->getName() + "_inPort" + std::to_string(portNum);
}

//std::string
//MasterInput::emitC(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag, bool checkFanout,
//                   bool forceFanout) {
//    return emitCExpr(cStatementQueue, outputPortNum, imag).getExpr();
//}

CExpr MasterInput::emitCExpr(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag) {
    //Get a Dummy Variable Object So We can use its formatter
    Variable var;
    var.setName(getCInputName(outputPortNum));
    var.setDataType(getOutputPort(outputPortNum)->getDataType());

    return CExpr(var.getCVarName(imag), true);
}
