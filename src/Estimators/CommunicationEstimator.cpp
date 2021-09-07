//
// Created by Christopher Yarp on 10/9/19.
//

#include "CommunicationEstimator.h"

#include "General/GeneralHelper.h"
#include "General/ErrorHelpers.h"

#include "PartitionNode.h"
#include "PartitionCrossing.h"
#include "Emitter/MultiThreadEmit.h"
#include "Scheduling/IntraPartitionScheduling.h"

#include <iostream>

int CommunicationEstimator::getCommunicationBitsForType(DataType dt){
    //Communication occurs with valid CPU types
    //TODO: change if bit fields are used or bools are packed
    DataType cpuType = dt.getCPUStorageType();

    int bits = cpuType.getTotalBits();
    //TODO: change if bool type is updated
    if(bits == 1){
        bits = 8;
    }

    if(dt.isComplex()){
        bits *=2; //If complex, double the amount of data transferred (to include the imag component)
    }

    //Account for vector/matrix types
    bits *= dt.numberOfElements();

    return bits;
}

std::map<std::pair<int, int>, EstimatorCommon::InterThreadCommunicationWorkload>
CommunicationEstimator::reportCommunicationWorkload(
        std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifoMap) {
    std::map<std::pair<int, int>, EstimatorCommon::InterThreadCommunicationWorkload> workload;

    for(auto it = fifoMap.begin(); it != fifoMap.end(); it++){
        int bytesPerSample = 0;
        int bytesPerTransaction = 0;
        int numFIFOs = it->second.size();

        for(int i = 0; i<it->second.size(); i++){
            std::vector<std::shared_ptr<InputPort>> ports = it->second[i]->getInputPorts();

            for(int j = 0; j<ports.size(); j++){
                DataType portDT = ports[j]->getDataType();
                int bits = getCommunicationBitsForType(portDT);

                int bytes = bits/8;
                bytesPerSample += bytes;
                bytesPerTransaction += bytes*it->second[i]->getBlockSizeCreateIfNot(j);
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

std::shared_ptr<PartitionNode> CommunicationEstimator::createPartitionNode(int partition){
    std::shared_ptr<PartitionNode> node = NodeFactory::createNode<PartitionNode>();
    node->setPartitionNum(partition);
    node->setName("Partition" + GeneralHelper::to_string(partition));
    return node;
}

Design CommunicationEstimator::createCommunicationGraph(Design &operatorGraph, bool summary, bool removeCrossingsWithInitCond){
    Design communicationGraph;

    //Find ThreadCrossingFIFOs
    std::vector<std::shared_ptr<ThreadCrossingFIFO>> partitionCrossingFIFOs;

    std::vector<std::shared_ptr<Node>> nodes = operatorGraph.getNodes();
    for(const std::shared_ptr<Node> &node : nodes){
        std::shared_ptr<ThreadCrossingFIFO> asThreadCrossingFIFO = GeneralHelper::isType<Node, ThreadCrossingFIFO>(node);
        if(asThreadCrossingFIFO){
            partitionCrossingFIFOs.push_back(asThreadCrossingFIFO);
        }
    }

    //Create Communication Graph
    std::map<int, std::shared_ptr<PartitionNode>> partitionNodeMap;
    std::map<std::pair<int, int>, int> minInitialState; //The pair is the tuple (partitionFrom, partitionTo)
    std::vector<std::pair<std::pair<int, int>, int>> initialStates; //The pair is the tuple (partitionFrom, partitionTo)
    std::map<std::pair<int, int>, int> totalBytesPerSample;
    std::vector<std::pair<std::pair<int, int>, int>> bytesPerSample; //The pair is the tuple (partitionFrom, partitionTo)
    std::map<std::pair<int, int>, int> totalBytesPerBlock;
    std::vector<std::pair<std::pair<int, int>, int>> bytesPerBlock; //The pair is the tuple (partitionFrom, partitionTo)
    std::map<std::pair<int, int>, double> totalBytesPerBaseRateSample;
    std::vector<std::pair<std::pair<int, int>, double>> bytesPerBaseRateSample; //The pair is the tuple (partitionFrom, partitionTo)

    for(const std::shared_ptr<ThreadCrossingFIFO> &fifo : partitionCrossingFIFOs){
        int srcPartition = fifo->getPartitionNum(); //The FIFO is in the src domain
        int dstPartition = fifo->getOutputPartition();

        //Assumes contiguous port numbers
        if(fifo->getInputPorts().size() != fifo->getOutputPorts().size()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("ThreadCrossingFIFO had an unequal number of Input and Output ports", fifo));
        }

        if(fifo->getInputPorts().empty()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("ThreadCrossingFIFO has no input ports", fifo));
        }

        int minInitialStateBlocks = 0;
        int fifoTotalBytesPerSample = 0;
        double fifoTotalBytesPerBaseRateSample = 0;
        int fifoTotalBytesPerBlock = 0;

        for(int i = 0; i<fifo->getInputPorts().size(); i++){
            int portTotalBytesPerSample = getCommunicationBitsForType(fifo->getOutputPort(i)->getDataType())/8;
            fifoTotalBytesPerSample += portTotalBytesPerSample;
            std::pair<int, int> portRate = std::pair<int, int>(1, 1);
            std::shared_ptr<ClockDomain> portClockDomain = fifo->getClockDomainCreateIfNot(i);
            if(portClockDomain){
                portRate = portClockDomain->getRateRelativeToBase();
            }
            fifoTotalBytesPerBaseRateSample += ((double) portTotalBytesPerSample*portRate.first)/portRate.second;
            fifoTotalBytesPerBlock += portTotalBytesPerSample*fifo->getBlockSizeCreateIfNot(i);

            //Note, the initial conditions are stored as an array of scalar numbers, need to know the number of elements at the port per sample (ie. the number of elements in an input/output vector to the port or 1 if scalar)
            //To get the number of cycles of initial condition in the FIFO, take the length of the scalar array and divide by the length of the
            int initialConditionElements = fifo->getInitConditions().empty() ? 0 : (fifo->getInitConditions()[i].size()/fifo->getOutputPort(i)->getDataType().numberOfElements());
            int initialStateBlocks = initialConditionElements/fifo->getBlockSizeCreateIfNot(i);
            if(i==0){
                minInitialStateBlocks = initialStateBlocks;
            }else{
                minInitialStateBlocks = std::min(minInitialStateBlocks, initialStateBlocks);
            }

            //TODO: Remove check
            std::shared_ptr<Node> srcNode = fifo->getInputPort(i)->getSrcOutputPort()->getParent();
            int inputPortSrcPartition = srcNode->getPartitionNum();
            if(srcPartition != inputPortSrcPartition){
                throw std::runtime_error(ErrorHelpers::genErrorStr("ThreadCrossingFIFO input partition disagrees with detected partition.  Input: " + srcNode->getFullyQualifiedName(), fifo));
            }
        }

        if(summary){
            if(minInitialState.find(std::pair<int, int>(srcPartition, dstPartition)) == minInitialState.end()){
                //If the entry in the map (key = src->dst partition) does not exist yet, simply create it and set the value
                minInitialState[std::pair<int, int>(srcPartition, dstPartition)] = minInitialStateBlocks;
                totalBytesPerSample[std::pair<int, int>(srcPartition, dstPartition)] = fifoTotalBytesPerSample;
                totalBytesPerBlock[std::pair<int, int>(srcPartition, dstPartition)] = fifoTotalBytesPerBlock;
                totalBytesPerBaseRateSample[std::pair<int, int>(srcPartition, dstPartition)] = fifoTotalBytesPerBaseRateSample;
            }else{
                //Take the min initial state of all the FIFOs going from src->dst partition, sum up the other metrics
                minInitialState[std::pair<int, int>(srcPartition, dstPartition)] = std::min(minInitialState[std::pair<int, int>(srcPartition, dstPartition)], minInitialStateBlocks);
                totalBytesPerSample[std::pair<int, int>(srcPartition, dstPartition)] += fifoTotalBytesPerSample;
                totalBytesPerBlock[std::pair<int, int>(srcPartition, dstPartition)] += fifoTotalBytesPerBlock;
                totalBytesPerBaseRateSample[std::pair<int, int>(srcPartition, dstPartition)] += fifoTotalBytesPerBaseRateSample;
            }
        }else{
            initialStates.push_back(std::pair<std::pair<int, int>, int>(std::pair<int, int>(srcPartition, dstPartition), minInitialStateBlocks));
            bytesPerSample.push_back(std::pair<std::pair<int, int>, int>(std::pair<int, int>(srcPartition, dstPartition), fifoTotalBytesPerSample));
            bytesPerBlock.push_back(std::pair<std::pair<int, int>, int>(std::pair<int, int>(srcPartition, dstPartition), fifoTotalBytesPerBlock));
            bytesPerBaseRateSample.push_back(std::pair<std::pair<int, int>, double>(std::pair<int, int>(srcPartition, dstPartition), fifoTotalBytesPerBaseRateSample));
        }
    }

    std::vector<std::shared_ptr<Node>> nodesToAdd, emptyNodes;
    std::vector<std::shared_ptr<Arc>> arcsToAdd, emptyArcs;

    //TODO: Refactor
    if(summary){
        for(const auto & crossing : minInitialState){
            std::pair<int, int> partitions = crossing.first;
            int initialState = crossing.second;
            int bytesPerSampleLoc = totalBytesPerSample[partitions];
            int bytesPerBlockLoc = totalBytesPerBlock[partitions];
            double bytesPerBaseRateSampleLoc = totalBytesPerBaseRateSample[partitions];

            communicationGraphCreationHelper(partitionNodeMap, nodesToAdd, arcsToAdd, partitions.first,
                                             partitions.second, initialState, bytesPerSampleLoc, bytesPerBlockLoc,
                                             bytesPerBaseRateSampleLoc, removeCrossingsWithInitCond);
        }
    }else{
        for(int i = 0; i < initialStates.size(); i++){
            std::pair<int, int> partitions = initialStates[i].first;
            int initialState = initialStates[i].second;
            int bytesPerSampleLoc = bytesPerSample[i].second;
            int bytesPerBlockLoc = bytesPerBlock[i].second;
            double bytesPerBaseRateSampleLoc = bytesPerBaseRateSample[i].second;

            communicationGraphCreationHelper(partitionNodeMap, nodesToAdd, arcsToAdd, partitions.first,
                                             partitions.second, initialState, bytesPerSampleLoc, bytesPerBlockLoc,
                                             bytesPerBaseRateSampleLoc, removeCrossingsWithInitCond);
        }
    }

    communicationGraph.addRemoveNodesAndArcs(nodesToAdd, emptyNodes, arcsToAdd, emptyArcs);

    communicationGraph.assignNodeIDs();
    communicationGraph.assignArcIDs();

    return communicationGraph;
}

void CommunicationEstimator::communicationGraphCreationHelper(std::map<int, std::shared_ptr<PartitionNode>> &partitionNodeMap,
                                                              std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                                              std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                                              int srcPartition, int dstPartition,
                                                              int initialState, int bytesPerSample, int bytesPerBlock,
                                                              double bytesPerBaseRateSample,
                                                              bool removeCrossingsWithInitCond) {
    if(!removeCrossingsWithInitCond || initialState == 0) {
        std::shared_ptr<PartitionNode> srcPartNode;
        if (partitionNodeMap.find(srcPartition) == partitionNodeMap.end()) {
            srcPartNode = createPartitionNode(srcPartition);
            partitionNodeMap[srcPartition] = srcPartNode;
            nodesToAdd.push_back(srcPartNode);
        } else {
            srcPartNode = partitionNodeMap[srcPartition];
        }

        std::shared_ptr<PartitionNode> dstPartNode;
        if (partitionNodeMap.find(dstPartition) == partitionNodeMap.end()) {
            dstPartNode = createPartitionNode(dstPartition);
            partitionNodeMap[dstPartition] = dstPartNode;
            nodesToAdd.push_back(dstPartNode);
        } else {
            dstPartNode = partitionNodeMap[dstPartition];
        }

        std::shared_ptr<PartitionCrossing> crossingArc = PartitionCrossing::connectNodes(
                srcPartNode->getOrderConstraintOutputPortCreateIfNot(),
                dstPartNode->getOrderConstraintInputPortCreateIfNot(), DataType());
        crossingArc->setInitStateCountBlocks(initialState);
        crossingArc->setBytesPerSample(bytesPerSample);
        crossingArc->setBytesPerBlock(bytesPerBlock);
        crossingArc->setBytesPerBaseRateSample(bytesPerBaseRateSample);
        arcsToAdd.push_back(crossingArc);
    }
}

void CommunicationEstimator::checkForDeadlock(Design &operatorGraph, std::string designName, std::string path){
    Design communicationGraph = createCommunicationGraph(operatorGraph, true, true);

    //Need to disconnect arcs to/from I/O partition since
    std::vector<std::shared_ptr<Node>> nodes = communicationGraph.getNodes();
    for(const std::shared_ptr<Node> &node : nodes){
        if(node->getPartitionNum() == IO_PARTITION_NUM){
            std::set<std::shared_ptr<Arc>> arcsToRemove = node->disconnectNode();
            std::vector<std::shared_ptr<Arc>> arcsToRemoveVec;
            arcsToRemoveVec.insert(arcsToRemoveVec.end(), arcsToRemove.begin(), arcsToRemove.end());
            std::vector<std::shared_ptr<Arc>> emptyArcs;
            std::vector<std::shared_ptr<Node>> emptyNodes;
            communicationGraph.addRemoveNodesAndArcs(emptyNodes, emptyNodes, emptyArcs, arcsToRemoveVec);
        }
    }

    std::exception err;
    try {
        IntraPartitionScheduling::scheduleTopologicalStort(communicationGraph, TopologicalSortParameters(), false, false,
                                                           designName+"partitionLoop", path, false,false);
    }catch(const std::exception &e){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Detected Deadlock Condition Between Partitions\n" + std::string(e.what())));
    }

    try {
        communicationGraph.verifyTopologicalOrder(false, SchedParams::SchedType::TOPOLOGICAL_CONTEXT);
    }catch(const std::exception &e){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Detected Deadlock Condition Between Partitions\n" + std::string(e.what())));
    }

    //TODO: Run through the list of partition nodes to check if they were scheduled
    //Validation does not error if there is an arc between 2 unscheduled nodes
    std::set<int> unscheduledPartitions;
    for(const std::shared_ptr<Node> &node : nodes) {
        if (node->getPartitionNum() != IO_PARTITION_NUM && node->getSchedOrder() == -1) {
            unscheduledPartitions.insert(node->getPartitionNum());
        }
    }

    if(!unscheduledPartitions.empty()){
        std::string unscheduledPartitionStr;
        bool first = true;
        for(int partition: unscheduledPartitions){
            if(first){
                first = false;
            }else{
                unscheduledPartitionStr += ", ";
            }

            unscheduledPartitionStr += GeneralHelper::to_string(partition);
        }

        throw std::runtime_error(ErrorHelpers::genErrorStr("Detected Deadlock Condition Involving the Following Partitions: " + unscheduledPartitionStr));
    }
}