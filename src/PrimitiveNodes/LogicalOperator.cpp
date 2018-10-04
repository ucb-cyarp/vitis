//
// Created by Christopher Yarp on 9/11/18.
//

#include "LogicalOperator.h"
#include "GraphCore/NodeFactory.h"
#include "General/GeneralHelper.h"

LogicalOperator::LogicalOp LogicalOperator::getLogicalOp() const {
    return logicalOp;
}

void LogicalOperator::setLogicalOp(LogicalOperator::LogicalOp logicalOp) {
    LogicalOperator::logicalOp = logicalOp;
}

LogicalOperator::LogicalOp LogicalOperator::parseLogicalOpString(std::string str) {
    if(str == "NOT"){
        return LogicalOp::NOT;
    }else if(str == "AND"){
        return LogicalOp::AND;
    }else if(str == "OR"){
        return LogicalOp::OR;
    }else if(str == "NAND"){
        return LogicalOp::NAND;
    }else if(str == "NOR"){
        return LogicalOp::NOR;
    }else if(str == "XOR"){
        return LogicalOp::XOR;
    }else if(str == "NXOR"){
        return LogicalOp::NXOR;
    }else{
        throw std::runtime_error("Logical Operator Unsupported: " + str);
    }
}

std::string LogicalOperator::logicalOpToString(LogicalOperator::LogicalOp op) {
    if(op == LogicalOp::NOT){
        return "NOT";
    }else if(op == LogicalOp::AND){
        return "AND";
    }else if(op == LogicalOp::OR){
        return "OR";
    }else if(op == LogicalOp::NAND){
        return "NAND";
    }else if(op == LogicalOp::NOR){
        return "NOR";
    }else if(op == LogicalOp::XOR){
        return "XOR";
    }else if(op == LogicalOp::NXOR){
        return "NXOR";
    }else{
        throw std::runtime_error("LogicalOp toString not implemented for this operator.");
    }
}

std::string LogicalOperator::logicalOpToCString(LogicalOperator::LogicalOp op) {
    if(op == LogicalOp::NOT){
        return "!";
    }else if(op == LogicalOp::AND){
        return "&&";
    }else if(op == LogicalOp::OR){
        return "||";
    }else if(op == LogicalOp::NAND){
        return "&&";
    }else if(op == LogicalOp::NOR){
        return "||";
    }else if(op == LogicalOp::XOR){
        return "==";
    }else if(op == LogicalOp::NXOR){
        return "==";
    }else{
        throw std::runtime_error("LogicalOp toCString not implemented for this operator.");
    }
}

bool LogicalOperator::logicalOpIsNegating(LogicalOp op){
    return op==LogicalOp::NOT || op==LogicalOp::NAND || op==LogicalOp::NOR || op==LogicalOp::NXOR;
}

LogicalOperator::LogicalOperator() : logicalOp(LogicalOp::NOT) {

}

LogicalOperator::LogicalOperator(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), logicalOp(LogicalOp::NOT) {

}

std::shared_ptr<LogicalOperator>
LogicalOperator::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                 std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<LogicalOperator> newNode = NodeFactory::createNode<LogicalOperator>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important property ====
    std::string logicalOpStr;

    if(dialect == GraphMLDialect::VITIS){
        //Vitis Name (LogicalOperator) -- LogicalOp
        logicalOpStr = dataKeyValueMap.at("LogicalOp");
    } else if(dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Name (Logic) -- Operator
        logicalOpStr = dataKeyValueMap.at("Operator");
    } else
    {
        throw std::runtime_error("Unsupported Dialect when parsing XML - LogicalOperator");
    }

    //Get Operator
    LogicalOp parsedLogicalOp = parseLogicalOpString(logicalOpStr);


    newNode->setLogicalOp(parsedLogicalOp);

    return newNode;
}

std::set<GraphMLParameter> LogicalOperator::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("LogicalOp", "string", true));

    return parameters;
}

xercesc::DOMElement *
LogicalOperator::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "LogicalOperator");

    GraphMLHelper::addDataNode(doc, thisNode, "LogicalOp", logicalOpToString(logicalOp));

    return thisNode;
}

std::string LogicalOperator::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: LogicalOperator\nLogicalOp:" + logicalOpToString(logicalOp);

    return label;
}

void LogicalOperator::validate() {
    Node::validate();

    if(logicalOp == LogicalOp::NOT) {
        if (inputPorts.size() != 1) {
            throw std::runtime_error("Validation Failed - LogicalOperator - Should Have Exactly 1 Input Ports");
        }
    }else{
        if (inputPorts.size() < 2) {
            throw std::runtime_error("Validation Failed - LogicalOperator - Should Have At Least 2 Input Ports");
        }
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - LogicalOperator - Should Have Exactly 1 Output Port");
    }

    //Validate ports are bools
    //TODO: Check if this to too restrictive (ie. rely on auto convert to bool)
    unsigned long numInputPorts = inputPorts.size();
    for(unsigned long i = 0; i<numInputPorts; i++) {
        if (!(getInputPort(i)->getDataType().isBool())) {
            throw std::runtime_error("Validation Failed - LogicalOperator - Input " + GeneralHelper::to_string(i) + " is not a bool");
        }
    }

    if(!(getOutputPort(0)->getDataType().isBool())){
        throw std::runtime_error("Validation Failed - LogicalOperator - Output is not a bool");
    }
}

CExpr LogicalOperator::emitCExpr(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag) {
    //TODO: Implement Vector Support
    if(getOutputPort(0)->getDataType().getWidth()>1){
        throw std::runtime_error("C Emit Error - Sum Support for Vector Types has Not Yet Been Implemented");
    }

    //Get the expressions for each input
    std::vector<std::string> inputExprs;

    unsigned long numInputPorts = inputPorts.size();
    for(unsigned long i = 0; i<numInputPorts; i++){
        std::shared_ptr<OutputPort> srcOutputPort = getInputPort(i)->getSrcOutputPort();
        int srcOutputPortNum = srcOutputPort->getPortNum();
        std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

        inputExprs.push_back(srcNode->emitC(cStatementQueue, srcOutputPortNum, imag));
    }

    std::string expr = "";
    if(logicalOp == LogicalOp::NOT){
        //This is a special case for a single operand
        expr = logicalOpToCString(LogicalOp::NOT) + "(" + inputExprs[0] + ")";
    }else{
        //For multiple operands
        std::string operandStr = logicalOpToCString(logicalOp);

        //For XOR and NXOR, explicitly convert operands to bool
        //Emit the 1st operand
        if(logicalOp == LogicalOp::XOR || logicalOp == LogicalOp::NXOR) {
            expr = "((bool)("+inputExprs[0]+"))";
        }else{
            expr = "("+inputExprs[0]+")";
        }

        //Emit the subsequent operands and operators
        for(unsigned long i=1; i<numInputPorts; i++){
            expr += operandStr; //Get Operand

            //For XOR and NXOR, explicitly convert operands to bool
            if(logicalOp == LogicalOp::XOR || logicalOp == LogicalOp::NXOR) {
                expr += "((bool)("+inputExprs[i]+"))";
            }else{
                expr += "("+inputExprs[i]+")";
            }
        }

        if(logicalOpIsNegating(logicalOp)){
            //We need to negate the expression
            expr = logicalOpToCString(LogicalOp::NOT) + "(" + expr + ")";
        }
    }

    return CExpr(expr, false);
}

LogicalOperator::LogicalOperator(std::shared_ptr<SubSystem> parent, LogicalOperator* orig) : PrimitiveNode(parent, orig), logicalOp(orig->logicalOp){

}

std::shared_ptr<Node> LogicalOperator::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<LogicalOperator>(parent, this);
}
