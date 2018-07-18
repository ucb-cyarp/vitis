//
// Created by Christopher Yarp on 7/17/18.
//

#include "Compare.h"

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
        throw std::runtime_error("Compare Operator Unsupported: " + str);
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
        throw std::runtime_error("CompareOP toString not implemented for this operator.");
    }
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
        throw std::runtime_error("Unsupported Dialect when parsing XML - Compare");
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
        throw std::runtime_error("Validation Failed - Compare - Should Have Exactly 2 Input Ports");
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - Compare - Should Have Exactly 1 Output Port");
    }
}
