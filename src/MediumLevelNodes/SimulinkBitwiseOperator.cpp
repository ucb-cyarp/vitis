//
// Created by Christopher Yarp on 6/2/20.
//

#include "SimulinkBitwiseOperator.h"
#include "General/ErrorHelpers.h"
#include "PrimitiveNodes/Constant.h"

SimulinkBitwiseOperator::SimulinkBitwiseOperator() : bitwiseOp(BitwiseOperator::BitwiseOp::AND), useMask(false) {

}

SimulinkBitwiseOperator::SimulinkBitwiseOperator(std::shared_ptr<SubSystem> parent) : MediumLevelNode(parent), bitwiseOp(BitwiseOperator::BitwiseOp::AND), useMask(false){

}

SimulinkBitwiseOperator::SimulinkBitwiseOperator(std::shared_ptr<SubSystem> parent, SimulinkBitwiseOperator *orig) : MediumLevelNode(parent, orig), bitwiseOp(orig->bitwiseOp), mask(orig->mask), useMask(orig->useMask){

}

BitwiseOperator::BitwiseOp SimulinkBitwiseOperator::getBitwiseOp() const {
    return bitwiseOp;
}

void SimulinkBitwiseOperator::setBitwiseOp(BitwiseOperator::BitwiseOp bitwiseOp) {
    SimulinkBitwiseOperator::bitwiseOp = bitwiseOp;
}

std::vector<NumericValue> SimulinkBitwiseOperator::getMask() const {
    return mask;
}

void SimulinkBitwiseOperator::setMask(const std::vector<NumericValue> &mask) {
    SimulinkBitwiseOperator::mask = mask;
}

bool SimulinkBitwiseOperator::isUseMask() const {
    return useMask;
}

void SimulinkBitwiseOperator::setUseMask(bool useMask) {
    SimulinkBitwiseOperator::useMask = useMask;
}

std::shared_ptr<SimulinkBitwiseOperator>
SimulinkBitwiseOperator::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                           std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node & Set Common Properties ====
    std::shared_ptr<SimulinkBitwiseOperator> newNode = NodeFactory::createNode<SimulinkBitwiseOperator>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    std::string bitwiseOpStr;
    std::string maskStr;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- BitwiseOp, UseMask, Mask
        bitwiseOpStr = dataKeyValueMap.at("BitwiseOp");
        maskStr = dataKeyValueMap.at("Mask");
        std::string useMaskStr = dataKeyValueMap.at("UseMask");
        newNode->useMask = GeneralHelper::parseBool(useMaskStr);
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Names -- logicop, UseBitMask, Numeric.BitMask
        bitwiseOpStr = dataKeyValueMap.at("logicop");
        maskStr = dataKeyValueMap.at("Numeric.BitMask");
        std::string useMaskStr = dataKeyValueMap.at("UseBitMask");
        if(useMaskStr == "on"){
            newNode->useMask = true;
        }else if(useMaskStr == "off"){
            newNode->useMask = false;
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown UseBitMask Option: " + useMaskStr, newNode));
        }
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - SimulinkBitwiseOperator", newNode));
    }

    //This parse method works for both vitis and Simulink.  It contains shift operations which are handled seperatly
    newNode->bitwiseOp = BitwiseOperator::parseBitwiseOpString(bitwiseOpStr);

    std::vector<NumericValue> maskValue = NumericValue::parseXMLString(maskStr);
    newNode->mask = maskValue;

    return newNode;
}

std::set<GraphMLParameter> SimulinkBitwiseOperator::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("BitwiseOp", "string", true));
    parameters.insert(GraphMLParameter("Mask", "string", true));
    parameters.insert(GraphMLParameter("UseMask", "boolean", true));

    return parameters;
}

void SimulinkBitwiseOperator::validate() {
    Node::validate();

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkBitwiseOperator - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    if(!getOutputPort(0)->getDataType().isScalar() && !getOutputPort(0)->getDataType().isVector()){
        //TODO: Implement Vector Support
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkBitwiseOperator - Vector Support is Not Yet Implemented", getSharedPointer()));
    }
    for(int i = 0; i<inputPorts.size(); i++) {
        if (!getInputPort(i)->getDataType().isScalar() && !getInputPort(i)->getDataType().isVector()) {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkBitwiseOperator - Vector Support is Not Yet Implemented", getSharedPointer()));
        }
    }

    if(useMask){
        if(inputPorts.size() != 1){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkBitwiseOperator - Should Have Exactly 1 Input Port When Using Mask", getSharedPointer()));
        }

        if(mask.size() != 1){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkBitwiseOperator - Mask Should be Scalar", getSharedPointer()));
        }

        if(mask[0].isFractional()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkBitwiseOperator - Mask Should be Integer", getSharedPointer()));
        }

        if(mask[0].isComplex()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkBitwiseOperator - Mask Should be Real", getSharedPointer()));
        }
    }else{
        if(inputPorts.size() <= 1){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkBitwiseOperator - Should Have >1 Input Ports When Using Mask", getSharedPointer()));
        }
    }

    for(int i = 0; i<inputPorts.size(); i++) {
        if (getInputPort(i)->getDataType().isFloatingPt()) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "Validation Failed - SimulinkBitwiseOperator - Input Port Must be Integer", getSharedPointer()));
        }
    }
}

xercesc::DOMElement *SimulinkBitwiseOperator::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode,
                                                          bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "SimulinkBitwiseOperator");

    GraphMLHelper::addDataNode(doc, thisNode, "BitwiseOp", BitwiseOperator::bitwiseOpToString(bitwiseOp));
    GraphMLHelper::addDataNode(doc, thisNode, "Mask", NumericValue::toString(mask));
    GraphMLHelper::addDataNode(doc, thisNode, "UseMask", GeneralHelper::to_string(useMask));

    return thisNode;
}

std::string SimulinkBitwiseOperator::typeNameStr() {
    return "SimulinkBitwiseOperator";
}

std::string SimulinkBitwiseOperator::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() +
             "\nBitwiseOp:" + BitwiseOperator::bitwiseOpToString(bitwiseOp) +
             "\nMask:" + NumericValue::toString(mask) +
             "\nUseMask:" + GeneralHelper::to_string(useMask);

    return label;
}

std::shared_ptr<Node> SimulinkBitwiseOperator::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<SimulinkBitwiseOperator>(parent, this);
}

std::shared_ptr<ExpandedNode> SimulinkBitwiseOperator::expand(std::vector<std::shared_ptr<Node>> &new_nodes,
                                                              std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                              std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                              std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                              std::shared_ptr<MasterUnconnected> &unconnected_master) {
    //Validate first to check that Gain is properly wired (ie. there is the proper number of ports, only 1 input arc, etc.)
    validate();

    //---- Expand the SimulinkBitwiseOperator Node to a BitwiseOperator and Constant ----
    std::shared_ptr<SubSystem> thisParent = parent;

    //Create Expanded Node and Add to Parent
    std::shared_ptr<ExpandedNode> expandedNode = NodeFactory::createNode<ExpandedNode>(thisParent, shared_from_this());

    //Remove Current Node from Parent and Set Parent to nullptr
    if(thisParent != nullptr) {
        thisParent->removeChild(shared_from_this());
    }
    parent = nullptr;
    //Add This node to the list of nodes to remove from the node vector
    deleted_nodes.push_back(shared_from_this());

    //Add Expanded Node to Node List
    new_nodes.push_back(expandedNode);

    //++++ Create BitwiseOperator Block and Rewire ++++
    std::shared_ptr<BitwiseOperator> bitwiseOperatorNode = NodeFactory::createNode<BitwiseOperator>(expandedNode);
    bitwiseOperatorNode->setName("BitwiseOperator");
    bitwiseOperatorNode->setOp(bitwiseOp);
    new_nodes.push_back(bitwiseOperatorNode);

    //Rewire Output
    std::set<std::shared_ptr<Arc>> outputArcs = getOutputPort(0)->getArcs();
    for(auto const & outputArc : outputArcs){
        outputArc->setSrcPortUpdateNewUpdatePrev(bitwiseOperatorNode->getOutputPortCreateIfNot(0));
    }

    //Rewire Input 1
    std::set<std::shared_ptr<Arc>> input1Arc = getInputPort(0)->getArcs();
    //Already validated to have 1 port
    (*input1Arc.begin())->setDstPortUpdateNewUpdatePrev(bitwiseOperatorNode->getInputPortCreateIfNot(0));

    //Create Constant Node or Rewire Other Inputs
    if(useMask){
        std::shared_ptr<Constant> maskNode = NodeFactory::createNode<Constant>(expandedNode);
        maskNode->setName("Mask");
        maskNode->setValue(mask);
        new_nodes.push_back(maskNode);

        //Validated mask was an real integer
        //TODO: implement vector support
        DataType maskDT = DataType(false, mask[0].isSigned(), false, mask[0].numIntegerBits(), 0, {1});
        maskDT = maskDT.getCPUStorageType(); //Avoid unnecessary casting/masking

        std::shared_ptr<Arc> maskArc = Arc::connectNodes(maskNode->getOutputPortCreateIfNot(0), bitwiseOperatorNode->getInputPortCreateIfNot(1), maskDT);
        new_arcs.push_back(maskArc);
    }else{
        //Rewire inputs
        for(int i = 1; i<inputPorts.size(); i++) {
            std::set<std::shared_ptr<Arc>> inputArc = getInputPort(i)->getArcs();
            //Already validated to have 1 port
            (*inputArc.begin())->setDstPortUpdateNewUpdatePrev(bitwiseOperatorNode->getInputPortCreateIfNot(i));
        }
    }

    //Rewire Order constraints
    std::set<std::shared_ptr<Arc>> orderConstraintInputArcs = getOrderConstraintInputArcs();
    for (auto const &orderConstraintInputArc : orderConstraintInputArcs) {
        orderConstraintInputArc->setDstPortUpdateNewUpdatePrev(
                bitwiseOperatorNode->getOrderConstraintInputPortCreateIfNot());
    }

    std::set<std::shared_ptr<Arc>> orderConstraintOutputArcs = getOrderConstraintOutputArcs();
    for (auto const &orderConstraintOutputArc : orderConstraintOutputArcs) {
        orderConstraintOutputArc->setSrcPortUpdateNewUpdatePrev(
                bitwiseOperatorNode->getOrderConstraintOutputPortCreateIfNot());
    }

    return expandedNode;
}
