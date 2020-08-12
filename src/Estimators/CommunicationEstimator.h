//
// Created by Christopher Yarp on 10/9/19.
//

#ifndef VITIS_COMMUNICATIONESTIMATOR_H
#define VITIS_COMMUNICATIONESTIMATOR_H

#include <map>
#include <utility>
#include "EstimatorCommon.h"
#include "MultiThread/ThreadCrossingFIFO.h"

/**
 * \addtogroup Estimators Estimators
 * @{
*/

namespace CommunicationEstimator {
    std::map<std::pair<int, int>, EstimatorCommon::InterThreadCommunicationWorkload> reportCommunicationWorkload(std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifoMap);

    void printComputeInstanceTable(std::map<std::pair<int, int>, EstimatorCommon::InterThreadCommunicationWorkload> commWorkload);
};

/*! @} */

#endif //VITIS_COMMUNICATIONESTIMATOR_H
