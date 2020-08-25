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
#include "GraphCore/DataType.h"
#include <algorithm>
#include <iostream>
#include <cstdio>

std::pair<std::map<EstimatorCommon::NodeOperation, int>, std::map<std::type_index, std::string>>
ComputationEstimator::reportComputeInstances(std::vector<std::shared_ptr<Node>> nodes, std::vector<std::shared_ptr<Node>> excludeNodes) {
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
        bool isExcluded = false;
        for(int j = 0; j<excludeNodes.size(); j++){
            if(nodes[i] == excludeNodes[j]){
                isExcluded = true;
                break;
            }
        }

        //Check if this node should be included
        bool isEnableInput = GeneralHelper::isType<Node,EnableInput>(nodes[i]) != nullptr;
        bool isOKSubsystem = GeneralHelper::isType<Node,ContextFamilyContainer>(nodes[i]) != nullptr || GeneralHelper::isType<Node,ContextContainer>(nodes[i]) != nullptr;
        bool isSubSystem = GeneralHelper::isType<Node, SubSystem>(nodes[i]) != nullptr;

        if(!isExcluded && (!isEnableInput && (!isSubSystem || isOKSubsystem))){
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
            int scalarInputs = 0;
            int vectorInputs = 0;
            int matrixInputs = 0;
            int maxElements = 0;
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
                if(portDT.isScalar()){
                    scalarInputs++;
                }else if(portDT.isVector()){
                    vectorInputs++;
                }else{
                    matrixInputs++;
                }
                maxElements = std::max(maxElements, portDT.numberOfElements());
            }

            EstimatorCommon::OperandType operandType = foundFloat ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT;
            EstimatorCommon::NodeOperation nodeOp(typeInd, operandType, bits, realInputs, cplxInputs, maxElements, scalarInputs, vectorInputs, matrixInputs);

            if(counts.find(nodeOp) == counts.end()){
                counts[nodeOp] = 1;
            }else{
                counts[nodeOp]++;
            }
        }
    }

    return std::pair<std::map<EstimatorCommon::NodeOperation, int>, std::map<std::type_index, std::string>>(counts, names);
}

EstimatorCommon::ComputeWorkload ComputationEstimator::reportComputeWorkload(std::vector<std::shared_ptr<Node>> nodes,
                                                       std::vector<std::shared_ptr<Node>> excludeNodes,
                                                       bool expandComplexOperators, bool expandHighLevelOperators,
                                                       ComputationEstimator::EstimatorOption includeIntermediateLoadStore,
                                                       ComputationEstimator::EstimatorOption includeInputOutputLoadStores){
    EstimatorCommon::ComputeWorkload workload;

    //TODO: Remove sanity check for duplicates
    std::set<std::shared_ptr<Node>> nodeSet;
    for(int i = 0; i<nodes.size(); i++){
        nodeSet.insert(nodes[i]);
    }
    if(nodeSet.size() != nodes.size()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Duplicate nodes in call to performance estimator"));
    }

    for(int i = 0; i<nodes.size(); i++){
        bool isExcluded = false;
        for(int j = 0; j<excludeNodes.size(); j++){
            if(nodes[i] == excludeNodes[j]){
                isExcluded = true;
                break;
            }
        }

        //Check if this node should be included
        bool isOKSubsystem = GeneralHelper::isType<Node,ContextFamilyContainer>(nodes[i]) != nullptr || GeneralHelper::isType<Node,ContextContainer>(nodes[i]) != nullptr;
        bool isSubSystem = GeneralHelper::isType<Node, SubSystem>(nodes[i]) != nullptr;

        if(!isExcluded && ((!isSubSystem || isOKSubsystem))){
            EstimatorCommon::ComputeWorkload nodeWorkload = nodes[i]->getComputeWorkloadEstimate(expandComplexOperators, expandHighLevelOperators, includeIntermediateLoadStore, includeInputOutputLoadStores);
            workload.addOperations(nodeWorkload);
        }
    }

    return workload;
}

EstimatorCommon::ComputeWorkload ComputationEstimator::breakVectorOps(EstimatorCommon::ComputeWorkload workload){
    EstimatorCommon::ComputeWorkload newWorkload;

    for(const auto & opCount : workload.operationCount){
        int opVecLen = opCount.first.vecLength;
        int brokenOpCount = opVecLen * opCount.second;
        EstimatorCommon::ComputeOperation scalarOp = opCount.first;
        scalarOp.vecLength = 1;

        std::map<EstimatorCommon::ComputeOperation, int> toAdd;
        toAdd[scalarOp] = brokenOpCount;
        newWorkload.addOperations(toAdd);
    }

    return newWorkload;
}

void ComputationEstimator::printComputeInstanceTable(
        std::map<int, std::map<EstimatorCommon::NodeOperation, int>> partitionOps,
        std::map<std::type_index, std::string> names) {

    //Operators in design
    std::set<EstimatorCommon::NodeOperation> operators;
    std::set<int> partitions;

    std::string portTypeLabel   = "Operand Type";
    std::string portWidthsLabel = "Operand Bits";
    std::string realPortsLabel  = "Real In Ports";
    std::string realCplxLabel   = "Cplx In Ports";
    std::string scalarPortsLabel = "Scalar In Ports";
    std::string vectorPortsLabel = "Vector In Ports";
    std::string matrixPortsLabel = "Matrix In Ports";
    std::string maxElementsLabel = "Max # Input Elements";

    //Used for formatting
    size_t maxDataStrLen = 0;
    size_t maxOperandTypeStrLen = portTypeLabel.size();
    size_t maxOperandWidthStrLen = portWidthsLabel.size();
    size_t maxRealPortsStrLen = realPortsLabel.size();
    size_t maxCplxPortsStrLen = realCplxLabel.size();
    size_t maxScalarPortsStrLen = scalarPortsLabel.size();
    size_t maxVectorPortsStrLen = vectorPortsLabel.size();
    size_t maxMatrixPortsStrLen = matrixPortsLabel.size();
    size_t maxMaxElementsStrLen = maxElementsLabel.size();

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
            maxScalarPortsStrLen = std::max(maxScalarPortsStrLen, GeneralHelper::to_string(opIt->first.scalarInputs).size());
            maxVectorPortsStrLen = std::max(maxVectorPortsStrLen, GeneralHelper::to_string(opIt->first.vectorInputs).size());
            maxMatrixPortsStrLen = std::max(maxMatrixPortsStrLen, GeneralHelper::to_string(opIt->first.matrixInputs).size());
            maxMaxElementsStrLen = std::max(maxMaxElementsStrLen, GeneralHelper::to_string(opIt->first.maxNumElements).size());
        }
    }

    std::string nodeTypeNameLabel = "Node Type";
    size_t maxTypeNamesStrLen = nodeTypeNameLabel.size();
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
    std::string maxScalarPortsFormatStr = " | %" + GeneralHelper::to_string(maxScalarPortsStrLen) + "d";
    std::string maxVectorPortsFormatStr = " | %" + GeneralHelper::to_string(maxVectorPortsStrLen) + "d";
    std::string maxMatrixPortsFormatStr = " | %" + GeneralHelper::to_string(maxMatrixPortsStrLen) + "d";
    std::string maxMaxElementsFormatStr = " | %" + GeneralHelper::to_string(maxMaxElementsStrLen) + "d";
    std::string maxScalarPortsLabelFormatStr = " | %" + GeneralHelper::to_string(maxScalarPortsStrLen) + "s";
    std::string maxVectorPortsLabelFormatStr = " | %" + GeneralHelper::to_string(maxVectorPortsStrLen) + "s";
    std::string maxMatrixPortsLabelFormatStr = " | %" + GeneralHelper::to_string(maxMatrixPortsStrLen) + "s";
    std::string maxMaxElementsLabelFormatStr = " | %" + GeneralHelper::to_string(maxMaxElementsStrLen) + "s";
    std::string countFormatStr = " | %"+GeneralHelper::to_string(maxDataStrLen)+"d";

    printf(nameFormatStr.c_str(), nodeTypeNameLabel.c_str());
    printf(operandFormatStr.c_str(), portTypeLabel.c_str());
    printf(operandBitsLabelFormatStr.c_str(), portWidthsLabel.c_str());
    printf(maxRealPortsLabelFormatStr.c_str(), realPortsLabel.c_str());
    printf(maxCplxPortsLabelFormatStr.c_str(), realCplxLabel.c_str());
    printf(maxScalarPortsLabelFormatStr.c_str(), scalarPortsLabel.c_str());
    printf(maxVectorPortsLabelFormatStr.c_str(), vectorPortsLabel.c_str());
    printf(maxMatrixPortsLabelFormatStr.c_str(), matrixPortsLabel.c_str());
    printf(maxMaxElementsLabelFormatStr.c_str(), maxElementsLabel.c_str());

    for(auto part = partitions.begin(); part != partitions.end(); part++){
        printf(countFormatStr.c_str(), *part);
    }
    std::cout << std::endl;

    //Sorting operators (note, comparator & set would need to be changed if any new operator node types are added later)
    EstimatorCommon::NodeOperationComparatorByName comparator(names);
    std::set<EstimatorCommon::NodeOperation, EstimatorCommon::NodeOperationComparatorByName> operatorSorted(comparator);
    operatorSorted.insert(operators.begin(), operators.end());

    //Print the rows (the operator)
    for(auto op = operatorSorted.begin(); op != operatorSorted.end(); op++){
        //Print the details of the
        std::string opName = names[op->nodeType];
        printf(nameFormatStr.c_str(), opName.c_str());

        std::string operandStr = EstimatorCommon::operandTypeToString(op->operandType).c_str();
        printf(operandFormatStr.c_str(), operandStr.c_str());

        printf(operandBitsFormatStr.c_str(), op->operandBits);

        printf(maxRealPortsFormatStr.c_str(), op->numRealInputs);
        printf(maxCplxPortsFormatStr.c_str(), op->numCplxInputs);
        printf(maxScalarPortsFormatStr.c_str(), op->scalarInputs);
        printf(maxVectorPortsFormatStr.c_str(), op->vectorInputs);
        printf(maxMatrixPortsFormatStr.c_str(), op->matrixInputs);
        printf(maxMaxElementsFormatStr.c_str(), op->maxNumElements);

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

void ComputationEstimator::printWorkloadTable(const std::map<int, EstimatorCommon::ComputeWorkload> &partitionOpsOrig, bool breakVectorOps_param){
    std::map<int, EstimatorCommon::ComputeWorkload> partitionOps;
    if(breakVectorOps_param){
        //Break vector ops into scalar ops
        for(const auto & partitionOpOrig : partitionOpsOrig){
            int partition = partitionOpOrig.first;
            EstimatorCommon::ComputeWorkload brokenWorkload = breakVectorOps(partitionOpOrig.second);
            partitionOps[partition] = brokenWorkload;
        }
    }else{
        //Use the origional workload without vector ops broken down
        partitionOps = partitionOpsOrig;
    }

    //Operators in design
    std::set<EstimatorCommon::ComputeOperation> operators;
    std::set<int> partitions;

    std::string portTypeLabel       = "Operand Type";
    std::string portWidthsLabel     = "Operand Bits";
    std::string portComplexityLabel = "Operand Complexity";
    std::string vectorLenLabel      = "Vector Len";

    //Used for formatting
    size_t maxDataStrLen = 0;
    size_t maxOperandTypeStrLen = portTypeLabel.size();
    size_t maxOperandWidthStrLen = portWidthsLabel.size();
    size_t maxOperandComplexityStrLen = portComplexityLabel.size();
    size_t maxVectorLenStrLen = vectorLenLabel.size();

    std::string opTypeNameLabel = "Op Type";
    size_t maxTypeNamesStrLen = opTypeNameLabel.size();
    for(const auto & partitionOp : partitionOps){
        partitions.insert(partitionOp.first);
        std::string partStr = GeneralHelper::to_string(partitionOp.first);
        maxDataStrLen = std::max(maxDataStrLen, partStr.size());
        for(const auto & opCount : partitionOp.second.operationCount){
            operators.insert(opCount.first);
            maxDataStrLen = std::max(maxDataStrLen, GeneralHelper::to_string(opCount.second).size());
            maxTypeNamesStrLen = std::max(maxTypeNamesStrLen, EstimatorCommon::opTypeToStr(opCount.first.opType).size());
            maxOperandTypeStrLen = std::max(maxOperandTypeStrLen, EstimatorCommon::operandTypeToString(opCount.first.operandType).size());
            maxOperandWidthStrLen = std::max(maxOperandWidthStrLen, GeneralHelper::to_string(opCount.first.operandBits).size());
            maxOperandComplexityStrLen = std::max(maxOperandComplexityStrLen, EstimatorCommon::operandComplexityToString(opCount.first.operandComplexity).size());
            maxVectorLenStrLen = std::max(maxVectorLenStrLen, GeneralHelper::to_string(opCount.first.vecLength).size());
        }
    }

    //Print the table

    //Print the header
    std::string nameFormatStr = "%" + GeneralHelper::to_string(maxTypeNamesStrLen) + "s";
    std::string operandTypeFormatStr = " | %" + GeneralHelper::to_string(maxOperandTypeStrLen) + "s";
    std::string operandBitsFormatStr = " | %" + GeneralHelper::to_string(maxOperandWidthStrLen) + "d";
    std::string operandBitsLabelFormatStr = " | %" + GeneralHelper::to_string(maxOperandWidthStrLen) + "s";
    std::string operandComplexityFormatStr = " | %" + GeneralHelper::to_string(maxOperandComplexityStrLen) + "s";
    std::string operandVectorLenFormatStr = " | %" + GeneralHelper::to_string(maxVectorLenStrLen) + "d";
    std::string operandVectorLenLabelFormatStr = " | %" + GeneralHelper::to_string(maxVectorLenStrLen) + "s";
    std::string countFormatStr = " | %"+GeneralHelper::to_string(maxDataStrLen)+"d";

    printf(nameFormatStr.c_str(), opTypeNameLabel.c_str());
    printf(operandTypeFormatStr.c_str(), portTypeLabel.c_str());
    printf(operandBitsLabelFormatStr.c_str(), portWidthsLabel.c_str());
    printf(operandComplexityFormatStr.c_str(), portComplexityLabel.c_str());
    if(!breakVectorOps_param) {
        printf(operandVectorLenLabelFormatStr.c_str(), vectorLenLabel.c_str());
    }

    for(auto part = partitions.begin(); part != partitions.end(); part++){
        printf(countFormatStr.c_str(), *part);
    }
    std::cout << std::endl;

    //Print the rows (the operator)
    for(const auto &op : operators){
        //Print the details of the
        std::string opName = EstimatorCommon::opTypeToStr(op.opType);
        printf(nameFormatStr.c_str(), opName.c_str());

        printf(operandTypeFormatStr.c_str(), EstimatorCommon::operandTypeToString(op.operandType).c_str());
        printf(operandBitsFormatStr.c_str(), op.operandBits);
        printf(operandComplexityFormatStr.c_str(), EstimatorCommon::operandComplexityToString(op.operandComplexity).c_str());
        if(!breakVectorOps_param) {
            printf(operandVectorLenFormatStr.c_str(), op.vecLength);
        }

        for(const auto &part : partitions) {
            //Print the count for each partition

            if(partitionOps[part].operationCount.find(op) != partitionOps[part].operationCount.end()){
                printf(countFormatStr.c_str(), partitionOps[part].operationCount[op]);
            }else{
                std::cout << " | " << GeneralHelper::getSpaces(maxDataStrLen);
            }
        }

        std::cout << std::endl;
    }
}

EstimatorCommon::ComputeWorkload ComputationEstimator::getIOLoadStoreWorkloadEst(
        std::vector<std::shared_ptr<InputPort>> &inputPorts, std::vector<std::shared_ptr<OutputPort>> &outputPorts){
    EstimatorCommon::ComputeWorkload workload;

    for(const auto &inputPort : inputPorts){
        DataType dataType = inputPort->getDataType();

        EstimatorCommon::ComputeOperation op(EstimatorCommon::OpType::LOAD,
                                             (dataType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT :
                                                                        EstimatorCommon::OperandType::INT),
                                             (dataType.isComplex() ? EstimatorCommon::OperandComplexity::COMPLEX :
                                                                     EstimatorCommon::OperandComplexity::REAL),
                                             dataType.getTotalBits(), dataType.numberOfElements());
        workload.addOperation(op);
    }

    for(const auto &outputPort : outputPorts){
        DataType dataType = outputPort->getDataType();

        EstimatorCommon::ComputeOperation op(EstimatorCommon::OpType::STORE,
                                             (dataType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT :
                                              EstimatorCommon::OperandType::INT),
                                             (dataType.isComplex() ? EstimatorCommon::OperandComplexity::COMPLEX :
                                              EstimatorCommon::OperandComplexity::REAL),
                                             dataType.getTotalBits(), dataType.numberOfElements());
        workload.addOperation(op);
    }

    return workload;
}

EstimatorCommon::ComputeWorkload ComputationEstimator::getIOLoadStoreWorkloadEst(std::shared_ptr<Node> node, EstimatorOption includeLoadStoreOps){
    std::vector<std::shared_ptr<InputPort>> inputPorts = node->getInputPorts();
    std::vector<std::shared_ptr<OutputPort>> outputPorts = node->getOutputPorts();

    if(includeLoadStoreOps == EstimatorOption::ENABLED ||
      (includeLoadStoreOps == EstimatorOption::NONSCALAR_ONLY && EstimatorCommon::containsNonScalarInputOrOutput(node))){
        return getIOLoadStoreWorkloadEst(inputPorts, outputPorts);
    }else if(includeLoadStoreOps == EstimatorOption::DISABLED){
        return EstimatorCommon::ComputeWorkload();
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown EstimatorOption"));
    }
}
