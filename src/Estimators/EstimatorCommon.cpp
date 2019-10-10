//
// Created by Christopher Yarp on 10/9/19.
//

#include <General/ErrorHelpers.h>
#include "EstimatorCommon.h"

EstimatorCommon::NodeOperation::NodeOperation(std::type_index nodeType, EstimatorCommon::OperandType operandType,
                                              int operandBits, int numRealInputs, int numCplxInputs) :
        nodeType(nodeType), operandType(operandType),
        operandBits(operandBits), numRealInputs(numRealInputs),
        numCplxInputs(numCplxInputs){

}

bool EstimatorCommon::NodeOperation::operator==(const EstimatorCommon::NodeOperation &rhs) const {
    return nodeType == rhs.nodeType &&
           operandType == rhs.operandType &&
           operandBits == rhs.operandBits &&
           numRealInputs == rhs.numRealInputs &&
           numCplxInputs == rhs.numCplxInputs;
}

bool EstimatorCommon::NodeOperation::operator!=(const EstimatorCommon::NodeOperation &rhs) const {
    return !(rhs == *this);
}

bool EstimatorCommon::NodeOperation::operator<(const EstimatorCommon::NodeOperation &rhs) const {
    if (nodeType < rhs.nodeType)
        return true;
    if (rhs.nodeType < nodeType)
        return false;
    if (operandType < rhs.operandType)
        return true;
    if (rhs.operandType < operandType)
        return false;
    if (operandBits < rhs.operandBits)
        return true;
    if (rhs.operandBits < operandBits)
        return false;
    if (numRealInputs < rhs.numRealInputs)
        return true;
    if (rhs.numRealInputs < numRealInputs)
        return false;
    return numCplxInputs < rhs.numCplxInputs;
}

bool EstimatorCommon::NodeOperation::operator>(const EstimatorCommon::NodeOperation &rhs) const {
    return rhs < *this;
}

bool EstimatorCommon::NodeOperation::operator<=(const EstimatorCommon::NodeOperation &rhs) const {
    return !(rhs < *this);
}

bool EstimatorCommon::NodeOperation::operator>=(const EstimatorCommon::NodeOperation &rhs) const {
    return !(*this < rhs);
}

std::string EstimatorCommon::operandTypeToString(EstimatorCommon::OperandType operandType) {
    switch(operandType){
        case EstimatorCommon::OperandType::FLOAT:
            return "FLOAT";
        case EstimatorCommon::OperandType::INT:
            return "INT";
        default:
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown OperandType"));
    }
}
