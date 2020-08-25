//
// Created by Christopher Yarp on 7/17/18.
//

#include "PrimitiveNode.h"

PrimitiveNode::PrimitiveNode() {

}

PrimitiveNode::PrimitiveNode(std::shared_ptr<SubSystem> parent) : Node(parent) {

}

bool PrimitiveNode::canExpand() {
    return false;
}

PrimitiveNode::PrimitiveNode(std::shared_ptr<SubSystem> parent, Node* orig) : Node(parent, orig) {
    //Nothing new to copy
}

EstimatorCommon::ComputeWorkload
PrimitiveNode::getComputeWorkloadEstimate(bool expandComplexOperators, bool expandHighLevelOperators,
                                          ComputationEstimator::EstimatorOption includeIntermediateLoadStore,
                                          ComputationEstimator::EstimatorOption includeInputOutputLoadStores) {

    //TODO: Remove once implemented for all primitive nodes

    //+++ Add I/O Load/Store if appripriate +++
    EstimatorCommon::ComputeWorkload workload = getIOLoadStoreWorkloadEst(getSharedPointer(),
                                                                          includeInputOutputLoadStores);

    //EST_UNIMPLEMENTED does not report type and length information
    workload.addOperation(EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::EST_UNIMPLEMENTED, false, false, 0, 1));

    return workload;
}
