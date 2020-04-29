//
// Created by Christopher Yarp on 4/21/20.
//

#include "Downsample.h"
#include "GraphCore/NodeFactory.h"
#include "General/ErrorHelpers.h"
#include "MultiRateHelpers.h"

Downsample::Downsample() : downsampleRatio(0) {

}

Downsample::Downsample(std::shared_ptr<SubSystem> parent) : RateChange(parent), downsampleRatio(0) {

}

Downsample::Downsample(std::shared_ptr<SubSystem> parent, Downsample *orig) : RateChange(parent, orig), downsampleRatio(orig->downsampleRatio) {

}

int Downsample::getDownsampleRatio() const {
    return downsampleRatio;
}

void Downsample::setDownsampleRatio(int downsampleRatio) {
    Downsample::downsampleRatio = downsampleRatio;
}

std::shared_ptr<Downsample>
Downsample::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                            std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<Downsample> newNode = NodeFactory::createNode<Downsample>(parent);
    newNode->setId(id);
    newNode->setName(name);

    if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //==== Check Supported Config (Only if Simulink Import)====
        std::string phaseStr = dataKeyValueMap.at("Numeric.phase");
        int phase = std::stoi(phaseStr);
        if(phase!=0){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Downsample blocks must have phase 0", newNode));
        }
    }


    std::string downsampleRatioStr;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- DownsampleRatio
        downsampleRatioStr = dataKeyValueMap.at("DownsampleRatio");
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        downsampleRatioStr = dataKeyValueMap.at("Numeric.N");
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Downsample", newNode));
    }

    newNode->downsampleRatio = std::stoi(downsampleRatioStr);

    return newNode;
}

std::set<GraphMLParameter> Downsample::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("DownsampleRatio", "string", true));

    return parameters;
}

xercesc::DOMElement *
Downsample::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "RateChange"); //This is a special type of block called a RateChange
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Upsample");

    GraphMLHelper::addDataNode(doc, thisNode, "DownsampleRatio", GeneralHelper::to_string(downsampleRatio));

    return thisNode;
}

std::string Downsample::typeNameStr(){
    return "Downsample";
}

std::string Downsample::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() + "\nDownsampleRatio:" + GeneralHelper::to_string(downsampleRatio);

    return label;
}

void Downsample::validate() {
    Node::validate();

    //Should have 1 input ports and 1 output port
    if(inputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Downsample - Should Have Exactly 1 Input Port", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Downsample - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    //Check that input port and the output port have the same type
    DataType outType = getOutputPort(0)->getDataType();
    DataType inType = getInputPort(0)->getDataType();

    if(inType != outType){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Upsample - DataType of Input Port Does not Match Output Port", getSharedPointer()));
    }

    //TODO: Add width options (single to vector)

    if(downsampleRatio < 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Downsample - Downsample (" + GeneralHelper::to_string(downsampleRatio) + ") should be >= 1", getSharedPointer()));
    }
}

//For the generic upsample node, it will be claimed that is combinational with no state.  In the specialized implementation
//versions for inputs and outputs, this may change.  It should be converted to a specialized version before state elements
//are introduced.  This should probably be done after pruning.

std::shared_ptr<Node> Downsample::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Downsample>(parent, this);
}

std::pair<int, int> Downsample::getRateChangeRatio(){
    return std::pair<int, int>(1, downsampleRatio);
}