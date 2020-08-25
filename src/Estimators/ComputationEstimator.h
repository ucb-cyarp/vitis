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

namespace ComputationEstimator {
    /**
     * @brief Used to denote an estimator option
     */
    enum class EstimatorOption{
        ENABLED, ///<Enabled
        DISABLED, ///<Disabled
        NONSCALAR_ONLY ///<Enabled with Vectors and Matrices Only
    };

    //Note that the vector length of the different load and stores will be the number of elements in that port
    //Each element is NOT reported as a separate op. - that type of information will be computed elsewhere
    //There will be a seperate op reported per port
    EstimatorCommon::ComputeWorkload getIOLoadStoreWorkloadEst(std::vector<std::shared_ptr<InputPort>> &inputPorts, std::vector<std::shared_ptr<OutputPort>> &outputPorts);

    /**
     * @brief Get the Estimated I/O Load/Store workload for the given node
     * @param node
     * @param includeLoadStoreOps if ENABLED, includes the load/store ops.  If VECTOR, includes the load/store ops if an input or output is a vector/matrix (not a scalar)
     * @return
     */
    EstimatorCommon::ComputeWorkload getIOLoadStoreWorkloadEst(std::shared_ptr<Node> node, EstimatorOption includeLoadStoreOps);

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
    std::pair<std::map<EstimatorCommon::NodeOperation, int>, std::map<std::type_index, std::string>> reportComputeInstances(std::vector<std::shared_ptr<Node>> nodes,  std::vector<std::shared_ptr<Node>> excludeNodes = {});

    /**
     * @brief This reports the estimated compute workload for a given set of nodes
     * @param nodes
     *
     * @note: ContextFamilyContainers are included in the count. Each one leverages the estimate function of the context root
     * @note: EnableInputs are included because they can introduce additional loads/stores depending on the implementation
     *
     * @returns a workload object with estimates with the given estimator configuration
     */
    EstimatorCommon::ComputeWorkload reportComputeWorkload(std::vector<std::shared_ptr<Node>> nodes,
                                                           std::vector<std::shared_ptr<Node>> excludeNodes,
                                                           bool expandComplexOperators, bool expandHighLevelOperators,
                                                           ComputationEstimator::EstimatorOption includeIntermediateLoadStore,
                                                           ComputationEstimator::EstimatorOption includeInputOutputLoadStores);

    /**
     * @brief Breaks vector operations in a given workload into indevidual scalar operations.  The origional workload object is uneffected and a new workload object is returned
     * @param workload
     * @return
     */
    EstimatorCommon::ComputeWorkload breakVectorOps(EstimatorCommon::ComputeWorkload workload);

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
    //TODO: Implement
    std::pair<std::map<EstimatorCommon::NodeOperation, int>, std::map<std::type_index, std::string>>  reportComputeInstancesConservativeWorstCase(std::vector<std::shared_ptr<Node>> topLevelNodes, int partitionNum);

    void printComputeInstanceTable(std::map<int, std::map<EstimatorCommon::NodeOperation, int>> partitionOps, std::map<std::type_index, std::string> names);

    void printWorkloadTable(const std::map<int, EstimatorCommon::ComputeWorkload> &partitionOps, bool breakVectorOps);

    //TODO: Implement primitive op estimator versions of the above functions
    //TODO: Include casts in primitive op estimator

};

/*! @} */

#endif //VITIS_COMPUTATIONESTIMATOR_H
