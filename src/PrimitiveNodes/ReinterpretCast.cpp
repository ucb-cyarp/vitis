//
// Created by Christopher Yarp on 2019-02-20.
//

#include "ReinterpretCast.h"

#include "GraphCore/NodeFactory.h"
#include "General/GeneralHelper.h"
#include <iostream>

ReinterpretCast::ReinterpretCast() : PrimitiveNode(), emittedBefore(false) {

}

ReinterpretCast::ReinterpretCast(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), emittedBefore(false) {

}

DataType ReinterpretCast::getTgtDataType() const {
    return tgtDataType;
}

void ReinterpretCast::setTgtDataType(const DataType dataType) {
    ReinterpretCast::tgtDataType = dataType;
}

std::shared_ptr<ReinterpretCast>
ReinterpretCast::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                      std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node and set common properties ====
    std::shared_ptr<ReinterpretCast> newNode = NodeFactory::createNode<ReinterpretCast>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    std::string datatypeStr;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- InheritType, TgtDataType (only if inherit type is InheritType::SPECIFIED)
        std::string inheritTypeStr = dataKeyValueMap.at("InheritType");

        datatypeStr = dataKeyValueMap.at("TgtDataType");

    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        throw std::runtime_error("Reinterpret Cast is not a Simulink Node");
    } else
    {
        throw std::runtime_error("Unsupported Dialect when parsing XML - Reinterpret Cast");
    }

    //NOTE: complex is set to true and width is set to 1 for now.  These will be resolved with a call to propagate from Arcs.
    DataType dataType;
    try {
        dataType = DataType(datatypeStr, true, 1);
        newNode->setTgtDataType(dataType);
    }catch(const std::invalid_argument& e){
        throw std::runtime_error("Warning: Could not parse specified DataType: " + datatypeStr);
    }

    return newNode;
}

std::set<GraphMLParameter> ReinterpretCast::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("TgtDataType", "string", true));

    return  parameters;
}

xercesc::DOMElement *ReinterpretCast::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode,
                                                     bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "DataTypeConversion");

    GraphMLHelper::addDataNode(doc, thisNode, "TgtDataType", tgtDataType.toString());

    return thisNode;
}

std::string ReinterpretCast::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: ReinterpretCast";
    label += "\nTgtDataType:" + tgtDataType.toString();

    return label;
}

void ReinterpretCast::propagateProperties() {
    if(getOutputPorts().size() < 1){
        throw std::runtime_error("Propagate Error - ReinterpretCast - No Output Ports (" + name + ")");
    }

    std::shared_ptr<OutputPort> output = getOutputPort(0);

    if(output->getArcs().size() < 1){
        throw std::runtime_error("Propagate Error - ReinterpretCast - No Output Arcs (" + name + ")");
    }

    //Have a valid arc
    std::shared_ptr<Arc> outArc = *(output->getArcs().begin());
    DataType outDataType = outArc->getDataType();

    //Propagate the complex and width to the tgtDataType as this was not known when the DataTypeConversion object was created
    tgtDataType.setComplex(outDataType.isComplex());
    tgtDataType.setWidth(outDataType.getWidth());
}

void ReinterpretCast::validate() {
    Node::validate(); //Perform the node level validation

    //Should have 1 input ports and 1 output port
    if(inputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - ReinterpretCast - Should Have Exactly 0 Input Port");
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - ReinterpretCast - Should Have Exactly 1 Output Port");
    }


    std::shared_ptr<Arc> outArc = *(outputPorts[0]->getArcs().begin());

    if(outArc->getDataType() != tgtDataType) {
        throw std::runtime_error("Validation Error - ReinterpretCast - Type Specified and Disagrees with Type of Output Arc");
    }

    //Check that the datatype is a CPU type (important for re-interpret cast_
    if(!tgtDataType.isCPUType()){
        throw std::runtime_error("Validation Error - ReinterpretCast - Type Specified must be a CPU Type");
    }

}

CExpr ReinterpretCast::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    std::string beforeCastName = name + "_n" + GeneralHelper::to_string(id) + "_b4_cast";
    std::string afterCastName = name + "_n" + GeneralHelper::to_string(id) + "_after_cast";

    //Get the Expression for the input (should only be 1)
    std::shared_ptr<OutputPort> srcOutputPort = getInputPort(0)->getSrcOutputPort();
    int srcOutputPortNum = srcOutputPort->getPortNum();
    std::shared_ptr<Node> srcNode = srcOutputPort->getParent();
    std::string inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, imag);

    DataType srcType = getInputPort(0)->getDataType();

    if(!emittedBefore) {
        emittedBefore = true;

        //TODO: Implement Vector Support
        if (getInputPort(0)->getDataType().getWidth() > 1) {
            throw std::runtime_error("C Emit Error - Sum Support for Vector Types has Not Yet Been Implemented");
        }

        //TODO: Implement Fixed Point Support
        if ((!getInputPort(0)->getDataType().isCPUType()) || (!getOutputPort(0)->getDataType().isCPUType())) {
            throw std::runtime_error(
                    "C Emit Error - DataType Conversion to/from Fixed Point Types has Not Yet Been Implemented");
        }

        if (tgtDataType != srcType) {
            //We actually need to do the cast
            cStatementQueue.push_back(srcType.toString(DataType::StringStyle::C, false) + " " + beforeCastName + " = " + inputExpr + ";");

            std::string castExpr = "*((" + tgtDataType.toString(DataType::StringStyle::C, false) + "*)((void*)(&" + beforeCastName + ")))";

            cStatementQueue.push_back(tgtDataType.toString(DataType::StringStyle::C, false) + " " + afterCastName + " = " + castExpr + ";");
        }
    }

    //Emit the final expression
    if (tgtDataType == srcType) {
        //Just return the input expression, no conversion required.
        return CExpr(inputExpr, false);
    } else {
        //Perform the reinterpret cast
        //TODO: Implement Fixed Point Support

        //Assign input expr to var
        return CExpr(afterCastName, true);
    }
}

ReinterpretCast::ReinterpretCast(std::shared_ptr<SubSystem> parent, ReinterpretCast* orig) : PrimitiveNode(parent, orig), tgtDataType(orig->tgtDataType), emittedBefore(orig->emittedBefore){

}

std::shared_ptr<Node> ReinterpretCast::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<ReinterpretCast>(parent, this);
}