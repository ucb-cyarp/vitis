//
// Created by Christopher Yarp on 10/9/19.
//

#include "CommunicationEstimator.h"

#include "General/GeneralHelper.h"
#include "GraphCore/DataType.h"

#include <iostream>

std::map<std::pair<int, int>, EstimatorCommon::InterThreadCommunicationWorkload>
CommunicationEstimator::reportCommunicationWorkload(
    //TODO: Fix with multiple block lengths for FIFOs.  May actually be OK as is, may just need to remove check
    std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifoMap) {
    std::map<std::pair<int, int>, EstimatorCommon::InterThreadCommunicationWorkload> workload;

    int blockSize = 0;
    bool foundBlockSize = false;
    bool foundConflictingBlockSize = false;
    for(auto it = fifoMap.begin(); it != fifoMap.end(); it++){
        int bytesPerSample = 0;
        int bytesPerTransaction = 0;
        int numFIFOs = it->second.size();

        for(int i = 0; i<it->second.size(); i++){
            std::vector<std::shared_ptr<InputPort>> ports = it->second[i]->getInputPorts();

            if(!foundBlockSize){
                blockSize = it->second[i]->getBlockSize();
                foundBlockSize = true;
            }else{
                if(!foundConflictingBlockSize){//Only report once
                    if(it->second[i]->getBlockSize() != blockSize) {
                        std::cerr << "A conflicting FIFO block size has been found.  Communication Load Estimations for Transaction Size may be inaccurate." << std::endl;
                        foundConflictingBlockSize = true;
                    }
                }
            }

            for(int j = 0; j<ports.size(); j++){
                DataType portDT = ports[j]->getDataType();
                //Communication occurs with valid CPU types
                //TODO: change if bit fields are used or bools are packed
                DataType cpuType = portDT.getCPUStorageType();

                int bits = cpuType.getTotalBits();
                //TODO: change if bool type is updated
                if(bits == 1){
                    bits = 8;
                }

                if(portDT.isComplex()){
                    bits *=2; //If complex, double the amount of data transfered (to include the imag component)
                }

                int bytes = bits/8;
                bytesPerSample += bytes;
                bytesPerTransaction += bytes*it->second[i]->getBlockSize();
            }
        }

        EstimatorCommon::InterThreadCommunicationWorkload commWorkload(bytesPerSample, bytesPerTransaction, numFIFOs);

        workload[it->first] = commWorkload;
    }

    return workload;
}

void CommunicationEstimator::printComputeInstanceTable(
        std::map<std::pair<int, int>, EstimatorCommon::InterThreadCommunicationWorkload> commWorkload) {

    std::string partitionFromLabel  = "From Partition";
    std::string partitionToLabel = "To Partition";
    std::string bytesPerSampleLabel = "Bytes Per Sample";
    std::string bytesPerBlockLabel = "Bytes Per Block";
    std::string numFIFOsLabel = "# FIFOs";

    //Used for formatting
    size_t maxPartitionFromLen = partitionFromLabel.size();
    size_t maxPartitionToLen = partitionToLabel.size();
    size_t maxBytesPerSampleLen = bytesPerSampleLabel.size();
    size_t maxBytesPerBlockLen = bytesPerBlockLabel.size();
    size_t maxNumFIFOsLen = numFIFOsLabel.size();

    for(auto it = commWorkload.begin(); it != commWorkload.end(); it++){
        maxPartitionFromLen = std::max(maxPartitionFromLen, GeneralHelper::to_string(it->first.first).size());
        maxPartitionToLen = std::max(maxPartitionToLen, GeneralHelper::to_string(it->first.second).size());
        maxBytesPerSampleLen = std::max(maxBytesPerSampleLen, GeneralHelper::to_string(it->second.numBytesPerSample).size());
        maxBytesPerBlockLen = std::max(maxBytesPerBlockLen, GeneralHelper::to_string(it->second.numBytesPerBlock).size());
        maxNumFIFOsLen = std::max(maxNumFIFOsLen, GeneralHelper::to_string(it->second.numFIFOs).size());
    }

    //Print the table

    //Print the header
    std::string partitionFromFormatStr = "%" + GeneralHelper::to_string(maxPartitionFromLen) + "d";
    std::string partitionToFormatStr = " | %" + GeneralHelper::to_string(maxPartitionToLen) + "d";
    std::string bytesPerSampleFormatStr = " | %" + GeneralHelper::to_string(maxBytesPerSampleLen) + "d";
    std::string bytesPerBlockFormatStr = " | %" + GeneralHelper::to_string(maxBytesPerBlockLen) + "d";
    std::string numFIFOsFormatStr = " | %" + GeneralHelper::to_string(maxNumFIFOsLen) + "d";

    std::string partitionFromLabelFormatStr = "%" + GeneralHelper::to_string(maxPartitionFromLen) + "s";
    std::string partitionToLabelFormatStr = " | %" + GeneralHelper::to_string(maxPartitionToLen) + "s";
    std::string bytesPerSampleLabelFormatStr = " | %" + GeneralHelper::to_string(maxBytesPerSampleLen) + "s";
    std::string bytesPerBlockLabelFormatStr = " | %" + GeneralHelper::to_string(maxBytesPerBlockLen) + "s";
    std::string numFIFOsLabelFormatStr = " | %" + GeneralHelper::to_string(maxNumFIFOsLen) + "s";

    printf(partitionFromLabelFormatStr.c_str(), partitionFromLabel.c_str());
    printf(partitionToLabelFormatStr.c_str(), partitionToLabel.c_str());
    printf(bytesPerSampleLabelFormatStr.c_str(), bytesPerSampleLabel.c_str());
    printf(bytesPerBlockLabelFormatStr.c_str(), bytesPerBlockLabel.c_str());
    printf(numFIFOsLabelFormatStr.c_str(), numFIFOsLabel.c_str());

    std::cout << std::endl;

    //Print the rows (the operator)
    for(auto it = commWorkload.begin(); it != commWorkload.end(); it++){
        printf(partitionFromFormatStr.c_str(), it->first.first);
        printf(partitionToFormatStr.c_str(), it->first.second);
        printf(bytesPerSampleFormatStr.c_str(), it->second.numBytesPerSample);
        printf(bytesPerBlockFormatStr.c_str(), it->second.numBytesPerBlock);
        printf(numFIFOsFormatStr.c_str(), it->second.numFIFOs);

        std::cout << std::endl;
    }
}
