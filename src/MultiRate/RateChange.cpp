//
// Created by Christopher Yarp on 4/20/20.
//

#include "RateChange.h"
#include "General/ErrorHelpers.h"
#include "MultiRateHelpers.h"

RateChange::RateChange() : useVectorSamplingMode(false) {

}

RateChange::RateChange(std::shared_ptr<SubSystem> parent) : Node(parent), useVectorSamplingMode(false) {

}

RateChange::RateChange(std::shared_ptr<SubSystem> parent, RateChange *orig) : Node(parent, orig), useVectorSamplingMode(orig->useVectorSamplingMode) {

}

void RateChange::removeKnownReferences() {
    Node::removeKnownReferences();

    //Also, remove from ClockDomain
    std::shared_ptr<ClockDomain> clockDomain = MultiRateHelpers::findClockDomain(getSharedPointer());
    std::shared_ptr<RateChange> this_cast = std::static_pointer_cast<RateChange>(getSharedPointer());
    clockDomain->removeRateChange(this_cast);
}

void RateChange::validate(){
    Node::validate();

    if(inputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - RateChange - Should Have Exactly 1 Input Port", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - RateChange - Should Have 1 or More Output Ports", getSharedPointer()));
    }

    //Check that this node has exactly one input and 1 or more outputs
}

bool RateChange::canExpand() {
    return false;
}

bool RateChange::isSpecialized() {
    return false;
}

bool RateChange::isInput() {
    return false;
}

bool RateChange::isUsingVectorSamplingMode() const {
    return useVectorSamplingMode;
}

void RateChange::setUseVectorSamplingMode(bool useVectorSamplingMode) {
    RateChange::useVectorSamplingMode = useVectorSamplingMode;
}

void RateChange::populateParametersFromRateChangeNode(std::shared_ptr<RateChange> orig) {
    name = orig->getName();
    partitionNum = orig->getPartitionNum();
    schedOrder = orig->getSchedOrder();
    useVectorSamplingMode = orig->isUsingVectorSamplingMode();
}

void RateChange::populateRateChangeParametersFromGraphML(int id, std::string name,
                                                         std::map<std::string, std::string> dataKeyValueMap,
                                                         GraphMLDialect dialect) {
    setId(id);
    setName(name);

    if (dialect == GraphMLDialect::VITIS) {
        //This is a vitis/laminar only parameter
        useVectorSamplingMode = GeneralHelper::parseBool(dataKeyValueMap.at("UseVectorSamplingMode"));
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        useVectorSamplingMode = false;
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - RateChange", getSharedPointer()));
    }
}

std::set<GraphMLParameter> RateChange::graphMLParameters() {
    std::set<GraphMLParameter> graphMLParameters = Node::graphMLParameters();
    graphMLParameters.emplace("UseVectorSamplingMode", "bool", true);

    return Node::graphMLParameters();
}

void RateChange::emitGraphMLProperties(xercesc::DOMDocument *doc, xercesc::DOMElement *thisNode) {
    GraphMLHelper::addDataNode(doc, thisNode, "UseVectorSamplingMode", GeneralHelper::to_string(useVectorSamplingMode));
}

std::vector<Variable> RateChange::getVariablesToDeclareOutsideClockDomain() {
    return std::vector<Variable>();
}

void RateChange::specializeForBlocking(int localBlockingLength,
                                       int localSubBlockingLength,
                                       std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                       std::vector<std::shared_ptr<Node>> &nodesToRemove,
                                       std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                       std::vector<std::shared_ptr<Arc>> &arcsToRemove,
                                       std::vector<std::shared_ptr<Node>> &nodesToRemoveFromTopLevel,
                                       std::map<std::shared_ptr<Arc>, int> &arcsWithDeferredBlockingExpansion){
    if(useVectorSamplingMode){
        //Do nothing, the logic is handled internally in the implementations of RateChange
    }else{
        Node::specializeForBlocking(localBlockingLength,
                                    localSubBlockingLength,
                                    nodesToAdd,
                                    nodesToRemove,
                                    arcsToAdd,
                                    arcsToRemove,
                                    nodesToRemoveFromTopLevel,
                                    arcsWithDeferredBlockingExpansion);
    }
}

bool RateChange::specializesForBlocking() {
    return useVectorSamplingMode;
}
