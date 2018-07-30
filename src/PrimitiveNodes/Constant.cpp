//
// Created by Christopher Yarp on 7/16/18.
//

#include "Constant.h"

Constant::Constant() {

}

Constant::Constant(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent){

}

std::vector<NumericValue> Constant::getValue() const {
    return value;
}

void Constant::setValue(const std::vector<NumericValue> &values) {
    Constant::value = values;
}

std::shared_ptr<Constant>
Constant::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                            std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {

    //==== Create Node and set common properties ====
    std::shared_ptr<Constant> newNode = NodeFactory::createNode<Constant>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    std::string valueStr;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- Value
        valueStr = dataKeyValueMap.at("Value");
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Names -- Numeric.Value
        valueStr = dataKeyValueMap.at("Numeric.Value");
    } else
    {
        throw std::runtime_error("Unsupported Dialect when parsing XML - Constant");
    }

    std::vector<NumericValue> values = NumericValue::parseXMLString(valueStr);
    newNode->setValue(values);

    return newNode;
}

std::set<GraphMLParameter> Constant::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("Value", "string", true));

    return  parameters;
}

xercesc::DOMElement *
Constant::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Constant");

    GraphMLHelper::addDataNode(doc, thisNode, "Value", NumericValue::toString(value));

    return thisNode;
}

std::string Constant::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: Constant\nValue:" + NumericValue::toString(value);

    return label;
}

void Constant::validate() {
    Node::validate();

    //Should have 0 input ports and 1 output port
    if(inputPorts.size() != 0){
        throw std::runtime_error("Validation Failed - Constant - Should Have Exactly 0 Input Ports");
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - Constant - Should Have Exactly 1 Output Port");
    }

    //Check there is at least 1 constant value
    if(value.size() < 1){
        throw std::runtime_error("Validation Failed - Constant - Should Have at Least 1 Value");
    }
}
