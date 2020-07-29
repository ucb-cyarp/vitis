//
// Created by Christopher Yarp on 7/16/18.
//

#include "DataTypeConversion.h"

#include "GraphCore/NodeFactory.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"
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
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown Inherit Type: " + str));
    }
}

std::string DataTypeConversion::inheritTypeToString(DataTypeConversion::InheritType type) {
    if(type == InheritType::SPECIFIED){
        return "SPECIFIED";
    }else if(type == InheritType::INHERIT_FROM_OUTPUT){
        return "INHERIT_FROM_OUTPUT";
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown Inherit Type"));
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
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Constant", newNode));
    }

    //Set the inherit type
    newNode->setInheritType(inheritTypeParsed);

    //Parse the datatype if inheritType is SPECIFIED, otherwise accept the default
    if(inheritTypeParsed == InheritType::SPECIFIED){
        //NOTE: complex is set to true and width is set to 1 for now.  These will be resolved with a call to propagate from Arcs.
        DataType dataType;
        try {
            dataType = DataType(datatypeStr, true, {1});
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

std::string DataTypeConversion::typeNameStr(){
    return "DataTypeConversion";
}

std::string DataTypeConversion::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() + "\nInheritType:" + DataTypeConversion::inheritTypeToString(inheritType);

    //Include TgtDataType only if inheritType is SPECIFIED
    if(inheritType == InheritType::SPECIFIED){
        label += "\nTgtDataType:" + tgtDataType.toString();
    }

    return label;
}

void DataTypeConversion::propagateProperties() {
    if(getOutputPorts().size() < 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Propagate Error - DataTypeConvert - No Output Ports (" + name + ")", getSharedPointer()));
    }

    std::shared_ptr<OutputPort> output = getOutputPort(0);

    if(output->getArcs().size() < 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Propagate Error - DataTypeConvert - No Output Arcs (" + name + ")", getSharedPointer()));
    }

    //Have a valid arc
    std::shared_ptr<Arc> outArc = *(output->getArcs().begin());
    DataType outDataType = outArc->getDataType();

    //Propagate the complex and width to the tgtDataType as this was not known when the DataTypeConversion object was created
    tgtDataType.setComplex(outDataType.isComplex());
    tgtDataType.setDimensions(outDataType.getDimensions());
}

void DataTypeConversion::validate() {
    Node::validate(); //Perform the node level validation

    if(inheritType == InheritType::SPECIFIED) {
        //Perform the type check
        //Only need to do for 1 arc since Node validation checks that all arcs have the same type

        if(getOutputPorts().size() < 1) {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Error - DataTypeConversion - No Output Ports", getSharedPointer()));
        }

        std::shared_ptr<OutputPort> output = getOutputPort(0);
        //Do not need to check if there are output arcs since the node level check enforces this

        std::shared_ptr<Arc> outArc = *(output->getArcs().begin());

        //Dimensions are not specified so do not check them as part of the type check
        DataType outType = outArc->getDataType();
        outType.setDimensions({1});
        DataType tgtType = tgtDataType;
        tgtType.setDimensions({1});
        if(outType != tgtType) {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Error - DataTypeConversion - Type Specified and Disagrees with Type of Output Arc", getSharedPointer()));
        }
    }

    //Should have 1 input ports and 1 output port
    if(inputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - DataTypeConversion - Should Have Exactly 1 Input Port", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - DataTypeConversion - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    if(getInputPort(0)->getDataType().getDimensions() != getOutputPort(0)->getDataType().getDimensions()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - DataTypeConversion - Input and output should have the same dimensions", getSharedPointer()));
    }
}

CExpr DataTypeConversion::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    //TODO: Implement Fixed Point Support
    if((!getInputPort(0)->getDataType().isCPUType()) || (!getOutputPort(0)->getDataType().isCPUType())) {
        throw std::runtime_error(ErrorHelpers::genErrorStr(
                "C Emit Error - DataType Conversion to/from Fixed Point Types has Not Yet Been Implemented", getSharedPointer()));
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
        //Inherit the dimensions from the output port
        tgtType.setDimensions(getOutputPort(0)->getDataType().getDimensions());
    }else if(inheritType == DataTypeConversion::InheritType::INHERIT_FROM_OUTPUT){
        tgtType = getOutputPort(0)->getDataType();
    }

    DataType srcType = getInputPort(0)->getDataType();

    Variable outVar;
    std::vector<std::string> forLoopIndexVars;
    std::vector<std::string> forLoopClose;
    if(!srcType.isScalar()){
        //Need to declare a temporary for the vector;
        std::string outputVarName = name + "_n" + GeneralHelper::to_string(id) + "_outVec";
        outVar = Variable(outputVarName, tgtType);
        cStatementQueue.push_back(outVar.getCVarDecl(imag, true, false, true, false) + ";");

        //Create nested loops for a given array
        std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                EmitterHelpers::generateVectorMatrixForLoops(srcType.getDimensions());

        std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
        forLoopIndexVars = std::get<1>(forLoopStrs);
        forLoopClose = std::get<2>(forLoopStrs);

        cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());
    }

    std::string inputExprDeref = inputExpr + (srcType.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars));
    //Only cast if the types are different
    std::string castExpr = (tgtType == srcType) ? inputExprDeref
                                                : "((" + tgtType.toString(DataType::StringStyle::C, false, false) + ") (" + inputExprDeref + "))";

    if(srcType.isScalar()){
        return CExpr(castExpr, false);
    }else{
        //emit assignment for vector
        cStatementQueue.push_back(outVar.getCVarName(imag) + EmitterHelpers::generateIndexOperation(forLoopIndexVars) + " = " + castExpr + ";");
        //Close for loop
        cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());

        return CExpr(outVar.getCVarName(imag), true);
    }
}

DataTypeConversion::DataTypeConversion(std::shared_ptr<SubSystem> parent, DataTypeConversion* orig) : PrimitiveNode(parent, orig), tgtDataType(orig->tgtDataType), inheritType(orig->inheritType){

}

std::shared_ptr<Node> DataTypeConversion::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<DataTypeConversion>(parent, this);
}
