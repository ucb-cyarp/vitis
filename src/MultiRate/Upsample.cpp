//
// Created by Christopher Yarp on 4/16/20.
//

#include "Upsample.h"
#include "UpsampleOutput.h"
#include "General/ErrorHelpers.h"
#include "GraphCore/NodeFactory.h"
#include "General/EmitterHelpers.h"

Upsample::Upsample() : upsampleRatio(0) {
}


Upsample::Upsample(std::shared_ptr<SubSystem> parent) : RateChange(parent), upsampleRatio(0) {

}

Upsample::Upsample(std::shared_ptr<SubSystem> parent, Upsample *orig) : RateChange(parent, orig), upsampleRatio(orig->upsampleRatio) {

}

void Upsample::populateParametersExceptRateChangeNodes(std::shared_ptr<Upsample> orig) {
    name = orig->getName();
    partitionNum = orig->getPartitionNum();
    schedOrder = orig->getSchedOrder();
    upsampleRatio = orig->getUpsampleRatio();
}

int Upsample::getUpsampleRatio() const {
    return upsampleRatio;
}

void Upsample::setUpsampleRatio(int upsampleRatio) {
    Upsample::upsampleRatio = upsampleRatio;
}

void Upsample::populateUpsampleParametersFromGraphML(int id, std::string name,
                                                   std::map<std::string, std::string> dataKeyValueMap,
                                                   GraphMLDialect dialect) {
    setId(id);
    setName(name);

    if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //==== Check Supported Config (Only if Simulink Import)====
        std::string phaseStr = dataKeyValueMap.at("Numeric.phase");
        int phase = std::stoi(phaseStr);
        if(phase!=0){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Upsample blocks must have phase 0", getSharedPointer()));
        }
    }

    std::string upsampleRatioStr;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- UpsampleRatio
        upsampleRatioStr = dataKeyValueMap.at("UpsampleRatio");
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        upsampleRatioStr = dataKeyValueMap.at("Numeric.N");
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Upsample", getSharedPointer()));
    }

    upsampleRatio = std::stoi(upsampleRatioStr);
}

void Upsample::emitGraphMLProperties(xercesc::DOMDocument *doc, xercesc::DOMElement *thisNode) {
    GraphMLHelper::addDataNode(doc, thisNode, "UpsampleRatio", GeneralHelper::to_string(upsampleRatio));
}

std::shared_ptr<Upsample>
Upsample::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                            std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<Upsample> newNode = NodeFactory::createNode<Upsample>(parent);

    newNode->populateUpsampleParametersFromGraphML(id, name, dataKeyValueMap, dialect);

    return newNode;
}

std::set<GraphMLParameter> Upsample::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("UpsampleRatio", "string", true));

    return parameters;
}

xercesc::DOMElement *
Upsample::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "RateChange"); //This is a special type of block called a RateChange
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Upsample");

    emitGraphMLProperties(doc, thisNode);

    return thisNode;
}

std::string Upsample::typeNameStr(){
    return "Upsample";
}

std::string Upsample::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() + "\nUpsampleRatio:" + GeneralHelper::to_string(upsampleRatio);

    return label;
}

void Upsample::validate() {
    RateChange::validate();

    //Check that input port and the output port have the same type
    DataType outType = getOutputPort(0)->getDataType();
    DataType inType = getInputPort(0)->getDataType();

    if(inType != outType){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Upsample - DataType of Input Port Does not Match Output Port", getSharedPointer()));
    }

    //TODO: Add width options (single to vector)

    if(upsampleRatio < 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Upsample - Upsample (" + GeneralHelper::to_string(upsampleRatio) + ") should be >= 1", getSharedPointer()));
    }

    //TODO: In Subclass, call validate method for Input/Ouput rate change blocks
}

//For the generic upsample node, it will be claimed that is combinational with no state.  In the specialized implementation
//versions for inputs and outputs, this may change.  It should be converted to a specialized version before state elements
//are introduced.  This should probably be done after pruning.

std::shared_ptr<Node> Upsample::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Upsample>(parent, this);
}

std::pair<int, int> Upsample::getRateChangeRatio(){
    return std::pair<int, int>(upsampleRatio, 1);
}

std::shared_ptr<RateChange>
Upsample::convertToRateChangeInputOutput(bool convertToInput, std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                         std::vector<std::shared_ptr<Node>> &nodesToRemove,
                                         std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                         std::vector<std::shared_ptr<Arc>> &arcToRemove) {
    //Note that addRemoveNodesAndArcs will call removeKnownReferences

    std::shared_ptr<UpsampleOutput> specificRcNode;

    if(convertToInput){
        //TODO: Implement
        throw std::runtime_error(
                ErrorHelpers::genErrorStr("Upsample not yet implemented", getSharedPointer()));
    }else {
        //Create New node and transfer parameters
        std::shared_ptr<UpsampleOutput> upsampleOutput = NodeFactory::createNode<UpsampleOutput>(parent);
        nodesToAdd.push_back(upsampleOutput); //Add to design
        specificRcNode = upsampleOutput;

        std::shared_ptr<Upsample> thisAsUpsample = std::static_pointer_cast<Upsample>(getSharedPointer());
        upsampleOutput->populateParametersExceptRateChangeNodes(thisAsUpsample);

        //Rewire arcs
        EmitterHelpers::transferArcs(thisAsUpsample, upsampleOutput);

        //Add this node to the nodes to be removed
        nodesToRemove.push_back(thisAsUpsample);
    }

    return specificRcNode;
}