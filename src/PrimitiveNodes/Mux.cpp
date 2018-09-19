//
// Created by Christopher Yarp on 7/16/18.
//

#include "Mux.h"
#include "GraphCore/NodeFactory.h"
#include <iostream>

Mux::Mux() : PrimitiveNode(), SelectNode(), booleanSelect(false) {

}

Mux::Mux(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), SelectNode(parent), booleanSelect(false) {

}

bool Mux::isBooleanSelect() const {
    return booleanSelect;
}

void Mux::setBooleanSelect(bool booleanSelect) {
    Mux::booleanSelect = booleanSelect;
}

std::shared_ptr<Mux>
Mux::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                       std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Check Valid Import ====
    if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Not supported for simulink import
        throw std::runtime_error("Mux import is not supported for the SIMULINK_EXPORT dialect");
    } else if (dialect == GraphMLDialect::VITIS) {
        //==== Create Node and set common properties ====
        std::shared_ptr<Mux> newNode = NodeFactory::createNode<Mux>(parent);
        newNode->setId(id);
        newNode->setName(name);

        return newNode;
    } else {
        throw std::runtime_error("Unsupported Dialect when parsing XML - Mux");
    }
}

std::set<GraphMLParameter> Mux::graphMLParameters() {
    return std::set<GraphMLParameter>(); //No parameters to return
}

xercesc::DOMElement *
Mux::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Mux");

    return thisNode;
}

std::string Mux::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: Mux";

    return label;
}

void Mux::validate() {
    Node::validate();

    selectorPort->validate();

    if(inputPorts.size() < 1){
        throw std::runtime_error("Validation Failed - Mux - Should Have 1 or More Input Ports");
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - Mux - Should Have Exactly 1 Output Port");
    }

    //Check that the select port is real
    if((*selectorPort->getArcs().begin())->getDataType().isComplex()){
        throw std::runtime_error("Validation Failed - Mux - Select Port Cannot be Complex");
    }

    //warn if floating point type
    //TODO: enforce integer for select (need to rectify Simulink use of double)
    if((*selectorPort->getArcs().begin())->getDataType().isFloatingPt()){
        std::cerr << "Warning: MUX Select Port is Driven by Floating Point" << std::endl;
    }

    //Check that all input ports and the output port have the same type
    DataType outType = getOutputPort(0)->getDataType();
    unsigned long numInputPorts = inputPorts.size();

    for(unsigned long i = 0; i<numInputPorts; i++){
        DataType inType = getInputPort(i)->getDataType();

        if(inType != outType){
            throw std::runtime_error("Validation Failed - Mux - DataType of Input Port Does not Match Output Port");
        }
    }

}

bool Mux::hasInternalFanout(int inputPort, bool imag) {
    return true;
}

CExpr Mux::emitCExpr(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag) {

    if(getOutputPort(0)->getDataType().getWidth()>1){
        throw std::runtime_error("C Emit Error - Mux Support for Vector Types has Not Yet Been Implemented");
    }

    //Get the expression for the select line
    std::shared_ptr<OutputPort> selectSrcOutputPort = getSelectorPort()->getSrcOutputPort();
    int selectSrcOutputPortNum = selectSrcOutputPort->getPortNum();
    std::shared_ptr<Node> selectSrcNode = selectSrcOutputPort->getParent();

    std::string selectExpr = selectSrcNode->emitC(cStatementQueue, selectSrcOutputPortNum, imag);

    //Get the expressions for each input
    std::vector<std::string> inputExprs;

    unsigned long numInputPorts = inputPorts.size();
    for(unsigned long i = 0; i<numInputPorts; i++){
        std::shared_ptr<OutputPort> srcOutputPort = getInputPort(i)->getSrcOutputPort();
        int srcOutputPortNum = srcOutputPort->getPortNum();
        std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

        inputExprs.push_back(srcNode->emitC(cStatementQueue, srcOutputPortNum, imag));
    }

    //Declare output tmp var
    std::string outputVarName = name+"_n"+std::to_string(id)+"_out";
    DataType outType = getOutputPort(0)->getDataType();
    Variable outVar = Variable(outputVarName, outType);

    //TODO: This function currently implements a seperate conditional for real and imag.  Merge them in a later version
    //to do this, need to pull both real and imagionary components of each input, construct both, and emit.
    //a state variable would be needed to determine that the mux had already been emitted when the other component's emit
    //call is made (ie. if real emit is called first, imag emit should simply return the output var).
    if(outType.isComplex()){
        cStatementQueue.push_back(outVar.getCVarDecl(true, true, false, true) + ";");
    }else{
        cStatementQueue.push_back(outVar.getCVarDecl(false, true, false, true) + ";");
    }

    DataType selectDataType = getSelectorPort()->getDataType();
    if(selectDataType.isBool()){
        //if/else statement
        std::string ifExpr = "if(" + selectExpr + "){";
        cStatementQueue.push_back(ifExpr);

        std::string trueAssign;
        std::string falseAssign;
        if(booleanSelect){
            //In this case, port 0 is the true port and port 1 is the false port
            trueAssign = outVar.getCVarName(imag) + " = " + inputExprs[0] + ";";
            falseAssign = outVar.getCVarName(imag) + " = " + inputExprs[1] + ";";
        }else{
            //This takes a select statement type perspective with the input ports being considered numbers
            //false is 0 and true is 1.
            trueAssign = outVar.getCVarName(imag) + " = " + inputExprs[1] + ";";
            falseAssign = outVar.getCVarName(imag) + " = " + inputExprs[0] + ";";
        }

        cStatementQueue.push_back(trueAssign);
        cStatementQueue.push_back("}else{");
        cStatementQueue.push_back(falseAssign);
        cStatementQueue.push_back("}");
    }else{
        //switch statement
        std::string switchExpr = "switch(" + selectExpr + "){";
        cStatementQueue.push_back(switchExpr);

        for(unsigned long i = 0; i<(numInputPorts-1); i++){
            std::string caseExpr = "case " + std::to_string(i) + ":";
            cStatementQueue.push_back(caseExpr);
            std::string caseAssign = outVar.getCVarName(imag) + " = " + inputExprs[i] + ";";
            cStatementQueue.push_back(caseAssign);
            cStatementQueue.push_back("break;");
        }

        cStatementQueue.push_back("default:");
        std::string caseAssign = outVar.getCVarName(imag) + " = " + inputExprs[numInputPorts-1] + ";";
        cStatementQueue.push_back(caseAssign);
        cStatementQueue.push_back("break;");
        cStatementQueue.push_back("}");
    }

    return CExpr(outVar.getCVarName(imag), true); //This is a variable

}

Mux::Mux(std::shared_ptr<SubSystem> parent, Mux* orig) : PrimitiveNode(parent, orig), booleanSelect(orig->booleanSelect){
    //The select port is not copied but a new one is created
    selectorPort = std::unique_ptr<SelectPort>(new SelectPort(this, 0)); //Don't need to do this in init as a raw pointer is passed to the port
}

std::shared_ptr<Node> Mux::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Mux>(parent, this);
}




