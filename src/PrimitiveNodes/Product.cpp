//
// Created by Christopher Yarp on 7/6/18.
//

#include "Product.h"
#include "GraphCore/NodeFactory.h"
#include "GraphMLTools/GraphMLHelper.h"
#include "General/GeneralHelper.h"

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
    }else if(simulinkInputs[0] == '*' || simulinkInputs[0] == '/' || simulinkInputs[0] == '|'){
        //An array of *,/
        unsigned long inputLength = simulinkInputs.size();
        for(unsigned long i = 0; i<inputLength; i++){
            bool op;
            if(simulinkInputs[i] == '*'){
                ops.push_back(true);
            }else if(simulinkInputs[i] == '/'){
                ops.push_back(false);
            }else if(simulinkInputs[i] == '|'){
                //This is is a placeholder character that changes the position of the ports in the GUI but does not effect their numbering
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

std::set<GraphMLParameter> Product::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("InputOps", "string", true));

    return parameters;
}

xercesc::DOMElement *
Product::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Product");

    GraphMLHelper::addDataNode(doc, thisNode, "InputOps", GeneralHelper::vectorToString(inputOp, "*", "/", "", false));

    return thisNode;
}
