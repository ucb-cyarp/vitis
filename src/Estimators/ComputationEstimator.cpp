//
// Created by Christopher Yarp on 10/9/19.
//

#include "General/ErrorHelpers.h"
#include "General/GeneralHelper.h"
#include "ComputationEstimator.h"
#include "GraphCore/EnableInput.h"
#include "GraphCore/ContextFamilyContainer.h"
#include "GraphCore/ContextContainer.h"
#include "GraphCore/SubSystem.h"
#include <algorithm>
#include <iostream>
#include <cstdio>

std::pair<std::map<EstimatorCommon::NodeOperation, int>, std::map<std::type_index, std::string>>
ComputationEstimator::reportComputeInstances(std::vector<std::shared_ptr<Node>> nodes) {
    std::map<EstimatorCommon::NodeOperation, int> counts;
    std::map<std::type_index, std::string> names;

    //TODO: Remove sanity check for duplicates
    std::set<std::shared_ptr<Node>> nodeSet;
    for(int i = 0; i<nodes.size(); i++){
        nodeSet.insert(nodes[i]);
    }
    if(nodeSet.size() != nodes.size()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Duplicate nodes in call to performance estimator"));
    }

    for(int i = 0; i<nodes.size(); i++){
        //Check if this node should be included
        bool isEnableInput = GeneralHelper::isType<Node,EnableInput>(nodes[i]) != nullptr;
        bool isOKSubsystem = GeneralHelper::isType<Node,ContextFamilyContainer>(nodes[i]) != nullptr || GeneralHelper::isType<Node,ContextContainer>(nodes[i]) != nullptr;
        bool isSubSystem = GeneralHelper::isType<Node, SubSystem>(nodes[i]) != nullptr;

        if(!isEnableInput && (!isSubSystem || isOKSubsystem)){
            //To avoid a compiler side-effect warning for typeid, we need to access the raw pointer https://stackoverflow.com/questions/33329531/c-equivalent-of-printf-or-formatted-output
            auto *rawPtr = nodes[i].get();
            std::type_index typeInd = std::type_index(typeid(*rawPtr)); //Note, need the typeID to be of the underlying class under the std::shared_ptr
            if(names.find(typeInd) == names.end()) {
                names[typeInd] = nodes[i]->typeNameStr();
            }

            int realInputs = 0;
            int cplxInputs = 0;
            bool foundFloat = false;
            int bits = 0;
            std::vector<std::shared_ptr<InputPort>> inputPorts = nodes[i]->getInputPorts();
            for(int j = 0; j < inputPorts.size(); j++){
                DataType portDT = inputPorts[j]->getDataType();
                if(portDT.isComplex()){
                    cplxInputs++;
                }else{
                    realInputs++;
                }
                foundFloat |= portDT.isFloatingPt();
                bits = std::max(bits, portDT.getTotalBits());
            }

            EstimatorCommon::OperandType operandType = foundFloat ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT;
            EstimatorCommon::NodeOperation nodeOp(typeInd, operandType, bits, realInputs, cplxInputs);

            if(counts.find(nodeOp) == counts.end()){
                counts[nodeOp] = 1;
            }else{
                counts[nodeOp]++;
            }
        }
    }

    return std::pair<std::map<EstimatorCommon::NodeOperation, int>, std::map<std::type_index, std::string>>(counts, names);
}

void ComputationEstimator::printComputeInstanceTable(
        std::map<int, std::map<EstimatorCommon::NodeOperation, int>> partitionOps,
        std::map<std::type_index, std::string> names) {

    //Operators in design
    std::set<EstimatorCommon::NodeOperation> operators;
    std::set<int> partitions;

    std::string portTypeLabel  = "Operand Type";
    std::string portWidthsLabel = "Operand Bits";
    std::string realPortsLabel = "Real In Ports";
    std::string realCplxLabel  = "Cplx In Ports";

    //Used for formatting
    size_t maxDataStrLen = 0;
    size_t maxOperandTypeStrLen = portTypeLabel.size();
    size_t maxOperandWidthStrLen = portWidthsLabel.size();
    size_t maxRealPortsStrLen = realPortsLabel.size();
    size_t maxCplxPortsStrLen = realCplxLabel.size();

    for(auto partIt = partitionOps.begin(); partIt != partitionOps.end(); partIt++){
        partitions.insert(partIt->first);
        std::string partStr = GeneralHelper::to_string(partIt->first);
        maxDataStrLen = std::max(maxDataStrLen, partStr.size());
        for(auto opIt = partIt->second.begin(); opIt != partIt->second.end(); opIt++){
            operators.insert(opIt->first);
            maxDataStrLen = std::max(maxDataStrLen, GeneralHelper::to_string(opIt->second).size());
            maxOperandTypeStrLen = std::max(maxOperandTypeStrLen, EstimatorCommon::operandTypeToString(opIt->first.operandType).size());
            maxOperandWidthStrLen = std::max(maxOperandWidthStrLen, GeneralHelper::to_string(opIt->first.operandBits).size());
            maxRealPortsStrLen = std::max(maxRealPortsStrLen, GeneralHelper::to_string(opIt->first.numRealInputs).size());
            maxCplxPortsStrLen = std::max(maxCplxPortsStrLen, GeneralHelper::to_string(opIt->first.numCplxInputs).size());
        }
    }

    std::string nodeTypeNameLabel = "Node Type";
    size_t maxTypeNamesStrLen = sizeof(nodeTypeNameLabel);
    for(auto it = names.begin(); it!=names.end(); it++){
        maxTypeNamesStrLen = std::max(maxTypeNamesStrLen, it->second.size());
    }

    //Print the table

    //Print the header
    std::string nameFormatStr = "%" + GeneralHelper::to_string(maxTypeNamesStrLen) + "s";
    std::string operandFormatStr = " | %" + GeneralHelper::to_string(maxOperandTypeStrLen) + "s";
    std::string operandBitsFormatStr = " | %" + GeneralHelper::to_string(maxOperandWidthStrLen) + "d";
    std::string maxRealPortsFormatStr = " | %" + GeneralHelper::to_string(maxRealPortsStrLen) + "d";
    std::string maxCplxPortsFormatStr = " | %" + GeneralHelper::to_string(maxCplxPortsStrLen) + "d";
    std::string operandBitsLabelFormatStr = " | %" + GeneralHelper::to_string(maxOperandWidthStrLen) + "s";
    std::string maxRealPortsLabelFormatStr = " | %" + GeneralHelper::to_string(maxRealPortsStrLen) + "s";
    std::string maxCplxPortsLabelFormatStr = " | %" + GeneralHelper::to_string(maxCplxPortsStrLen) + "s";
    std::string countFormatStr = " | %"+GeneralHelper::to_string(maxDataStrLen)+"d";

    printf(nameFormatStr.c_str(), nodeTypeNameLabel.c_str());
    printf(operandFormatStr.c_str(), portTypeLabel.c_str());
    printf(operandBitsLabelFormatStr.c_str(), portWidthsLabel.c_str());
    printf(maxRealPortsLabelFormatStr.c_str(), realPortsLabel.c_str());
    printf(maxCplxPortsLabelFormatStr.c_str(), realCplxLabel.c_str());

    for(auto part = partitions.begin(); part != partitions.end(); part++){
        printf(countFormatStr.c_str(), *part);
    }
    std::cout << std::endl;

    //Print the rows (the operator)
    for(auto op = operators.begin(); op != operators.end(); op++){
        //Print the details of the
        std::string opName = names[op->nodeType];
        printf(nameFormatStr.c_str(), opName.c_str());

        std::string operandStr = EstimatorCommon::operandTypeToString(op->operandType).c_str();
        printf(operandFormatStr.c_str(), operandStr.c_str());

        printf(operandBitsFormatStr.c_str(), op->operandBits);

        printf(maxRealPortsFormatStr.c_str(), op->numRealInputs);

        printf(maxCplxPortsFormatStr.c_str(), op->numCplxInputs);

        for(auto part = partitions.begin(); part != partitions.end(); part++) {
            //Print the count for each partition

            if(partitionOps[*part].find(*op) != partitionOps[*part].end()){
                printf(countFormatStr.c_str(), partitionOps[*part][*op]);
            }else{
                std::cout << " | " << GeneralHelper::getSpaces(maxDataStrLen);
            }
        }

        std::cout << std::endl;
    }
}
