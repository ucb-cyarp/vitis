//
// Created by Christopher Yarp on 7/23/18.
//

#include "SimulinkMultiPortSwitch.h"

#include "GraphCore/ExpandedNode.h"
#include "PrimitiveNodes/Mux.h"
#include "PrimitiveNodes/Constant.h"
#include "PrimitiveNodes/Sum.h"

#include <iostream>

SimulinkMultiPortSwitch::IndexType SimulinkMultiPortSwitch::parseIndexTypeStr(std::string str) {
    if(str == "ZERO_BASED"){
        return IndexType::ZERO_BASED;
    }else if(str == "ONE_BASED"){
        return IndexType::ONE_BASED;
    }else if(str == "CUSTOM"){
        return IndexType::CUSTOM;
    }else{
        throw std::runtime_error("Unknown index type: " + str);
    }
}

std::string SimulinkMultiPortSwitch::indexTypeToString(SimulinkMultiPortSwitch::IndexType indexType) {
    if(indexType == IndexType::ZERO_BASED){
        return "ZERO_BASED";
    }else if(indexType == IndexType::ONE_BASED){
        return "ONE_BASED";
    }else if(indexType == IndexType::CUSTOM){
        return "CUSTOM";
    }else{
        throw std::runtime_error("Unknown index type");
    }
}

SimulinkMultiPortSwitch::SimulinkMultiPortSwitch() : indexType(IndexType::ONE_BASED){

}

SimulinkMultiPortSwitch::SimulinkMultiPortSwitch(std::shared_ptr<SubSystem> parent) : MediumLevelNode(parent), indexType(IndexType::ONE_BASED) {

}

SimulinkMultiPortSwitch::IndexType SimulinkMultiPortSwitch::getIndexType() const {
    return indexType;
}

void SimulinkMultiPortSwitch::setIndexType(SimulinkMultiPortSwitch::IndexType indexType) {
    SimulinkMultiPortSwitch::indexType = indexType;
}

std::shared_ptr<SimulinkMultiPortSwitch>
SimulinkMultiPortSwitch::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                           std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node & Set Common Properties ====
    std::shared_ptr<SimulinkMultiPortSwitch> newNode = NodeFactory::createNode<SimulinkMultiPortSwitch>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    IndexType indexTypeVal;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- IndexType
        indexTypeVal = parseIndexTypeStr(dataKeyValueMap.at("IndexType"));
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Names -- DataPortOrder
        std::string indexTypeStr = dataKeyValueMap.at("DataPortOrder");

        if(indexTypeStr == "Zero-based contiguous"){
            indexTypeVal = IndexType::ZERO_BASED;
        }else if(indexTypeStr == "One-based contiguous"){
            indexTypeVal = IndexType::ONE_BASED;
        }else if(indexTypeStr == "Specify indices"){
            indexTypeVal = IndexType::CUSTOM;
        }else{
            throw std::runtime_error("Unknown Simulink indexing type - SimulinkMultiPortSwitch");
        }
    } else
    {
        throw std::runtime_error("Unsupported Dialect when parsing XML - SimulinkMultiPortSwitch");
    }

    //TODO: Support Custom IndexType
    if(indexTypeVal == IndexType::CUSTOM){
        throw std::runtime_error("IndexType CUSTOM is not currently supported for SimulinkMultiPortSwitch");
    }

    newNode->setIndexType(indexTypeVal);

    return newNode;
}

bool SimulinkMultiPortSwitch::expand(std::vector<std::shared_ptr<Node>> &new_nodes,
                                     std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                     std::vector<std::shared_ptr<Arc>> &new_arcs,
                                     std::vector<std::shared_ptr<Arc>> &deleted_arcs) {
    //Validate first to check that the CompareToConstant block is properly wired
    validate();

    //---- Expand the CompareToConstant node into a compare and constant ----
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

    //++++ Create Mux Block and Rewire ++++
    std::shared_ptr<Mux> muxNode = NodeFactory::createNode<Mux>(thisParent);
    muxNode->setName("Mux");
    new_nodes.push_back(muxNode);


    //Rewire inputs
    unsigned long numInPorts = inputPorts.size();
    for(unsigned long i = 1; i<numInPorts; i++){//Start with port 1 as this is the first data port
        std::shared_ptr<Arc> inputArc = *(inputPorts[i]->getArcs().begin());
        muxNode->addInArcUpdatePrevUpdateArc(i-1, inputArc); //Rewire to i-1 port of Mux
    }

    //Rewire outputs
    std::set<std::shared_ptr<Arc>> outputArcs = outputPorts[0]->getArcs(); //Make a copy of the arc set since re-assigning the arc will remove it from the port's set
    for(auto outputArc = outputArcs.begin(); outputArc != outputArcs.end(); outputArc++) {
        muxNode->addOutArcUpdatePrevUpdateArc(0, *outputArc);
    }

    //++++ Rewire the select line and decrement by 1 if needed ++++
    //Get the select Line
    std::shared_ptr<Arc> inputArcSel = *(inputPorts[0]->getArcs().begin());

    if(indexType == IndexType::ZERO_BASED){
        //Zero based, simply wire up the select input to the select port of the mux
        muxNode->addSelectArcUpdatePrevUpdateArc(inputArcSel);
    }else if(indexType == IndexType::ONE_BASED){
        //Need to decrement the index
        std::shared_ptr<Constant> constantNode = NodeFactory::createNode<Constant>(parent);
        constantNode->setName("Constant");
        constantNode->setValue(std::vector<NumericValue>{NumericValue(-1, 0, std::complex<double>(0, 0), false, false)});
        new_nodes.push_back(constantNode);

        std::shared_ptr<Sum> minusNode = NodeFactory::createNode<Sum>(parent);
        minusNode->setName("Sub");
        minusNode->setInputSign(std::vector<bool>{true, false});
        new_nodes.push_back(minusNode);

        //Rewire select arc
        minusNode->addInArcUpdatePrevUpdateArc(0, inputArcSel); //Port 1 is the + port of the minusNode

        //Wire constant to subtract and subtract to select line
        std::shared_ptr<Arc> constantArc = Arc::connectNodes(constantNode, 0, minusNode, 1, inputArcSel->getDataType());
        new_arcs.push_back(constantArc);

        std::shared_ptr<Arc> indArc = Arc::connectNodes(minusNode, 0, muxNode, inputArcSel->getDataType());
        new_arcs.push_back(indArc);

    }else if(indexType == IndexType::CUSTOM){
        //TODO: Implement CUSTOM IndexType (involves including a LUT in expansion)
        throw std::runtime_error("CUSTOM IndexType is not currently supported for SimulinkMultiPortSwitch");
    }else{
        throw std::runtime_error("Unknown Index Type While Expanding - SimulinkMultiPortSwitch");
    }

    return true;
}

std::set<GraphMLParameter> SimulinkMultiPortSwitch::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("IndexType", "string", true));

    return parameters;
}

xercesc::DOMElement *SimulinkMultiPortSwitch::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode,
                                                          bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "SimulinkMultiPortSwitch");

    GraphMLHelper::addDataNode(doc, thisNode, "IndexType", indexTypeToString(indexType));

    return thisNode;
}

std::string SimulinkMultiPortSwitch::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: SimulinkMultiPortSwitch\nIndexType: " + indexTypeToString(indexType);

    return label;
}

void SimulinkMultiPortSwitch::validate() {
    Node::validate();


    if(inputPorts.size() < 2){
        throw std::runtime_error("Validation Failed - SimulinkMultiPortSwitch - Should Have 2 or More Input Ports");
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - SimulinkMultiPortSwitch - Should Have Exactly 1 Output Port");
    }

    //Check that the select port is real
    std::shared_ptr<Arc> selectArc = *(inputPorts[0]->getArcs().begin());
    if(selectArc->getDataType().isComplex()){
        throw std::runtime_error("Validation Failed - SimulinkMultiPortSwitch - Select Port Cannot be Complex");
    }

    //Check for sufficient width if indexType is ONE_BASED
    if(indexType==IndexType::ONE_BASED && !selectArc->getDataType().isFloatingPt() && (selectArc->getDataType().getTotalBits()-selectArc->getDataType().getFractionalBits()) < 2){
        throw std::runtime_error("Validation Failed - SimulinkMultiPortSwitch - Select Port Does Not Have Sufficient Width");
    }


    //warn if floating point type
    //TODO: enforce integer for select (need to rectify Simulink use of double)
    if(selectArc->getDataType().isFloatingPt()){
        std::cerr << "Warning: SimulinkMultiPortSwitch Select Port is Driven by Floating Point" << std::endl;
    }

    //Check that all input ports and the output port have the same type
    DataType outType = getOutputPort(0)->getDataType();
    unsigned long numInputPorts = inputPorts.size();

    for(unsigned long i = 0; i<numInputPorts; i++){
        DataType inType = getInputPort(i)->getDataType();

        if(inType != outType){
            throw std::runtime_error("Validation Failed - SimulinkMultiPortSwitch - DataType of Input Port Does not Match Output Port");
        }
    }
}

SimulinkMultiPortSwitch::SimulinkMultiPortSwitch(std::shared_ptr<SubSystem> parent,
                                                 std::shared_ptr<SimulinkMultiPortSwitch> orig): MediumLevelNode(parent, orig), indexType(orig->indexType) {

}

