//
// Created by Christopher Yarp on 7/23/18.
//

#include "ThresholdSwitch.h"
#include "PrimitiveNodes/Mux.h"
#include "MediumLevelNodes/CompareToConstant.h"
#include "GraphCore/ExpandedNode.h"
#include "GraphCore/NodeFactory.h"
#include "General/ErrorHelpers.h"
#include <iostream>

ThresholdSwitch::ThresholdSwitch() : compareOp(Compare::CompareOp::LT) {

}

ThresholdSwitch::ThresholdSwitch(std::shared_ptr<SubSystem> parent) : MediumLevelNode(parent), compareOp(Compare::CompareOp::LT) {

}

std::vector<NumericValue> ThresholdSwitch::getThreshold() const {
    return threshold;
}

void ThresholdSwitch::setThreshold(const std::vector<NumericValue> &threshold) {
    ThresholdSwitch::threshold = threshold;
}

Compare::CompareOp ThresholdSwitch::getCompareOp() const {
    return compareOp;
}

void ThresholdSwitch::setCompareOp(Compare::CompareOp compareOp) {
    ThresholdSwitch::compareOp = compareOp;
}

std::shared_ptr<ThresholdSwitch>
ThresholdSwitch::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                   std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node & Set Common Properties ====
    std::shared_ptr<ThresholdSwitch> newNode = NodeFactory::createNode<ThresholdSwitch>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    std::vector<NumericValue> thresholdVal;
    Compare::CompareOp compareOpVal;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- Threshold, CompareOp
        thresholdVal = NumericValue::parseXMLString(dataKeyValueMap.at("Threshold"));
        compareOpVal = Compare::parseCompareOpString(dataKeyValueMap.at("CompareOp"));

    } else if(dialect == GraphMLDialect::SIMULINK_EXPORT){
        //Simulink Names -- Threshold, Criteria

        std::string compareOpStr = dataKeyValueMap.at("Criteria");
        //Comparison notation is different for the switch
        if(compareOpStr == "u2 > Threshold"){
            compareOpVal = Compare::CompareOp::GT;
            thresholdVal = NumericValue::parseXMLString(dataKeyValueMap.at("Numeric.Threshold"));
        }else if(compareOpStr == "u2 >= Threshold"){
            compareOpVal = Compare::CompareOp::GEQ;
            thresholdVal = NumericValue::parseXMLString(dataKeyValueMap.at("Numeric.Threshold"));
        }else if(compareOpStr == "u2 ~= 0"){
            compareOpVal = Compare::CompareOp::NEQ;
            thresholdVal.push_back(NumericValue(0, 0, std::complex<double>(0, 0), false, false)); //Set the constant to 0
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown Simulink ThresholdSwitch Comparison: " + compareOpStr, newNode));
        }

    } else
    {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - ThresholdSwitch", newNode));
    }


    newNode->setThreshold(thresholdVal);
    newNode->setCompareOp(compareOpVal);

    return newNode;
}

std::shared_ptr<ExpandedNode> ThresholdSwitch::expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                      std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                      std::shared_ptr<MasterUnconnected> &unconnected_master) {
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
    std::shared_ptr<Mux> muxNode = NodeFactory::createNode<Mux>(expandedNode);
    muxNode->setName("Mux");
    muxNode->setBooleanSelect(true);
    new_nodes.push_back(muxNode);

    //NOTE: Port 0 Is Assigned to Port 0
    //      Port 1 Is Assigned to Select (Or CompareToConstant then Select)
    //      Port 2 Is Assigned to Port 1
    //
    //      The signal to treat port 0 as true and port 1 as false (if/else mode) is controlled by the Mux parameter setBooleanSelect
    //
    //      Do Port 0 and Port 2 Now
    std::shared_ptr<Arc> inputArc0 = *(inputPorts[0]->getArcs().begin());
    muxNode->addInArcUpdatePrevUpdateArc(0, inputArc0); //Should remove the arc from this node's port

    std::shared_ptr<Arc> inputArc2 = *(inputPorts[2]->getArcs().begin());
    muxNode->addInArcUpdatePrevUpdateArc(1, inputArc2); //Should remove the arc from this node's port

    std::set<std::shared_ptr<Arc>> outputArcs = outputPorts[0]->getArcs(); //Make a copy of the arc set since re-assigning the arc will remove it from the port's set
    for(auto outputArc = outputArcs.begin(); outputArc != outputArcs.end(); outputArc++) {
        muxNode->addOutArcUpdatePrevUpdateArc(0, *outputArc);
    }

    //++++ Create CompareToConstant If Necessary, Direct Connect Otherwise ++++
    //Check if the input port is a bool or not (unsigned, real, int with 1 bit)
    std::shared_ptr<Arc> inputArcSel = *(inputPorts[1]->getArcs().begin());
    if(inputArcSel->getDataType().isBool()){
        //The select line is a bool, directly wire it to the mux select port
        //Note: boolean inputs were allowed for the switch block in Simulink.  Bool input
        //to select line ignore the threshold and compareOp

        //Note:  This should work because bool can be mapped to an int (false = 0, true = 1) which are used
        //       to drive the mux select.
        muxNode->addSelectArcUpdatePrevUpdateArc(inputArcSel);
    }else{
        //The input to the select line is not a boolean, insert a CompareToConstant block and wire
        std::shared_ptr<CompareToConstant> compareNode = NodeFactory::createNode<CompareToConstant>(expandedNode);
        compareNode->setName("CompareToConstant");
        compareNode->setCompareConst(threshold); //Set the threshold of the comparison
        compareNode->setCompareOp(compareOp); //Set the operator used in the comparison
        new_nodes.push_back(compareNode);

        //Wire input select line to input of CompareToConstant
        compareNode->addInArcUpdatePrevUpdateArc(0, inputArcSel);


        //Wire the output of the CompareToConstant to the select line of the Mux
        DataType compareOutputType = DataType(false, false, false, 1, 0, {1}); //Bool type of width 1
        //NOTE: This wire is a boolean type.  However, this should work because bool can be mapped to an int
        //      (false = 0, true = 1) which are used to drive the mux select.


        std::shared_ptr<Arc> compareOutputArc = Arc::connectNodes(compareNode, 0, muxNode, compareOutputType);
        new_arcs.push_back(compareOutputArc);
    }

    return expandedNode;
}

std::set<GraphMLParameter> ThresholdSwitch::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("Threshold", "string", true));
    parameters.insert(GraphMLParameter("CompareOp", "string", true));

    return parameters;
}

xercesc::DOMElement *
ThresholdSwitch::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "ThresholdSwitch");

    GraphMLHelper::addDataNode(doc, thisNode, "Threshold", NumericValue::toString(threshold));
    GraphMLHelper::addDataNode(doc, thisNode, "CompareOp", Compare::compareOpToString(compareOp));

    return thisNode;
}

std::string ThresholdSwitch::typeNameStr(){
    return "ThresholdSwitch";
}

std::string ThresholdSwitch::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() + "\nCompareOp: " + Compare::compareOpToString(compareOp) + "\nThreshold:" + NumericValue::toString(threshold);

    return label;
}

void ThresholdSwitch::validate() {
    Node::validate();

    if(inputPorts.size() != 3){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ThresholdSwitch - Should Have Exactly 3 Input Ports", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ThresholdSwitch - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    //Check that there is at least 1 value for the comparison
    if(threshold.size() < 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ThresholdSwitch - Should Have At Least 1 Threshold Value", getSharedPointer()));
    }

    //Check that all input ports and the output port have the same type
    //TODO: Simulink actually does not appear to enforce this.  Perhaps add auto datatype conversion?
    DataType outType = getOutputPort(0)->getDataType();
    unsigned long numInputPorts = inputPorts.size();

    //Port 0 is the line passed when true
    //Port 1 is the select line
    //Port 2 is the line passed when false
    DataType inType0 = getInputPort(0)->getDataType();
    if(inType0 != outType){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ThresholdSwitch - DataType of Input Port 0 Does not Match Output Port", getSharedPointer()));
    }

    DataType inType2 = getInputPort(2)->getDataType();
    if(inType2 != outType){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ThresholdSwitch - DataType of Input Port 2 Does not Match Output Port", getSharedPointer()));
    }

}

ThresholdSwitch::ThresholdSwitch(std::shared_ptr<SubSystem> parent, ThresholdSwitch* orig) : MediumLevelNode(parent, orig), threshold(orig->threshold), compareOp(orig->compareOp) {

}

std::shared_ptr<Node> ThresholdSwitch::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<ThresholdSwitch>(parent, this);
}

