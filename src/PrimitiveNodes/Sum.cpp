//
// Created by Christopher Yarp on 7/6/18.
//

#include "Sum.h"
#include "GraphCore/NodeFactory.h"

Sum::Sum() {

}

Sum::Sum(std::shared_ptr<SubSystem> parent) : Node(parent) {

}

std::vector<bool> Sum::getInputSign() const {
    return inputSign;
}

void Sum::setInputSign(const std::vector<bool> &inputSign) {
    Sum::inputSign = inputSign;
}

std::shared_ptr<Sum> Sum::createFromSimulinkGraphML(int id, std::map<std::string, std::string> dataKeyValueMap, std::shared_ptr<SubSystem> parent) {
    std::shared_ptr<Sum> newNode = NodeFactory::createNode<Sum>(parent);
    newNode->setId(id);

    //==== Import important property -- Inputs ====
    std::string simulinkInputs = dataKeyValueMap.at("Inputs");

    //There are multiple cases for inputs.  One is a string of + or - signs.  The other is a number.
    std::vector<bool> signs;

    if(simulinkInputs.empty()){
        throw std::runtime_error("Empty Inputs parameter passed to Sum");
    }else if(simulinkInputs[0] == '+' || simulinkInputs[0] == '-'){
        //An array of +,-
        unsigned long inputLength = simulinkInputs.size();
        for(unsigned long i = 0; i<inputLength; i++){
            bool sign;
            if(simulinkInputs[i] == '+'){
                sign = true;
            }else if(simulinkInputs[i] == '-'){
                sign = false;
            }else{
                throw std::runtime_error("Unknown format for Sum Input Parameter");
            }

            signs.push_back(sign);
        }
    }else{
        //Parmater is a number
        int numInputs = std::stoi(simulinkInputs);

        for(int i = 0; i<numInputs; i++){
            signs.push_back(true);
        }
    }

    newNode->setInputSign(signs);

    return newNode;
}