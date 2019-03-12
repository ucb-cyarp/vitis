//
// Created by Christopher Yarp on 7/16/18.
//

#include "DataTypeConversion.h"

#include "GraphCore/NodeFactory.h"
#include <iostream>

DataTypeConversion::DataTypeConversion() : PrimitiveNode(), inheritType(DataTypeConversion::InheritType::SPECIFIED) {

}

DataTypeConversion::DataTypeConversion(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), inheritType(DataTypeConversion::InheritType::SPECIFIED) {

}

DataTypeConversion::InheritType DataTypeConversion::parseInheritType(std::string str) {
    if (str == "SPECIFIED") {
        return InheritType::SPECIFIED;
    } else if (str == "INHERIT_FROM_OUTPUT"){
        return InheritType::INHERIT_FROM_OUTPUT;
    }else{
        throw std::runtime_error("Unknown Inherit Type: " + str);
    }
}

std::string DataTypeConversion::inheritTypeToString(DataTypeConversion::InheritType type) {
    if(type == InheritType::SPECIFIED){
        return "SPECIFIED";
    }else if(type == InheritType::INHERIT_FROM_OUTPUT){
        return "INHERIT_FROM_OUTPUT";
    }else{
        throw std::runtime_error("Unknown Inherit Type");
    }
}

DataType DataTypeConversion::getTgtDataType() const {
    return tgtDataType;
}

void DataTypeConversion::setTgtDataType(const DataType dataType) {
    DataTypeConversion::tgtDataType = dataType;
}

DataTypeConversion::InheritType DataTypeConversion::getInheritType() const {
    return inheritType;
}

void DataTypeConversion::setInheritType(DataTypeConversion::InheritType inheritType) {
    DataTypeConversion::inheritType = inheritType;
}

std::shared_ptr<DataTypeConversion>
DataTypeConversion::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                   std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node and set common properties ====
    std::shared_ptr<DataTypeConversion> newNode = NodeFactory::createNode<DataTypeConversion>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    InheritType inheritTypeParsed;
    std::string datatypeStr;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- InheritType, TgtDataType (only if inherit type is InheritType::SPECIFIED)
        std::string inheritTypeStr = dataKeyValueMap.at("InheritType");

        //Convert this to an inheritType
        inheritTypeParsed = DataTypeConversion::parseInheritType(inheritTypeStr);

        //Get the datatype str if the inheritType is SPECIFIED
        if(inheritTypeParsed == InheritType::SPECIFIED){
            datatypeStr = dataKeyValueMap.at("TgtDataType");
        }
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Names -- OutDataTypeStr
        std::string simulinkStr = dataKeyValueMap.at("OutDataTypeStr");

        //Will specify either that the type is inherited or provides the type
        if(simulinkStr == "Inherit: Inherit via back propagation"){
            inheritTypeParsed = InheritType::INHERIT_FROM_OUTPUT;
        }else{
            inheritTypeParsed = InheritType::SPECIFIED;
            datatypeStr = simulinkStr;
        }
    } else
    {
        throw std::runtime_error("Unsupported Dialect when parsing XML - Constant");
    }

    //Set the inherit type
    newNode->setInheritType(inheritTypeParsed);

    //Parse the datatype if inheritType is SPECIFIED, otherwise accept the default
    if(inheritTypeParsed == InheritType::SPECIFIED){
        //NOTE: complex is set to true and width is set to 1 for now.  These will be resolved with a call to propagate from Arcs.
        DataType dataType;
        try {
            dataType = DataType(datatypeStr, true, 1);
            newNode->setTgtDataType(dataType);
        }catch(const std::invalid_argument& e){
            std::cerr << "Warning: Could not parse specified DataType: " << datatypeStr << ". Reverting DataTypeConvert " << newNode->getFullyQualifiedName() << " to Using Inherited Type ..." << std::endl;
            newNode->setInheritType(InheritType::INHERIT_FROM_OUTPUT);
        }
    }

    return newNode;
}

std::set<GraphMLParameter> DataTypeConversion::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("InheritType", "string", true));
    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("TgtDataType", "string", true));

    return  parameters;
}

xercesc::DOMElement *DataTypeConversion::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode,
                                                     bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "DataTypeConversion");

    GraphMLHelper::addDataNode(doc, thisNode, "InheritType", DataTypeConversion::inheritTypeToString(inheritType));

    //The TgtDataType is only included if the InheritType is Specified
    if(inheritType == InheritType::SPECIFIED){
        GraphMLHelper::addDataNode(doc, thisNode, "TgtDataType", tgtDataType.toString());
    }

    return thisNode;
}

std::string DataTypeConversion::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: DataTypeConversion\nInheritType:" + DataTypeConversion::inheritTypeToString(inheritType);

    //Include TgtDataType only if inheritType is SPECIFIED
    if(inheritType == InheritType::SPECIFIED){
        label += "\nTgtDataType:" + tgtDataType.toString();
    }

    return label;
}

void DataTypeConversion::propagateProperties() {
    if(getOutputPorts().size() < 1){
        throw std::runtime_error("Propagate Error - DataTypeConvert - No Output Ports (" + name + ")");
    }

    std::shared_ptr<OutputPort> output = getOutputPort(0);

    if(output->getArcs().size() < 1){
        throw std::runtime_error("Propagate Error - DataTypeConvert - No Output Arcs (" + name + ")");
    }

    //Have a valid arc
    std::shared_ptr<Arc> outArc = *(output->getArcs().begin());
    DataType outDataType = outArc->getDataType();

    //Propagate the complex and width to the tgtDataType as this was not known when the DataTypeConversion object was created
    tgtDataType.setComplex(outDataType.isComplex());
    tgtDataType.setWidth(outDataType.getWidth());
}

void DataTypeConversion::validate() {
    Node::validate(); //Perform the node level validation

    if(inheritType == InheritType::SPECIFIED) {
        //Perform the type check
        //Only need to do for 1 arc since Node validation checks that all arcs have the same type

        if(getOutputPorts().size() < 1) {
            throw std::runtime_error("Validation Error - DataTypeConversion - No Output Ports");
        }

        std::shared_ptr<OutputPort> output = getOutputPort(0);
        //Do not need to check if there are output arcs since the node level check enforces this

        std::shared_ptr<Arc> outArc = *(output->getArcs().begin());

        if(outArc->getDataType() != tgtDataType) {
            throw std::runtime_error("Validation Error - DataTypeConversion - Type Specified and Disagrees with Type of Output Arc");
        }
    }

    //Should have 1 input ports and 1 output port
    if(inputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - DataTypeConversion - Should Have Exactly 0 Input Port");
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - DataTypeConversion - Should Have Exactly 1 Output Port");
    }
}

CExpr DataTypeConversion::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    //TODO: Implement Vector Support
    if(getInputPort(0)->getDataType().getWidth()>1){
        throw std::runtime_error("C Emit Error - Sum Support for Vector Types has Not Yet Been Implemented");
    }

    //TODO: Implement Fixed Point Support
    if((!getInputPort(0)->getDataType().isCPUType()) || (!getOutputPort(0)->getDataType().isCPUType())) {
        throw std::runtime_error(
                "C Emit Error - DataType Conversion to/from Fixed Point Types has Not Yet Been Implemented");
    }

    //Get the Expression for the input (should only be 1)
    std::shared_ptr<OutputPort> srcOutputPort = getInputPort(0)->getSrcOutputPort();
    int srcOutputPortNum = srcOutputPort->getPortNum();
    std::shared_ptr<Node> srcNode = srcOutputPort->getParent();
    std::string inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, imag);

    //Type Conversion logic
    DataType tgtType;

    if(inheritType == DataTypeConversion::InheritType::SPECIFIED){
        tgtType = tgtDataType;
    }else if(inheritType == DataTypeConversion::InheritType::INHERIT_FROM_OUTPUT){
        tgtType = getOutputPort(0)->getDataType();
    }

    DataType srcType = getInputPort(0)->getDataType();


    if(tgtType == srcType){
        //Just return the input expression, no conversion required.
        return CExpr(inputExpr, false);
    }else{
        //Perform the cast
        //TODO: Implement Fixed Point Support

        std::string outputExpr = "((" + tgtType.toString(DataType::StringStyle::C, false) + ") (" + inputExpr + "))" ;
        return CExpr(outputExpr, false);
    }
}

DataTypeConversion::DataTypeConversion(std::shared_ptr<SubSystem> parent, DataTypeConversion* orig) : PrimitiveNode(parent, orig), tgtDataType(orig->tgtDataType), inheritType(orig->inheritType){

}

std::shared_ptr<Node> DataTypeConversion::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<DataTypeConversion>(parent, this);
}
