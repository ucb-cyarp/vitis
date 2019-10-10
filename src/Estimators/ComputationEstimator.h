//
// Created by Christopher Yarp on 10/9/19.
//

#ifndef VITIS_COMPUTATIONESTIMATOR_H
#define VITIS_COMPUTATIONESTIMATOR_H

#include "GraphCore/Node.h"
#include "EstimatorCommon.h"
#include <typeindex>
#include <string>

/**
 * \addtogroup Estimators Estimators
 * @brief A group of classes for conducting performance estimation of a given design.
 * @{
*/

class ComputationEstimator {
public:

    /**
     * @brief This reports the number of compute instances in a given set of nodes
     * @param nodes
     *
     * @note: ContextFamilyContainers, ContextContainers are included in the count. SubSystems, EnabeledSubsystems, ExpandedSubsystems are not.
     * @note: EnabledInput ports are not included in the count as they are purely virtual.  EnabledOutputs are included because of their latch like behavior
     *
     * @note: If any floating point input is discovered, the operator is considered to be a floating point operator.  See the primitive operator estimator to get an inclusion of the datatype conversion (if aproprate)
     * @note: If there are a variable number of bits across the inputs, the largest
     *
     *
     * @returns a map of NodeOperations to counts.  Also returns a map of type_indexs to class names
     */
    static std::pair<std::map<EstimatorCommon::NodeOperation, int>, std::map<std::type_index, std::string>> reportComputeInstances(std::vector<std::shared_ptr<Node>> nodes);

    /**
     * @brief This reports a conservative worse case of the number of compute instances in a design by only including counts from 1 of several mutually exclusive contexts.
     *
     * @warning: This is a conservative estimate because it relies on the mutual exclusivity of contexts within a single ContextFamulyContainer (such as Mux) and does not attempt to determine if contexts in different ContextFamilyContainers are mutually exclusive by some property of their drivers.
     *
     * @note: ContextFamilyContainers, ContextContainers are included in the count. SubSystems, EnabeledSubsystems, ExpandedSubsystems are not.
     * @note: EnabledInput ports are not included in the count as they are purely virtual.  EnabledOutputs are included because of their latch like behavior
     *
     * @param topLevelNodes
     * @param partitionNum
     */
    static std::pair<std::map<EstimatorCommon::NodeOperation, int>, std::map<std::type_index, std::string>>  reportComputeInstancesConservativeWorstCase(std::vector<std::shared_ptr<Node>> topLevelNodes, int partitionNum);

    static void printComputeInstanceTable(std::map<int, std::map<EstimatorCommon::NodeOperation, int>> partitionOps, std::map<std::type_index, std::string> names);

    //TODO: Implement primitive op estimator versions of the above functions
    //TODO: Include casts in primitive op estimator

};

/*! @} */

#endif //VITIS_COMPUTATIONESTIMATOR_H
