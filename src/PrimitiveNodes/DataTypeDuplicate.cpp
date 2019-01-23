//
// Created by Christopher Yarp on 9/11/18.
//

#include "DataTypeDuplicate.h"
#include "GraphCore/NodeFactory.h"

DataTypeDuplicate::DataTypeDuplicate() {

}

DataTypeDuplicate::DataTypeDuplicate(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent) {

}

std::shared_ptr<DataTypeDuplicate>
DataTypeDuplicate::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                     std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<DataTypeDuplicate> newNode = NodeFactory::createNode<DataTypeDuplicate>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //There are no properties to import

    return newNode;
}

std::set<GraphMLParameter> DataTypeDuplicate::graphMLParameters() {
    //No Parameters
    return Node::graphMLParameters();
}

xercesc::DOMElement *DataTypeDuplicate::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode,
                                                    bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "DataTypeDuplicate");

    return thisNode;
}

std::string DataTypeDuplicate::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: DataTypeDuplicate";

    return label;
}

void DataTypeDuplicate::validate() {
    Node::validate();

    if(inputPorts.size() >= 1){
        //Actually validate types since there are inputs
        DataType port0Type = getInputPort(0)->getDataType();

        unsigned long numInputPorts = inputPorts.size();

        for(unsigned long i = 1; i<numInputPorts; i++){
            DataType portIType = getInputPort(i)->getDataType();

            if(port0Type != portIType){
                throw std::runtime_error("Validation Failed - DataTypeDuplicate - Input Ports Do not Have Same Type");
            }
        }
    }


}

CExpr DataTypeDuplicate::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    throw std::runtime_error("Emit Failed - DataTypeDuplicate - Attempted to Emit \"Constraint Only\" Block With No Outputs");
    return Node::emitCExpr(cStatementQueue, schedType, outputPortNum, imag);
}

DataTypeDuplicate::DataTypeDuplicate(std::shared_ptr<SubSystem> parent, DataTypeDuplicate* orig) : PrimitiveNode(parent, orig){
    //No new values to copy, just call superclass constructor
}

std::shared_ptr<Node> DataTypeDuplicate::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<DataTypeDuplicate>(parent, this);
}
