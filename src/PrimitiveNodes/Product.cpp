//
// Created by Christopher Yarp on 7/6/18.
//

#include "Product.h"
#include "GraphCore/NodeFactory.h"

Product::Product() {

}

Product::Product(std::shared_ptr<SubSystem> parent) : Node(parent) {

}

std::vector<bool> Product::getInputOp() const {
    return inputOp;
}

void Product::setInputOp(const std::vector<bool> &inputOp) {
    Product::inputOp = inputOp;
}

std::shared_ptr<Product> Product::createFromSimulinkGraphML(int id, std::map<std::string, std::string> dataKeyValueMap,
                                                            std::shared_ptr<SubSystem> parent) {
    std::shared_ptr<Product> newNode = NodeFactory::createNode<Product>(parent);
    newNode->setId(id);

    //==== Import important property -- Inputs ====
    std::string simulinkInputs = dataKeyValueMap.at("Inputs");

    //There are multiple cases for inputs.  One is a string of * or /.  The other is a number.
    std::vector<bool> ops;

    if(simulinkInputs.empty()){
        throw std::runtime_error("Empty Inputs parameter passed to Product");
    }else if(simulinkInputs[0] == '*' || simulinkInputs[0] == '/'){
        //An array of *,/
        unsigned long inputLength = simulinkInputs.size();
        for(unsigned long i = 0; i<inputLength; i++){
            bool op;
            if(simulinkInputs[i] == '*'){
                op = true;
            }else if(simulinkInputs[i] == '/'){
                op = false;
            }else{
                throw std::runtime_error("Unknown format for Product Input Parameter");
            }

            ops.push_back(op);
        }
    }else{
        //Parmater is a number
        int numInputs = std::stoi(simulinkInputs);

        for(int i = 0; i<numInputs; i++){
            ops.push_back(true);
        }
    }

    newNode->setInputOp(ops);

    return newNode;
}
