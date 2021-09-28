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
     * @brief Get the total number of bytes required to transfer a value of a given datatype.
     * Doubles the size if the type is complex.  Returns 1 byte for a boolean.  Also converts to a CPU storage type.
     * Also accounts for vector/matrix types.
     * @param dt
     * @return
     */
    int getCommunicationBitsForType(DataType dt);

    /**
     * @brief Used when creating the communication graph from a list of partition crossings
     */
    void communicationGraphCreationHelper(std::map<int, std::shared_ptr<PartitionNode>> &partitionNodeMap,
                                          std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                          std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                          int srcPartition, int dstPartition,
                                          int initialState, int bytesPerSample, int bytesPerBlock,
                                          double bytesPerBaseRateSample,
                                          bool removeCrossingsWithInitCond);
};

/*! @} */

#endif //VITIS_COMMUNICATIONESTIMATOR_H
