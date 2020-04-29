//
// Created by Christopher Yarp on 4/21/20.
//

#include "Repeat.h"
#include "General/ErrorHelpers.h"
#include "GraphCore/NodeFactory.h"

Repeat::Repeat() : upsampleRatio(0) {

}

Repeat::Repeat(std::shared_ptr<SubSystem> parent) : RateChange(parent), upsampleRatio(0) {

}

Repeat::Repeat(std::shared_ptr<SubSystem> parent, Repeat* orig) : RateChange(parent, orig), upsampleRatio(orig->upsampleRatio) {

}

int Repeat::getUpsampleRatio() const {
    return upsampleRatio;
}

void Repeat::setUpsampleRatio(int upsampleRatio) {
    Repeat::upsampleRatio = upsampleRatio;
}

std::shared_ptr<Repeat>
Repeat::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                            std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<Repeat> newNode = NodeFactory::createNode<Repeat>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //Note:  Repeat does not have a phase parameter in simulink and thus it will not be checked.

    std::string upsampleRatioStr;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- UpsampleRatio
        upsampleRatioStr = dataKeyValueMap.at("UpsampleRatio");
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        upsampleRatioStr = dataKeyValueMap.at("Numeric.N");
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Repeat", newNode));
    }

    newNode->upsampleRatio = std::stoi(upsampleRatioStr);

    return newNode;
}

std::set<GraphMLParameter> Repeat::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("UpsampleRatio", "string", true));

    return parameters;
}

xercesc::DOMElement *
Repeat::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "RateChange"); //This is a special type of block called a RateChange
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Repeat");

    GraphMLHelper::addDataNode(doc, thisNode, "UpsampleRatio", GeneralHelper::to_string(upsampleRatio));

    return thisNode;
}

std::string Repeat::typeNameStr(){
    return "Repeat";
}

std::string Repeat::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() + "\nUpsampleRatio:" + GeneralHelper::to_string(upsampleRatio);

    return label;
}

void Repeat::validate() {
    Node::validate();

    //Should have 1 input ports and 1 output port
    if(inputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Repeat - Should Have Exactly 1 Input Port", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Repeat - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    //Check that input port and the output port have the same type
    DataType outType = getOutputPort(0)->getDataType();
    DataType inType = getInputPort(0)->getDataType();

    if(inType != outType){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Repeat - DataType of Input Port Does not Match Output Port", getSharedPointer()));
    }

    //TODO: Add width options (single to vector)

    if(upsampleRatio < 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Repeat - Upsample (" + GeneralHelper::to_string(upsampleRatio) + ") should be >= 1", getSharedPointer()));
    }
}

//For the generic upsample node, it will be claimed that is combinational with no state.  In the specialized implementation
//versions for inputs and outputs, this may change.  It should be converted to a specialized version before state elements
//are introduced.  This should probably be done after pruning.

std::shared_ptr<Node> Repeat::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Repeat>(parent, this);
}

std::pair<int, int> Repeat::getRateChangeRatio(){
    return std::pair<int, int>(upsampleRatio, 1);
}