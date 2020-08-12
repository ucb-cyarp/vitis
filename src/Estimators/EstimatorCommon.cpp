//
// Created by Christopher Yarp on 10/9/19.
//

#include <General/ErrorHelpers.h>
#include "EstimatorCommon.h"

EstimatorCommon::NodeOperation::NodeOperation(std::type_index nodeType, OperandType operandType, int operandBits,
                                              int numRealInputs, int numCplxInputs, int maxNumElements,
                                              int scalarInputs, int vectorInputs,  int matrixInputs) :
        nodeType(nodeType), operandType(operandType),
        operandBits(operandBits), numRealInputs(numRealInputs),
        numCplxInputs(numCplxInputs), maxNumElements(maxNumElements),
        scalarInputs(scalarInputs), vectorInputs(vectorInputs), matrixInputs(matrixInputs){

}

bool EstimatorCommon::NodeOperation::operator==(const EstimatorCommon::NodeOperation &rhs) const {
    return nodeType == rhs.nodeType &&
           operandType == rhs.operandType &&
           operandBits == rhs.operandBits &&
           numRealInputs == rhs.numRealInputs &&
           numCplxInputs == rhs.numCplxInputs &&
           maxNumElements == rhs.maxNumElements &&
           scalarInputs == rhs.scalarInputs &&
           vectorInputs == rhs.vectorInputs &&
           matrixInputs == rhs.matrixInputs;
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
    if (numCplxInputs < rhs.numCplxInputs)
        return true;
    if (rhs.numCplxInputs < numCplxInputs)
        return false;
    if (maxNumElements < rhs.maxNumElements)
        return true;
    if (rhs.maxNumElements < maxNumElements)
        return false;
    if (scalarInputs < rhs.scalarInputs)
        return true;
    if (rhs.scalarInputs < scalarInputs)
        return false;
    if (vectorInputs < rhs.vectorInputs)
        return true;
    if (rhs.vectorInputs < vectorInputs)
        return false;
    return matrixInputs < rhs.matrixInputs;
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

std::string EstimatorCommon::operandComplexityToString(OperandComplexity operandComplexity){
    switch(operandComplexity){
        case EstimatorCommon::OperandComplexity::REAL:
            return "Real";
        case EstimatorCommon::OperandComplexity::COMPLEX:
            return "Complex";
        case EstimatorCommon::OperandComplexity::MIXED:
            return "Mixed";
        default:
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown OperandComplexity"));
    }
}

std::string EstimatorCommon::opTypeToStr(OpType opType){
    switch(opType){
        case EstimatorCommon::OpType::ADD_SUB:
            return "Add/Sub";
        case EstimatorCommon::OpType::MULT:
            return "Mult";
        case EstimatorCommon::OpType::DIV:
            return "Div";
        case EstimatorCommon::OpType::BITWISE:
            return "Bitwise";
        case EstimatorCommon::OpType::LOGICAL:
            return "Logical";
        case EstimatorCommon::OpType::CAST:
            return "Cast";
        case EstimatorCommon::OpType::LOAD:
            return "Load";
        case EstimatorCommon::OpType::STORE:
            return "Store";
        case EstimatorCommon::OpType::BRANCH:
            return "Branch";
        case EstimatorCommon::OpType::EXP:
            return "Exp";
        case EstimatorCommon::OpType::LN:
            return "Ln";
        case EstimatorCommon::OpType::SIN:
            return "Sin";
        case EstimatorCommon::OpType::COS:
            return "Cos";
        case EstimatorCommon::OpType::TAN:
            return "Tan";
        case EstimatorCommon::OpType::ATAN:
            return "Atan";
        case EstimatorCommon::OpType::ATAN2:
            return "Atan2";
        default:
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown OpType"));
    }
}

bool EstimatorCommon::isPrimitiveOpType(OpType opType){
    switch(opType){
        case EstimatorCommon::OpType::ADD_SUB:
            return true;
        case EstimatorCommon::OpType::MULT:
            return true;
        case EstimatorCommon::OpType::DIV:
            return true;
        case EstimatorCommon::OpType::BITWISE:
            return true;
        case EstimatorCommon::OpType::LOGICAL:
            return true;
        case EstimatorCommon::OpType::CAST:
            return true;
        case EstimatorCommon::OpType::LOAD:
            return true;
        case EstimatorCommon::OpType::STORE:
            return true;
        case EstimatorCommon::OpType::BRANCH:
            return true;
        case EstimatorCommon::OpType::EXP:
            return false;
        case EstimatorCommon::OpType::LN:
            return false;
        case EstimatorCommon::OpType::SIN:
            return false;
        case EstimatorCommon::OpType::COS:
            return false;
        case EstimatorCommon::OpType::TAN:
            return false;
        case EstimatorCommon::OpType::ATAN:
            return false;
        case EstimatorCommon::OpType::ATAN2:
            return false;
        default:
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown OpType"));
    }
}

EstimatorCommon::NodeOperationComparatorByName::NodeOperationComparatorByName(
        std::map<std::type_index, std::string> nameMapping) : nameMapping(nameMapping) {}

bool EstimatorCommon::NodeOperationComparatorByName::comp(const NodeOperation &a, const NodeOperation &b) const{
    //Check for the string name first
    if(nameMapping.at(a.nodeType) < nameMapping.at(b.nodeType)){
        return true;
    }else if(nameMapping.at(a.nodeType) > nameMapping.at(b.nodeType)){
        return false;
    }

    //Then use the orig relational operator names are equal
    return a<b;
}

bool EstimatorCommon::NodeOperationComparatorByName::operator()(const EstimatorCommon::NodeOperation &a,
                                                                const EstimatorCommon::NodeOperation &b) const {
    return comp(a, b);
}

EstimatorCommon::InterThreadCommunicationWorkload::InterThreadCommunicationWorkload(int numBytesPerSample,
        int numBytesPerBlock, int numFIFOs) : numBytesPerSample(numBytesPerSample),
        numBytesPerBlock(numBytesPerBlock), numFIFOs(numFIFOs) {
}

EstimatorCommon::InterThreadCommunicationWorkload::InterThreadCommunicationWorkload() :
    numBytesPerSample(0), numBytesPerBlock(0), numFIFOs(0){

}
