//
// Created by Christopher Yarp on 7/6/18.
//

#include <vector>

#include "Delay.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/NumericValue.h"

#include "General/GeneralHelper.h"

Delay::Delay() : delayValue(0){

}

Delay::Delay(std::shared_ptr<SubSystem> parent) : Node(parent), delayValue(0) {

}

int Delay::getDelayValue() const {
    return delayValue;
}

void Delay::setDelayValue(int delayValue) {
    Delay::delayValue = delayValue;
}

std::vector<NumericValue> Delay::getInitCondition() const {
    return initCondition;
}

void Delay::setInitCondition(const std::vector<NumericValue> &initCondition) {
    Delay::initCondition = initCondition;
}

std::shared_ptr<Delay> Delay::createFromSimulinkGraphML(int id, std::map<std::string, std::string> dataKeyValueMap,
                                                        std::shared_ptr<SubSystem> parent) {
    std::shared_ptr<Delay> newNode = NodeFactory::createNode<Delay>(parent);
    newNode->setId(id);

    //==== Check Supported Config ====
    std::string delayLengthSource = dataKeyValueMap.at("DelayLengthSource");

    if(delayLengthSource != "Dialog"){
        throw std::runtime_error("Delay block must specify Delay Source as \"Dialog\"");
    }

    std::string initialConditionSource = dataKeyValueMap.at("InitialConditionSource");
    if(initialConditionSource != "Dialog"){
        throw std::runtime_error("Delay block must specify Initial Condition Source source as \"Dialog\"");
    }

    //==== Import important property -- DelayLength ====
    std::string delayStr = dataKeyValueMap.at("Numeric.DelayLength");

    int delayVal = std::stoi(delayStr);
    newNode->setDelayValue(delayVal);

    //==== Import important property -- InitialCondition ====
    std::string initialConditionStr = dataKeyValueMap.at("Numeric.InitialCondition");

    std::vector<NumericValue> initialConds = NumericValue::parseXMLString(initialConditionStr);
    newNode->setInitCondition(initialConds);

    return newNode;
}

std::set<GraphMLParameter> Delay::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("DelayLength", "string", true));
    parameters.insert(GraphMLParameter("InitialCondition", "string", true));

    return parameters;
}

xercesc::DOMElement *
Delay::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Delay");

    GraphMLHelper::addDataNode(doc, thisNode, "DelayLength", std::to_string(delayValue));

    GraphMLHelper::addDataNode(doc, thisNode, "InitialCondition", NumericValue::toString(initCondition));

    return thisNode;
}
