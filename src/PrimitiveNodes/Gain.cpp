//
// Created by Christopher Yarp on 7/16/18.
//

#include "Gain.h"

Gain::Gain() {

}

Gain::Gain(std::shared_ptr<SubSystem> parent) : Node(parent) {

}

std::vector<NumericValue> Gain::getGain() const {
    return gain;
}

void Gain::setGain(const std::vector<NumericValue> &gain) {
    Gain::gain = gain;
}

std::shared_ptr<Gain>
Gain::createFromSimulinkGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node & Set Common Properties ====
    std::shared_ptr<Gain> newNode = NodeFactory::createNode<Gain>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    std::string gainStr;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- Gain
        gainStr = dataKeyValueMap.at("Gain");
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Names -- Numeric.Gain
        gainStr = dataKeyValueMap.at("Numeric.Gain");
    } else
    {
        throw std::runtime_error("Unsupported Dialect when parsing XML - Gain");
    }

    std::vector<NumericValue> gainValue = NumericValue::parseXMLString(gainStr);

    newNode->setGain(gainValue);

    return newNode;
}

std::set<GraphMLParameter> Gain::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("Gain", "string", true));

    return parameters;
}

xercesc::DOMElement *
Gain::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Gain");

    GraphMLHelper::addDataNode(doc, thisNode, "Gain", NumericValue::toString(gain));

    return thisNode;
}

std::string Gain::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: Gain\nGain:" + NumericValue::toString(gain);

    return label;
}

