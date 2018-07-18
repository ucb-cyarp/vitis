//
// Created by Christopher Yarp on 7/6/18.
//

#include "Sum.h"
#include "GraphCore/NodeFactory.h"
#include "General/GeneralHelper.h"

Sum::Sum() {

}

Sum::Sum(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent) {

}

std::vector<bool> Sum::getInputSign() const {
    return inputSign;
}

void Sum::setInputSign(const std::vector<bool> &inputSign) {
    Sum::inputSign = inputSign;
}

std::shared_ptr<Sum> Sum::createFromGraphML(int id, std::string name,
                                            std::map<std::string, std::string> dataKeyValueMap,
                                            std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<Sum> newNode = NodeFactory::createNode<Sum>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important property ====
    std::string inputSigns;

    if(dialect == GraphMLDialect::VITIS){
        //Vitis Name -- InputSigns
        inputSigns = dataKeyValueMap.at("InputSigns");
    } else if(dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Name -- Inputs
        inputSigns = dataKeyValueMap.at("Inputs");
    } else
    {
        throw std::runtime_error("Unsupported Dialect when parsing XML - Product");
    }

    //There are multiple cases for inputs.  One is a string of + or - signs.  The other is a number.
    std::vector<bool> signs;

    if(inputSigns.empty()){
        throw std::runtime_error("Empty Inputs parameter passed to Sum");
    }else if(inputSigns[0] == '+' || inputSigns[0] == '-' || inputSigns[0] == '|'){
        //An array of +,-,|
        unsigned long inputLength = inputSigns.size();
        for(unsigned long i = 0; i<inputLength; i++){
            if(inputSigns[i] == '+'){
                signs.push_back(true);
            }else if(inputSigns[i] == '-'){
                signs.push_back(false);
            }else if(inputSigns[i] == '|'){
                //This is is a placeholder character that changes the position of the ports in the GUI but does not effect their numbering
            }else{
                throw std::runtime_error("Unknown format for Sum Input Parameter");
            }
        }
    }else{
        //Parmater is a number
        int numInputs = std::stoi(inputSigns);

        for(int i = 0; i<numInputs; i++){
            signs.push_back(true);
        }
    }

    newNode->setInputSign(signs);

    return newNode;
}

std::set<GraphMLParameter> Sum::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("InputSigns", "string", true));

    return parameters;
}

xercesc::DOMElement *
Sum::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Sum");

    GraphMLHelper::addDataNode(doc, thisNode, "InputSigns", GeneralHelper::vectorToString(inputSign, "+", "-", "", false));

    return thisNode;
}

std::string Sum::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: Sum\nInputOps:" + GeneralHelper::vectorToString(inputSign, "+", "-", "", false);

    return label;
}

void Sum::validate() {
    Node::validate();

    if(inputPorts.size() < 2){
        throw std::runtime_error("Validation Failed - Sum - Should Have 2 or More Input Ports");
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - Sum - Should Have Exactly 1 Output Port");
    }
}
