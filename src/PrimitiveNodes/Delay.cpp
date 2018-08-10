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

Delay::Delay(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), delayValue(0) {

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

std::shared_ptr<Delay> Delay::createFromGraphML(int id, std::string name,
                                                std::map<std::string, std::string> dataKeyValueMap,
                                                std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<Delay> newNode = NodeFactory::createNode<Delay>(parent);
    newNode->setId(id);
    newNode->setName(name);

    if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //==== Check Supported Config (Only if Simulink Import)====
        std::string delayLengthSource = dataKeyValueMap.at("DelayLengthSource");

        if (delayLengthSource != "Dialog") {
            throw std::runtime_error("Delay block must specify Delay Source as \"Dialog\"");
        }

        std::string initialConditionSource = dataKeyValueMap.at("InitialConditionSource");
        if (initialConditionSource != "Dialog") {
            throw std::runtime_error("Delay block must specify Initial Condition Source source as \"Dialog\"");
        }
    }

    //==== Import important properties ====
    std::string delayStr;
    std::string initialConditionStr;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- DelayLength, InitialCondit
        delayStr = dataKeyValueMap.at("DelayLength");
        initialConditionStr = dataKeyValueMap.at("InitialCondition");
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Names -- Numeric.DelayLength, Numeric.InitialCondition
        delayStr = dataKeyValueMap.at("Numeric.DelayLength");
        initialConditionStr = dataKeyValueMap.at("Numeric.InitialCondition");
    } else
    {
        throw std::runtime_error("Unsupported Dialect when parsing XML - Delay");
    }

    int delayVal = std::stoi(delayStr);
    newNode->setDelayValue(delayVal);

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

std::string Delay::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: Delay\nDelayLength:" + std::to_string(delayValue) + "\nInitialCondition: " + NumericValue::toString(initCondition);

    return label;
}

void Delay::validate() {
    Node::validate();

    //Should have 1 input ports and 1 output port
    if(inputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - Delay - Should Have Exactly 1 Input Port");
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - Delay - Should Have Exactly 1 Output Port");
    }

    //Check that input port and the output port have the same type
    DataType outType = getOutputPort(0)->getDataType();
    DataType inType = getInputPort(0)->getDataType();

    if(inType != outType){
        throw std::runtime_error("Validation Failed - Delay - DataType of Input Port Does not Match Output Port");
    }
}
