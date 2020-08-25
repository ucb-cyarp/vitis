//
// Created by Christopher Yarp on 8/22/18.
//

#include "HighLevelNode.h"

HighLevelNode::HighLevelNode() {

}

HighLevelNode::HighLevelNode(std::shared_ptr<SubSystem> parent) : Node(parent) {

}

bool HighLevelNode::canExpand() {
    return true;
}

HighLevelNode::HighLevelNode(std::shared_ptr<SubSystem> parent, HighLevelNode* orig) : Node(parent, orig) {
    //Nothing new to copy here, just call the superclass constructor
}

EstimatorCommon::ComputeWorkload
HighLevelNode::getComputeWorkloadEstimate(bool expandComplexOperators, bool expandHighLevelOperators,
                                          ComputationEstimator::EstimatorOption includeIntermediateLoadStore,
                                          ComputationEstimator::EstimatorOption includeInputOutputLoadStores) {
    //By default, high level nodes do not report workload estimates, their expanded nodes do

    return EstimatorCommon::ComputeWorkload();
}
