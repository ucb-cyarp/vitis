//
// Created by Christopher Yarp on 2018-12-17.
//

#include "ContextVariableUpdate.h"
#include "General/GeneralHelper.h"
#include "SubSystem.h"
#include "ContextRoot.h"
#include "NodeFactory.h"

ContextVariableUpdate::ContextVariableUpdate() : contextRoot(nullptr) {

}

ContextVariableUpdate::ContextVariableUpdate(std::shared_ptr<SubSystem> parent) : Node(parent), contextRoot(nullptr), contextVarIndex(0) {

}

ContextVariableUpdate::ContextVariableUpdate(std::shared_ptr<SubSystem> parent, ContextVariableUpdate *orig) : Node(parent, orig), contextRoot(nullptr), contextVarIndex(0) {

}

std::shared_ptr<ContextRoot> ContextVariableUpdate::getContextRoot() const {
    return contextRoot;
}

void ContextVariableUpdate::setContextRoot(const std::shared_ptr<ContextRoot> &contextRoot) {
    ContextVariableUpdate::contextRoot = contextRoot;
}

int ContextVariableUpdate::getContextVarIndex() const {
    return contextVarIndex;
}

void ContextVariableUpdate::setContextVarIndex(int contextVarIndex) {
    ContextVariableUpdate::contextVarIndex = contextVarIndex;
}

std::set<GraphMLParameter> ContextVariableUpdate::graphMLParameters() {
    return Node::graphMLParameters();
}

xercesc::DOMElement *
ContextVariableUpdate::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    throw std::runtime_error("XML Emit for ContextVariableUpdate not yet implemented");
}

bool ContextVariableUpdate::canExpand() {
    return false;
}

void ContextVariableUpdate::validate() {
    Node::validate();

    if(contextRoot == nullptr){
        throw std::runtime_error("A ContextVariableUpdate node has no contextRoot node");
    }

    if(contextVarIndex < contextRoot->getNumSubContexts()){
        throw std::runtime_error("SubContext " + GeneralHelper::to_string(contextVarIndex) + " requested by ContextVariableUpdate does not exist in context with " + GeneralHelper::to_string(contextRoot->getNumSubContexts()) + " subcontexts");
    }

    if(inputPorts.size() > 1){
        throw std::runtime_error("ContextVariableUpdate can have either 0 or 1 input ports");
    }

    if(inputPorts.size() != outputPorts.size()){
        throw std::runtime_error("ContextVariableUpdate must have an equal number of input and output ports");
    }
}

std::shared_ptr<Node> ContextVariableUpdate::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<ContextVariableUpdate>(parent, this);
}

CExpr ContextVariableUpdate::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    DataType inputDataType = getInputPort(0)->getDataType();
    std::shared_ptr<OutputPort> srcPort = getInputPort(0)->getSrcOutputPort();
    int srcOutPortNum = srcPort->getPortNum();
    std::shared_ptr<Node> srcNode = srcPort->getParent();

    std::string stateInputName = name + "_n" + GeneralHelper::to_string(id) + "_contextVar" + GeneralHelper::to_string(contextVarIndex) + "Input";
    Variable stateInputVar = Variable(stateInputName, getInputPort(0)->getDataType());

    //Emit the upstream
    std::string inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, imag);

    //Assign the expr to a temporary variable

    //TODO: Implement Vector Support (need to loop over input variable indexes (will be stored as a variable due to defualt behavior of internal fanoud_

    std::string inputDeclAssign = stateInputVar.getCVarDecl(imag, false, false, false) + " = " + inputExpr + ";";
    cStatementQueue.push_back(inputDeclAssign);

    //Assign to ContextVariable
    Variable contextVariable = contextRoot->getCContextVar(contextVarIndex);

    std::string contextVariableAssign = contextVariable.getCVarName(imag) + " = " + stateInputVar.getCVarName(imag) + ";";
    cStatementQueue.push_back(contextVariableAssign);

    //Return the temporary var
    return CExpr(stateInputVar.getCVarName(imag), true);
}
