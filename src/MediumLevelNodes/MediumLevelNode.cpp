//
// Created by Christopher Yarp on 7/17/18.
//

#include "MediumLevelNode.h"

MediumLevelNode::MediumLevelNode() {

}

MediumLevelNode::MediumLevelNode(std::shared_ptr<SubSystem> parent) : Node(parent) {

}

bool MediumLevelNode::canExpand() {
    return true;
}

MediumLevelNode::MediumLevelNode(std::shared_ptr<SubSystem> parent, MediumLevelNode* orig) : Node(parent, orig) {
    //Nothing new to copy here, just call superclass constructor
}

EstimatorCommon::ComputeWorkload
MediumLevelNode::getComputeWorkloadEstimate(bool expandComplexOperators, bool expandHighLevelOperators,
                                            ComputationEstimator::EstimatorOption includeIntermediateLoadStore,
                                            ComputationEstimator::EstimatorOption includeInputOutputLoadStores) {
    //By default, medium level nodes do not report compute workload.  Their expanded primitive nodes do.

    return EstimatorCommon::ComputeWorkload();
}
