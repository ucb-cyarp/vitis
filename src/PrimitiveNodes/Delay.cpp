//
// Created by Christopher Yarp on 7/6/18.
//

#include <vector>

#include "Delay.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/NumericValue.h"

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
