//
// Created by Christopher Yarp on 10/25/18.
//

#include "StateUpdate.h"
#include "General/EmitterHelpers.h"

std::shared_ptr<Node> StateUpdate::getPrimaryNode() const {
    return primaryNode;
}

void StateUpdate::setPrimaryNode(const std::shared_ptr<Node> &primaryNode) {
    StateUpdate::primaryNode = primaryNode;
}

StateUpdate::StateUpdate() : primaryNode(nullptr){

}

StateUpdate::StateUpdate(std::shared_ptr<SubSystem> parent) : Node(parent), primaryNode(nullptr) {

}

StateUpdate::StateUpdate(std::shared_ptr<SubSystem> parent, StateUpdate *orig) : Node(parent, orig) {

}

void StateUpdate::validate() {
    Node::validate();

    if(primaryNode == nullptr){
        throw std::runtime_error("A StateUpdate node has no primary node");
    }

    if(!primaryNode->hasState()){
        throw std::runtime_error("A StateUpdate node has a primary node which does not have state");
    }

    if(inputPorts.size() > 1){
        throw std::runtime_error("StateUpdate can have either 0 or 1 input ports");
        //This is because the
    }

    if(inputPorts.size() != outputPorts.size()){
        throw std::runtime_error("StateUpdate must have an equal number of input and output ports");
    }

    if(inputPorts.size() == 1){
        if(getInputPort(0)->getDataType() != getOutputPort(0)->getDataType()){
            throw std::runtime_error("StateUpdate input and output ports must have the same type");
        }
    }
}

std::shared_ptr<Node> StateUpdate::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<StateUpdate>(parent, this);
}

void StateUpdate::emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, std::shared_ptr<StateUpdate> stateUpdateSrc) {
    std::shared_ptr<StateUpdate> thisAsStateUpdate = std::static_pointer_cast<StateUpdate>(getSharedPointer());
    primaryNode->emitCStateUpdate(cStatementQueue, schedType, thisAsStateUpdate);
}

std::set<GraphMLParameter> StateUpdate::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("PrimaryNode", "string", true));

    return parameters;
}

xercesc::DOMElement *
StateUpdate::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "StateUpdate");

    GraphMLHelper::addDataNode(doc, thisNode, "PrimaryNode", primaryNode->getFullGraphMLPath());

    return thisNode;
}

bool StateUpdate::canExpand() {
    return false;
}

CExpr StateUpdate::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    //Get the expressions for the input
    std::string inputExpr;

    std::shared_ptr<OutputPort> srcOutputPort = getInputPort(outputPortNum)->getSrcOutputPort();
    int srcOutputPortNum = srcOutputPort->getPortNum();
    std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

    inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, imag);

    DataType datatype = getInputPort(outputPortNum)->getDataType();

    if(!datatype.isScalar()) {
        //If a vector/matrix, create a temporary and copy the values into it.  Cannot rely on the emitter to create a variable for it
        //A temporary is made to address some corner cases, like when the input is a delay, where the value of the expression could
        //change later if the state was updated.
        std::string vecOutName = name+"_n"+GeneralHelper::to_string(id)+ "_outVec"; //Changed to
        Variable vecOutVar = Variable(vecOutName, datatype);
        cStatementQueue.push_back(vecOutVar.getCVarDecl(imag, true, false, true, false) + ";");

        //Create nested loops for a given array
        std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                EmitterHelpers::generateVectorMatrixForLoops(datatype.getDimensions());
        std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
        std::vector<std::string> forLoopIndexVars = std::get<1>(forLoopStrs);
        std::vector<std::string> forLoopClose = std::get<2>(forLoopStrs);

        cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());

        //Dereference
        std::string inputExprDeref = inputExpr + EmitterHelpers::generateIndexOperation(forLoopIndexVars);

        //Copy to temporary variable
        std::string tmpAssignment = vecOutVar.getCVarName(imag) + EmitterHelpers::generateIndexOperation(forLoopIndexVars) + " = " + inputExprDeref + ";";
        cStatementQueue.push_back(tmpAssignment);

        //Close for loop
        cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());

        //return temporary as a variable
        return CExpr(vecOutVar.getCVarName(imag), true);
    }else{
        //If a scalar, just pass on the expression.  A variable will be created for it by the emitter
        return CExpr(inputExpr, false);
    }
}

std::string StateUpdate::typeNameStr(){
    return "StateUpdate";
}

std::string StateUpdate::labelStr(){
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr();

    return label;
}