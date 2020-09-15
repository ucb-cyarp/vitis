//
// Created by Christopher Yarp on 2019-02-21.
//

#include <General/GeneralHelper.h>
#include "BitwiseOperator.h"
#include "General/ErrorHelpers.h"

BitwiseOperator::BitwiseOp BitwiseOperator::parseBitwiseOpString(std::string str) {
    if(str == "NOT"){
        return BitwiseOp::NOT;
    }else if(str == "AND"){
        return BitwiseOp::AND;
    }else if(str == "OR"){
        return BitwiseOp::OR;
    }else if(str == "XOR"){
        return BitwiseOp::XOR;
    }else if(str == "SHIFT_LEFT"){
        return BitwiseOp::SHIFT_LEFT;
    }else if(str == "SHIFT_RIGHT"){
        return BitwiseOp::SHIFT_RIGHT;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Bitwise Operator Unsupported: " + str));
    }
}

std::string BitwiseOperator::bitwiseOpToString(BitwiseOperator::BitwiseOp op) {
    if(op == BitwiseOp::NOT){
        return "NOT";
    }else if(op == BitwiseOp::AND){
        return "AND";
    }else if(op == BitwiseOp::OR){
        return "OR";
    }else if(op == BitwiseOp::XOR){
        return "XOR";
    }else if(op == BitwiseOp::SHIFT_LEFT){
        return "SHIFT_LEFT";
    }else if(op == BitwiseOp::SHIFT_RIGHT){
        return "SHIFT_RIGHT";
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("BitwiseOp toString not implemented for this operator."));
    }
}

std::string BitwiseOperator::bitwiseOpToCString(BitwiseOperator::BitwiseOp op) {
    if(op == BitwiseOp::NOT){
        return "~";
    }else if(op == BitwiseOp::AND){
        return "&";
    }else if(op == BitwiseOp::OR){
        return "|";
    }else if(op == BitwiseOp::XOR){
        return "^";
    }else if(op == BitwiseOp::SHIFT_LEFT){
        return "<<";
    }else if(op == BitwiseOp::SHIFT_RIGHT){
        return ">>";
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("BitwiseOp toCString not implemented for this operator."));
    }
}

BitwiseOperator::BitwiseOp BitwiseOperator::getOp() const {
    return op;
}

void BitwiseOperator::setOp(BitwiseOperator::BitwiseOp op) {
    BitwiseOperator::op = op;
}

std::shared_ptr<BitwiseOperator>
BitwiseOperator::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                   std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<BitwiseOperator> newNode = NodeFactory::createNode<BitwiseOperator>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important property ====
    std::string bitwiseOpStr;
    std::string shiftAmountStr;

    if(dialect == GraphMLDialect::VITIS){
        //Vitis Name (LogicalOperator) -- LogicalOp
        bitwiseOpStr = dataKeyValueMap.at("BitwiseOp");
    } else if(dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Only support a subset of the options for the simulink version of the node
        bitwiseOpStr = dataKeyValueMap.at("logicop");
        if(bitwiseOpStr == "NAND" || bitwiseOpStr == "NOR"){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Simulink Bitwise Operator: " + bitwiseOpStr, newNode));
        }

        if(dataKeyValueMap.at("UseBitMask") != "off"){
            //TODO: Implement bitwise operator with constant expansion
            throw std::runtime_error(ErrorHelpers::genErrorStr("The UseBitMask option of Simulink Bitwise Operator is Currently Unsupported - Connect a Constant Block instead", newNode));
        }

        //Number of port error check handled in validate method

//        std::vector<NumericValue> numPorts = NumericValue::parseXMLString("Numeric.NumInputPorts");
//        if (numPorts.size() != 1) {
//            throw std::runtime_error("Error Parsing NumInputPorts - DigitalModulator");
//        }
//        if (numPorts[0].isComplex() || numPorts[0].isFractional()) {
//            throw std::runtime_error("Error Parsing NumInputPorts - DigitalModulator");
//        }
//
//        if(bitwiseOpStr != "NOT" && numPorts[0].getRealInt() != 2){
//            throw std::runtime_error("Only Simulink Bitwise Operators with 2 Input Ports (excepet for NOT) are Currently Supported");
//        }
    } else
    {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - BitwiseOperator", newNode));
    }

    //Get Operator
    BitwiseOp parsedLogicalOp = parseBitwiseOpString(bitwiseOpStr);
    newNode->setOp(parsedLogicalOp);

    return newNode;
}

std::set<GraphMLParameter> BitwiseOperator::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("BitwiseOp", "string", true));
    parameters.insert(GraphMLParameter("ShiftAmount", "string", true));

    return parameters;
}

xercesc::DOMElement *
BitwiseOperator::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "BitwiseOperator");

    GraphMLHelper::addDataNode(doc, thisNode, "BitwiseOp", bitwiseOpToString(op));

    return thisNode;
}

std::string BitwiseOperator::typeNameStr(){
    return "BitwiseOperator";
}

std::string BitwiseOperator::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() + "\nBitwiseOp:" + bitwiseOpToString(op);

    return label;
}

void BitwiseOperator::validate() {
    Node::validate();

    if(op == BitwiseOp::NOT) {
        if (inputPorts.size() != 1) {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - BitwiseOperator - Should Have Exactly 1 Input Port", getSharedPointer()));
        }
    }else if(op == BitwiseOp::SHIFT_LEFT || op == BitwiseOp::SHIFT_RIGHT) {
        if (inputPorts.size() != 2) {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - \"(\"+inputExprs[0]+\")\"  - Should Have at most 2 Input Ports", getSharedPointer()));
        }

        DataType shiftPortType = getOutputPort(0)->getDataType();
        if(shiftPortType.isComplex() || shiftPortType.isFloatingPt()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - BitwiseOperator - Shift Amount Should be a Real Integer", getSharedPointer()));
        }
    }else{
        if (inputPorts.size() < 2) {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - BitwiseOperator - Should Have Exactly 2 Input Ports", getSharedPointer()));
        }

        for(int i = 0; i<inputPorts.size(); i++){
            DataType inType = getInputPort(i)->getDataType();
            if(inType.isFloatingPt()){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - BitwiseOperator - Inputs Should Not be Floating Point", getSharedPointer()));
            }
        }
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - BitwiseOperator - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    if(getOutputPort(0)->getDataType().isFloatingPt()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - BitwiseOperator - Output Should Not be Floating Point", getSharedPointer()));
    }

}

CExpr BitwiseOperator::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    //TODO: Implement Vector Support
    if(!getOutputPort(0)->getDataType().isScalar()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("C Emit Error - BitwiseOperator Support for Vector Types has Not Yet Been Implemented" , getSharedPointer()));
    }

    //Get the expressions for each input
    std::vector<CExpr> inputExprs;

    unsigned long numInputPorts = inputPorts.size();
    for(unsigned long i = 0; i<numInputPorts; i++){
        std::shared_ptr<OutputPort> srcOutputPort = getInputPort(i)->getSrcOutputPort();
        int srcOutputPortNum = srcOutputPort->getPortNum();
        std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

        inputExprs.push_back(srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, imag));
    }

    std::string expr = "";
    if(op == BitwiseOp::NOT) {
        //This is a special case for a single operand
        expr = bitwiseOpToCString(BitwiseOp::NOT) + "(" + inputExprs[0].getExpr() + ")";
    }else{
        //For >=2 operands
        for(unsigned long i = 0; i<numInputPorts; i++) {
            if(i > 0){
                expr += bitwiseOpToCString(op);
            }
            expr += "(" + inputExprs[i].getExpr() + ")";
        }
    }

    //Already validated inputs are not floating pt
    //Find the type the expression will be promoted to
    //See https://en.cppreference.com/w/c/language/conversion
    //With types of the same width, unsigned wins and signed types are converted to unsigned
    //With types of different widths, if the signed type can represent the values of the unsigned type, the unsigned type
    //is converted to the signed type
    DataType largestType = getInputPort(0)->getDataType();
    for(int i = 1; i<numInputPorts; i++){
        DataType type = getInputPort(i)->getDataType();
        if(type.getTotalBits()>largestType.getTotalBits()){
            largestType = type;
        }else if(type.getTotalBits() == largestType.getTotalBits() && !type.isSignedType()){
            largestType = type;
        }
    }

    //Cast the expression to the correct output type (if necessary)
    std::string outputExpr = DataType::cConvertType(expr, largestType, getOutputPort(0)->getDataType());

    return CExpr(outputExpr, CExpr::ExprType::SCALAR_EXPR);
}

BitwiseOperator::BitwiseOperator(std::shared_ptr<SubSystem> parent, BitwiseOperator* orig) : PrimitiveNode(parent, orig), op(orig->op){

}

std::shared_ptr<Node> BitwiseOperator::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<BitwiseOperator>(parent, this);
}

BitwiseOperator::BitwiseOperator() : PrimitiveNode(), op(BitwiseOp::NOT){

}

BitwiseOperator::BitwiseOperator(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), op(BitwiseOp::NOT){

}
