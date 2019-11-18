//
// Created by Christopher Yarp on 7/16/18.
//

#include "Constant.h"
#include "GraphCore/NodeFactory.h"
#include "General/ErrorHelpers.h"

Constant::Constant() {

}

Constant::Constant(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent){

}

std::vector<NumericValue> Constant::getValue() const {
    return value;
}

void Constant::setValue(const std::vector<NumericValue> &values) {
    Constant::value = values;
}

std::shared_ptr<Constant>
Constant::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                            std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {

    //==== Create Node and set common properties ====
    std::shared_ptr<Constant> newNode = NodeFactory::createNode<Constant>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    std::string valueStr;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- Value
        valueStr = dataKeyValueMap.at("Value");
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Names -- Numeric.Value
        valueStr = dataKeyValueMap.at("Numeric.Value");
    } else
    {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Constant", newNode));
    }

    std::vector<NumericValue> values = NumericValue::parseXMLString(valueStr);
    newNode->setValue(values);

    return newNode;
}

std::set<GraphMLParameter> Constant::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("Value", "string", true));

    return  parameters;
}

xercesc::DOMElement *
Constant::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Constant");

    GraphMLHelper::addDataNode(doc, thisNode, "Value", NumericValue::toString(value));

    return thisNode;
}

std::string Constant::typeNameStr(){
    return "Constant";
}

std::string Constant::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() + "\nValue:" + NumericValue::toString(value);

    return label;
}

void Constant::validate() {
    Node::validate();

    //Should have 0 input ports and 1 output port
    if(inputPorts.size() != 0){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Constant - Should Have Exactly 0 Input Ports", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Constant - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    //Check there is at least 1 constant value
    if(value.size() < 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Constant - Should Have at Least 1 Value", getSharedPointer()));
    }

    //Check that width of value is the width of the output
    if(value.size() != getOutputPort(0)->getDataType().getWidth()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Constant - Width of Value Does Not Match Width of Output", getSharedPointer()));
    }
}

CExpr Constant::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    //Get the datatype of the output port
    std::shared_ptr<OutputPort> outputPort = getOutputPort(outputPortNum);
    DataType outputType = outputPort->getDataType();

    DataType outputTypeSingle = outputType;
    outputTypeSingle.setWidth(1);
    DataType outputCPUType = outputTypeSingle.getCPUStorageType(); //Want the CPU type to not have the array suffix since the cast will be for each element

    //Should have already validated and checked width of datatype and value match

    std::string expr = "";

    //Check if the type has width > 1, if so, need to emit an array
    unsigned long width = value.size();

    if(width == 1){
        expr += "(";

        //Emit datatype (the CPU type used for storage)
        expr += "(" + outputCPUType.toString(DataType::StringStyle::C) + ") ";
        //Emit value
        expr += value[0].toStringComponent(imag, outputType); //Convert to the real type, not the CPU storage type
        expr += ")";
    }else{
        //Emit an array

        std::string storageTypeStr = outputCPUType.toString(DataType::StringStyle::C);

        expr += "{";

        //Emit datatype (the CPU type used for storage)
        expr += "(" + storageTypeStr + ") ";
        //Emit Value
        expr += value[0].toStringComponent(imag, outputType); //Convert to the real type, not the CPU storage type

        for(unsigned long i = 0; i<width; i++){
            //Emit datatype (the CPU type used for storage)
            expr += ", (" + storageTypeStr + ") ";
            //Emit Value
            expr += value[i].toStringComponent(imag, outputType); //Convert to the real type, not the CPU storage type
        }

        expr += "}";
    }

    return CExpr(expr, false);
}

std::string
Constant::emitC(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag,
                bool checkFanout, bool forceFanout) {
    //TODO: should forced fanout be allowed - ie. should it be made possible for the user to mandate that constants be stored in temps
    //Run the parent emitC routien except do not check for fanout.
    return Node::emitC(cStatementQueue, schedType, outputPortNum, imag, false, false);
}

Constant::Constant(std::shared_ptr<SubSystem> parent, Constant* orig) : PrimitiveNode(parent, orig), value(orig->value) {

}

std::shared_ptr<Node> Constant::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Constant>(parent, this);
}


