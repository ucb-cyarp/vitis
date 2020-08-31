//
// Created by Christopher Yarp on 10/9/19.
//

#ifndef VITIS_COMMUNICATIONESTIMATOR_H
#define VITIS_COMMUNICATIONESTIMATOR_H

#include <map>
#include <utility>
#include "EstimatorCommon.h"
#include "MultiThread/ThreadCrossingFIFO.h"
#include "GraphCore/Design.h"
#include "PartitionNode.h"

/**
 * \addtogroup Estimators Estimators
 * @{
*/

namespace CommunicationEstimator {
    std::map<std::pair<int, int>, EstimatorCommon::InterThreadCommunicationWorkload> reportCommunicationWorkload(std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifoMap);

    void printComputeInstanceTable(std::map<std::pair<int, int>, EstimatorCommon::InterThreadCommunicationWorkload> commWorkload);

    std::shared_ptr<PartitionNode> createPartitionNode(int partition);

    /**
     * @brief Creates a communication graph with partitions represented as single nodes and communication between
     * partitions represented by arcs.
     *
     * This graph can be exported to GraphML for visualization and used to check for deadlock.
     *
     * @param operatorGraph the partitioned design composed of synthesizable operators and ThreadCrossingFIFOs
     * @return
     */
    Design createCommunicationGraph(Design &operatorGraph, bool summary, bool removeCrossingsWithInitCond);

    //In the process, creates a new communication graph
    void checkForDeadlock(Design &operatorGraph, std::string designName, std::string path);

    /**
     * @brief Used when creating the communication graph from a list of partition crossings
     * @param partitionNodeMap
     * @param nodesToAdd
     * @param arcsToAdd
     * @param crossing
     */
    void communicationGraphCreationHelper(std::map<int, std::shared_ptr<PartitionNode>> &partitionNodeMap,
                                          std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                          std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                          const std::pair<const std::pair<int, int>, int> &crossing,
                                          bool removeCrossingsWithInitCond);
};

/*! @} */

#endif //VITIS_COMMUNICATIONESTIMATOR_H
