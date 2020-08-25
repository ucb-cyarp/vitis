//
// Created by Christopher Yarp on 7/16/18.
//

#include "Constant.h"
#include "GraphCore/NodeFactory.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"

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
    if(value.size() != getOutputPort(0)->getDataType().numberOfElements()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Constant - Width of Value Does Not Match Width of Output", getSharedPointer()));
    }
}

CExpr Constant::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    //Get the datatype of the output port
    std::shared_ptr<OutputPort> outputPort = getOutputPort(outputPortNum);
    DataType outputType = outputPort->getDataType();

    DataType outputTypeSingle = outputType;
    outputTypeSingle.setDimensions({1});
    DataType outputCPUType = outputTypeSingle.getCPUStorageType(); //Want the CPU type to not have the array suffix since the cast will be for each element

    //Should have already validated and checked width of datatype and value match

    if(outputType.isScalar()){
        std::string expr = "(";

        //Emit datatype (the CPU type used for storage)
        expr += "(" + outputCPUType.toString(DataType::StringStyle::C) + ") ";
        //Emit value
        expr += value[0].toStringComponent(imag, outputType); //Convert to the real type, not the CPU storage type
        expr += ")";
        return CExpr(expr, false);
    }else{
        //Return the globally declared array
        std::string constVarName = name + "_n" + GeneralHelper::to_string(id) + "_ConstVal";
        //Should be called after arcs have been added
        DataType outputStorageType = outputType.getCPUStorageType();
        Variable constVar = Variable(constVarName, outputStorageType);

        return CExpr(constVar.getCVarName(imag), true);
    }
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

bool Constant::hasGlobalDecl() {
    //The constant is declared globally if it is a vector/matrix;
    return value.size() > 1;
}

std::string Constant::getGlobalDecl() {
    //Should be validated before this point
    if(value.size()>1){
        //Emit an array
        std::string constVarName = name + "_n" + GeneralHelper::to_string(id) + "_ConstVal";

        //Should be called after arcs have been added
        DataType outputType = getOutputPort(0)->getDataType();
        DataType outputStorageType = outputType.getCPUStorageType();
        Variable constVar = Variable(constVarName, outputStorageType);

        std::vector<int> outputDimensions = outputType.getDimensions();
        std::string expr = "const " + constVar.getCVarDecl(false, true, false, true, false) +
                " = " +  EmitterHelpers::arrayLiteral(outputDimensions, value, false, outputType, outputStorageType) + ";";

        if(outputType.isComplex()){
            expr += "\nconst " + constVar.getCVarDecl(true, true, false, true, false) +
                   " = " +  EmitterHelpers::arrayLiteral(outputDimensions, value, true, outputType, outputStorageType) + ";";
        }

        return expr;
    }

    return "";
}

EstimatorCommon::ComputeWorkload
Constant::getComputeWorkloadEstimate(bool expandComplexOperators, bool expandHighLevelOperators,
                                     ComputationEstimator::EstimatorOption includeIntermediateLoadStore,
                                     ComputationEstimator::EstimatorOption includeInputOutputLoadStores) {
    //There really isn't any operation from the constant.  The stores for the constant occure (if they even occur) before execution starts
    //Downstream nodes may load from the constant but that will be accounted for in the downstream node's I/O estimate
    return EstimatorCommon::ComputeWorkload();
}


