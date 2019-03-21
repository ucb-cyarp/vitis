//
// Created by Christopher Yarp on 7/17/18.
//

#include "Compare.h"
#include "GraphCore/NodeFactory.h"
#include "General/ErrorHelpers.h"

Compare::Compare() : compareOp(CompareOp::LT) {

}

Compare::Compare(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), compareOp(CompareOp::LT) {

}

Compare::CompareOp Compare::parseCompareOpString(std::string str) {
    if(str == "<"){
        return CompareOp::LT;
    }else if(str == "<="){
        return CompareOp::LEQ;
    }else if(str == ">"){
        return CompareOp::GT;
    }else if(str == ">="){
        return CompareOp::GEQ;
    }else if(str == "=="){
        return CompareOp::EQ;
    }else if(str == "!="){
        return CompareOp::NEQ;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Compare Operator Unsupported: " + str));
    }
}

std::string Compare::compareOpToString(Compare::CompareOp op) {
    if(op == CompareOp::LT){
        return "<";
    }else if(op == CompareOp::LEQ){
        return "<=";
    }else if(op == CompareOp::GT){
        return ">";
    }else if(op == CompareOp::GEQ){
        return ">=";
    }else if(op == CompareOp::EQ){
        return "==";
    }else if(op == CompareOp::NEQ){
        return "!=";
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("CompareOP toString not implemented for this operator."));
    }
}

std::string Compare::compareOpToCString(Compare::CompareOp op) {
    //The compareOpToString is already in the C style
    return compareOpToString(op);
}

Compare::CompareOp Compare::getCompareOp() const {
    return compareOp;
}

void Compare::setCompareOp(Compare::CompareOp compareOp) {
    Compare::compareOp = compareOp;
}

std::shared_ptr<Compare>
Compare::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                           std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<Compare> newNode = NodeFactory::createNode<Compare>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important property ====
    std::string compareOpStr;

    if(dialect == GraphMLDialect::VITIS){
        //Vitis Name (Compare) -- CompareOp
        compareOpStr = dataKeyValueMap.at("CompareOp");
    } else if(dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Name (RelationalOperator) -- Operator
        compareOpStr = dataKeyValueMap.at("Operator");
    } else
    {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Compare", newNode));
    }

    //Get Operator
    CompareOp parsedCompareOp = parseCompareOpString(compareOpStr);


    newNode->setCompareOp(parsedCompareOp);

    return newNode;
}

std::set<GraphMLParameter> Compare::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("CompareOp", "string", true));

    return parameters;
}

xercesc::DOMElement *
Compare::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Compare");

    GraphMLHelper::addDataNode(doc, thisNode, "CompareOp", compareOpToString(compareOp));

    return thisNode;
}

std::string Compare::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: Compare\nCompareOp:" + compareOpToString(compareOp);

    return label;
}

void Compare::validate() {
    Node::validate();

    if(inputPorts.size() != 2){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Compare - Should Have Exactly 2 Input Ports", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Compare - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    //Validate output port is bool

    if(!(getOutputPort(0)->getDataType().isBool())){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Compare - Output is not a bool", getSharedPointer()));
    }

    if(getInputPort(0)->getDataType().isComplex()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Compare - Currently Do Not Support Comparision of Complex Inputs", getSharedPointer()));
    }

    if(getInputPort(1)->getDataType().isComplex()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Compare - Currently Do Not Support Comparision of Complex Inputs", getSharedPointer()));
    }
}

CExpr Compare::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    //TODO: Implement Vector Support
    if(getInputPort(0)->getDataType().getWidth()>1 || getInputPort(1)->getDataType().getWidth()>1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("C Emit Error - Compare Support for Vector Types has Not Yet Been Implemented", getSharedPointer()));
    }

    //Get the expressions for each input
    std::vector<std::string> inputExprs;

    unsigned long numInputPorts = inputPorts.size();
    for(unsigned long i = 0; i<numInputPorts; i++){
        std::shared_ptr<OutputPort> srcOutputPort = getInputPort(i)->getSrcOutputPort();
        int srcOutputPortNum = srcOutputPort->getPortNum();
        std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

        inputExprs.push_back(srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, imag));
    }

    //Will rely on automatic type promoition in C when performing comparisions unless fixed point type
    //TODO: Implement Fixed point support
    bool foundFixedPt = false;

    for(unsigned long i = 0; i<numInputPorts; i++){
        DataType portDataType = getInputPort(i)->getDataType();
        if(portDataType.getFractionalBits() != 0){
            foundFixedPt = true;
        }
    }

    if(!foundFixedPt){
        //Relying on C automatic type promotion for non fixed point types

        std::string expr = "(" + inputExprs[0] + ") " + compareOpToCString(compareOp) + " (" + inputExprs[1] + ")";

        return CExpr(expr, false);
    }
    else{
        //TODO: Finish
        throw std::runtime_error(ErrorHelpers::genErrorStr("C Emit Error - Fixed Point Not Yet Implemented for Compare", getSharedPointer()));
    }

    return CExpr("", false);
}

Compare::Compare(std::shared_ptr<SubSystem> parent, Compare* orig) : PrimitiveNode(parent, orig), compareOp(orig->compareOp) {

}

std::shared_ptr<Node> Compare::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Compare>(parent, this);
}
