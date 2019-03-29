//
// Created by Christopher Yarp on 2019-03-15.
//

#include <General/GeneralHelper.h>
#include "TappedDelay.h"
#include "General/ErrorHelpers.h"

int TappedDelay::getDelays() const {
    return delays;
}

void TappedDelay::setDelays(int delays) {
    TappedDelay::delays = delays;
}

std::vector<NumericValue> TappedDelay::getInitVals() const {
    return initVals;
}

void TappedDelay::setInitVals(const std::vector<NumericValue> &initVals) {
    TappedDelay::initVals = initVals;
}

TappedDelay::TappedDelay() : HighLevelNode(), delays(0) {

}

TappedDelay::TappedDelay(std::shared_ptr<SubSystem> parent) : HighLevelNode(parent), delays(0) {

}

std::shared_ptr<TappedDelay>
TappedDelay::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                               std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node & Set Common Properties ====
    std::shared_ptr<TappedDelay> newNode = NodeFactory::createNode<TappedDelay>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    std::string delaysStr;
    std::string initValStr;

    if(dialect == GraphMLDialect::SIMULINK_EXPORT){
        delaysStr = dataKeyValueMap.at("Numeric.NumDelays");
        initValStr = dataKeyValueMap.at("Numeric.vinit");
    }else if(dialect == GraphMLDialect::VITIS){
        delaysStr = dataKeyValueMap.at("Delays");
        initValStr = dataKeyValueMap.at("InitialStates");
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown GraphML Dialect", newNode));
    }

    std::vector<NumericValue> delays = NumericValue::parseXMLString(delaysStr);
    if(delays.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unable to Parse Delays in Tapped Delay Line", newNode));
    }

    if(delays[0].isComplex() || delays[0].isFractional()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unable to Parse Delays in Tapped Delay Line", newNode));
    }

    newNode->setDelays(delays[0].getRealInt());

    std::vector<NumericValue> initVals = NumericValue::parseXMLString(initValStr);
    newNode->setInitVals(initVals);

    return newNode;
}

std::shared_ptr<ExpandedNode>
TappedDelay::expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes,
                    std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                    std::shared_ptr<MasterUnconnected> &unconnected_master) {
    //Validate first to check that the DiscreteFIR block is properly wired
    validate();

    //TODO: Implement FIR Expansion
    throw std::runtime_error(ErrorHelpers::genErrorStr("TappedDelay Expansion not yet implemented in VITIS.  If importing from Simulink, use Simulink expansion option in export script", getSharedPointer()));

    return Node::expand(new_nodes, deleted_nodes, new_arcs, deleted_arcs, unconnected_master);
}

std::set<GraphMLParameter> TappedDelay::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("Delays", "string", true));
    parameters.insert(GraphMLParameter("InitialStates", "string", true));

    return parameters;
}

xercesc::DOMElement *
TappedDelay::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "TappedDelay");

    GraphMLHelper::addDataNode(doc, thisNode, "Delays", GeneralHelper::to_string(delays));
    GraphMLHelper::addDataNode(doc, thisNode, "InitialStates", NumericValue::toString(initVals));

    return thisNode;
}

std::string TappedDelay::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: TappedDelay\nDelays: " + GeneralHelper::to_string(delays);
    label += "\nInitialStates: " + NumericValue::toString(initVals);

    return label;
}

void TappedDelay::validate() {
    Node::validate();

    if (inputPorts.size() != 1) {
        throw std::runtime_error(ErrorHelpers::genErrorStr(
                "Validation Failed - TappedDelay - Should Have Exactly 1 Input Port With Fixed Coefs", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - TappedDelay - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

}

std::shared_ptr<Node> TappedDelay::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<TappedDelay>(parent, this);
}

TappedDelay::TappedDelay(std::shared_ptr<SubSystem> parent, TappedDelay *orig) : HighLevelNode(parent, orig), delays(orig->delays), initVals(orig->initVals) {

}
