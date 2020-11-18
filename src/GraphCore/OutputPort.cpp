//
// Created by Christopher Yarp on 7/17/18.
//

#include "OutputPort.h"
#include "DataType.h"
#include "Arc.h"
#include "Node.h"
#include "General/GeneralHelper.h"

OutputPort::OutputPort() : cEmittedRe(false), cEmittedIm(false) {

}

OutputPort::OutputPort(Node *parent, int portNum) : Port(parent, portNum), cEmittedRe(false), cEmittedIm(false) {

}

bool OutputPort::isCEmittedRe() const {
    return cEmittedRe;
}

void OutputPort::setCEmittedRe(bool cEmittedRe) {
    OutputPort::cEmittedRe = cEmittedRe;
}

bool OutputPort::isCEmittedIm() const {
    return cEmittedIm;
}

void OutputPort::setCEmittedIm(bool cEmittedIm) {
    OutputPort::cEmittedIm = cEmittedIm;
}

CExpr OutputPort::getCEmitReStr() const {
    return cEmitReStr;
}

void OutputPort::setCEmitReStr(const CExpr &cEmitReStr) {
    OutputPort::cEmitReStr = cEmitReStr;
}

CExpr OutputPort::getCEmitImStr() const {
    return cEmitImStr;
}

void OutputPort::setCEmitImStr(const CExpr &cEmitImStr) {
    OutputPort::cEmitImStr = cEmitImStr;
}



void OutputPort::validate() {
    if(arcs.size() < 1){
        throw std::runtime_error("Validation Failed - Output Port Unconnected (Should be Connected to Unconnected Master)");
    }

    auto firstArc = arcs.begin();

    DataType outputType = (*firstArc).lock()->getDataType();
    double outputSampleTime = (*firstArc).lock()->getSampleTime();

    for(auto arc = (firstArc++); arc != arcs.end(); arc++){
        if((*arc).lock()->getDataType() != outputType){
            throw std::runtime_error("Validation Failed - Output Port DataType Mismatch (" + parent->getFullyQualifiedName() + ")");
        }

        if((*arc).lock()->getSampleTime() != outputSampleTime){
            throw std::runtime_error("Validation Failed - Output Port SampleTime Mismatch (" + parent->getFullyQualifiedName() + ")");
        }
    }
}

std::shared_ptr<OutputPort> OutputPort::getSharedPointerOutputPort() {
    if(parent != nullptr) {
        return std::shared_ptr<OutputPort>(parent->getSharedPointer(), this);
    }
    else {
        //Hopefully should not happen but just in case
        //return std::shared_ptr<Port>(nullptr);
        throw std::runtime_error("Pointer requested from port that has no parent");
    }
}

std::string OutputPort::getCOutputVarNameBase() {
    return parent->getName() + "_n" + GeneralHelper::to_string(parent->getId()) + "_outPort" + GeneralHelper::to_string(portNum);;
}

Variable OutputPort::getCOutputVar() {
    std::string varName = getCOutputVarNameBase();

    DataType origType = getDataType();

    Variable var = Variable(varName, origType);

    return var;
}

std::string OutputPort::getCOutputVarName(bool imag) {
    std::string varName = getCOutputVarNameBase() + (imag ? VITIS_C_VAR_NAME_IM_SUFFIX : VITIS_C_VAR_NAME_RE_SUFFIX);

    return varName;
}