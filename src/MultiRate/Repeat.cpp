//
// Created by Christopher Yarp on 4/21/20.
//

#include "Repeat.h"
#include "General/ErrorHelpers.h"
#include "GraphCore/NodeFactory.h"
#include "RepeatOutput.h"
#include "General/EmitterHelpers.h"

Repeat::Repeat() : upsampleRatio(0) {

}

Repeat::Repeat(std::shared_ptr<SubSystem> parent) : RateChange(parent), upsampleRatio(0) {

}

Repeat::Repeat(std::shared_ptr<SubSystem> parent, Repeat* orig) : RateChange(parent, orig), upsampleRatio(orig->upsampleRatio) {

}

void Repeat::populateParametersExceptRateChangeNodes(std::shared_ptr<Repeat> orig) {
    name = orig->getName();
    partitionNum = orig->getPartitionNum();
    schedOrder = orig->getSchedOrder();
    upsampleRatio = orig->getUpsampleRatio();
}

int Repeat::getUpsampleRatio() const {
    return upsampleRatio;
}

void Repeat::setUpsampleRatio(int upsampleRatio) {
    Repeat::upsampleRatio = upsampleRatio;
}

void Repeat::populateRepeatParametersFromGraphML(int id, std::string name,
                                                     std::map<std::string, std::string> dataKeyValueMap,
                                                     GraphMLDialect dialect) {
    setId(id);
    setName(name);

    //Note:  Repeat does not have a phase parameter in simulink and thus it will not be checked.

    std::string upsampleRatioStr;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- UpsampleRatio
        upsampleRatioStr = dataKeyValueMap.at("UpsampleRatio");
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        upsampleRatioStr = dataKeyValueMap.at("Numeric.N");
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Repeat", getSharedPointer()));
    }

    upsampleRatio = std::stoi(upsampleRatioStr);
}

void Repeat::emitGraphMLProperties(xercesc::DOMDocument *doc, xercesc::DOMElement *thisNode) {
    GraphMLHelper::addDataNode(doc, thisNode, "UpsampleRatio", GeneralHelper::to_string(upsampleRatio));
}

std::shared_ptr<Repeat>
Repeat::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                            std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<Repeat> newNode = NodeFactory::createNode<Repeat>(parent);

    newNode->populateRepeatParametersFromGraphML(id, name, dataKeyValueMap, dialect);

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

    Repeat::emitGraphMLProperties(doc, thisNode);

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

    if(useVectorSamplingMode){
        DataType inputDTScaled = inType;
        //Scale the outer dimension
        std::vector<int> inputDim = inputDTScaled.getDimensions();
        std::pair<int, int> rateChange = getRateChangeRatio();
        inputDim[0] *= rateChange.first;
        inputDim[0] /= rateChange.second;
        inputDTScaled.setDimensions(inputDim);

        if(inputDTScaled != outType){
            throw std::runtime_error(ErrorHelpers::genErrorStr("When using VectorSamplingMode, output dimensions should be scaled relative to input dimensions", getSharedPointer()));
        }
    }else {
        if (inType != outType) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "Validation Failed - Repeat - DataType of Input Port Does not Match Output Port",
                    getSharedPointer()));
        }
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

std::shared_ptr<RateChange>
Repeat::convertToRateChangeInputOutput(bool convertToInput, std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                       std::vector<std::shared_ptr<Node>> &nodesToRemove,
                                       std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                       std::vector<std::shared_ptr<Arc>> &arcToRemove) {
    //Note that addRemoveNodesAndArcs will call removeKnownReferences

    std::shared_ptr<Repeat> specificRcNode;

    if(convertToInput){
        //TODO: Implement
        throw std::runtime_error(
                ErrorHelpers::genErrorStr("RepeatInput not yet implemented", getSharedPointer()));
    }else {
        //Create New node and transfer parameters
        std::shared_ptr<RepeatOutput> repeatOutput = NodeFactory::createNode<RepeatOutput>(parent);
        nodesToAdd.push_back(repeatOutput); //Add to design
        specificRcNode = repeatOutput;

        std::shared_ptr<Repeat> thisAsRepeat = std::static_pointer_cast<Repeat>(getSharedPointer());
        repeatOutput->populateParametersExceptRateChangeNodes(thisAsRepeat);

        //Rewire arcs
        EmitterHelpers::transferArcs(thisAsRepeat, repeatOutput);

        //Add this node to the nodes to be removed
        nodesToRemove.push_back(thisAsRepeat);
    }

    return specificRcNode;
}