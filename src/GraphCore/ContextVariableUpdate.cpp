//
// Created by Christopher Yarp on 2018-12-17.
//

#include "ContextVariableUpdate.h"
#include "General/GeneralHelper.h"
#include "General/EmitterHelpers.h"
#include "General/ErrorHelpers.h"
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
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("ContextRoot", "string", true));

    return parameters;
}

xercesc::DOMElement *
ContextVariableUpdate::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "ContextVaribleUpdate");

    std::shared_ptr<Node> asNode = GeneralHelper::isType<ContextRoot, Node>(contextRoot); //ContextRoot should also be node TODO: fix diamond inherit
    if(asNode == nullptr){
        throw std::runtime_error("Could not cast ContextRoot as Node");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "ContextRoot", asNode->getFullGraphMLPath());

    return thisNode;
}

bool ContextVariableUpdate::canExpand() {
    return false;
}

void ContextVariableUpdate::validate() {
    Node::validate();

    if(contextRoot == nullptr){
        throw std::runtime_error(ErrorHelpers::genErrorStr("A ContextVariableUpdate node has no contextRoot node", getSharedPointer()));
    }

    if(contextVarIndex < contextRoot->getNumSubContexts()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("SubContext " + GeneralHelper::to_string(contextVarIndex) + " requested by ContextVariableUpdate does not exist in context with " + GeneralHelper::to_string(contextRoot->getNumSubContexts()) + " subcontexts", getSharedPointer()));
    }

    if(inputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("ContextVariableUpdate must have exactly 1 input port", getSharedPointer()));
    }

    if(inputPorts.size() != outputPorts.size()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("ContextVariableUpdate must have an equal number of input and output ports", getSharedPointer()));
    }

    //Check that the datatypes of the input and output port match
    if(getInputPort(0)->getDataType() != getOutputPort(0)->getDataType()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("ContextVariableUpdate input and output must have the same type", getSharedPointer()));
    }

    //Check that the input and output datatype are the same as the context variable
    if(getInputPort(0)->getDataType() != contextRoot->getCContextVar(contextVarIndex).getDataType()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("ContextVariableUpdate input and ContextVariable must have the same type", getSharedPointer()));
    }

}

std::shared_ptr<Node> ContextVariableUpdate::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<ContextVariableUpdate>(parent, this);
}

CExpr ContextVariableUpdate::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    //TODO: Currently, circular buffers are not supported for Context Variables because of the lack of a callback
    //      to the ContextRoot on how to handle them (ie. is the buffer overwritten, write to front, write to back?)

    DataType inputDataType = getInputPort(0)->getDataType();
    std::shared_ptr<OutputPort> srcPort = getInputPort(0)->getSrcOutputPort();
    int srcOutPortNum = srcPort->getPortNum();
    std::shared_ptr<Node> srcNode = srcPort->getParent();

    //--- Emit the upstream ---
    CExpr inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, imag);

    //Used to create a temporary variable to store the input to this node.  The idea was that this would allow the output
    //to be used inside the context without relying on the underlying context variable.  However, this is superflous as this node
    //sets the context variable before emitting anything for its output.  Because of this, the state variable can be returned as the output.

    //There are some corner cases where a temporary would be needed.  These include when a node other than the corresponding ContextRoot
    //is at the output.  If this occurs, the state variable could concievable be updated again by a downstream ContextVariableUpdate node.
    //Another case is if the scheduler is not context aware.  In this case, the update of the context variable occurs elsewhere and this
    //block should act as a passthrough.  This should be rare as there really isn't any point in these nodes existing unless the scheduler is
    //context aware, but is an easy enough case to handle.

    bool emitTemporary = false;
    std::set<std::shared_ptr<Arc>> outArcs = getOutputPort(0)->getArcs();
    for(auto arc : outArcs){
        std::shared_ptr<Node> contextRootAsNode = GeneralHelper::isType<ContextRoot, Node>(contextRoot);
        if(contextRoot != nullptr && contextRootAsNode == nullptr){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Found a ContextRoot that is node a node while processing ContextVariableUpdateNode", getSharedPointer()));
        }
        if(arc->getDstPort()->getParent() != contextRootAsNode){
            emitTemporary = true;
        }
    }
    if(!SchedParams::isContextAware(schedType)){
        emitTemporary = true;
    }

    DataType datatype = getInputPort(0)->getDataType();

    //--- Declare temporary outside for loop ---
    Variable stateInputVar;
    if(emitTemporary){
        std::string stateInputName = name + "_n" + GeneralHelper::to_string(id) + "_contextVar" + GeneralHelper::to_string(contextVarIndex) + "Input";
        //Note that if the input is a vector/matrix, the  temporary variable will be a
        stateInputVar = Variable(stateInputName, datatype);
        //Emit the variable declaration seperatly from the assignment incase we need to loop
        std::string inputDecl = stateInputVar.getCVarDecl(imag, true, false, true) + ";";
        cStatementQueue.push_back(inputDecl);
    }

    //If matrix/vector, need to emit for loop
    std::vector<std::string> forLoopIndexVars;
    std::vector<std::string> forLoopClose;
    if(!datatype.isScalar()){
        //Create nested loops for a given array
        std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                EmitterHelpers::generateVectorMatrixForLoops(datatype.getDimensions());

        std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
        forLoopIndexVars = std::get<1>(forLoopStrs);
        forLoopClose = std::get<2>(forLoopStrs);

        cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());
    }

    Variable contextVariable = contextRoot->getCContextVar(contextVarIndex);

    std::string termToAssign;
    //If not a matrix, forLoopIndexVars and deref are effectivly ignored.  A check is made to make sure forLoopIndexVars is empty
    std::vector<std::string> emptyArr;
    std::string inputExprDeref = inputExpr.getExprIndexed(datatype.isScalar() ? emptyArr : forLoopIndexVars, true);
    if(emitTemporary) {
        //--- Assign the expr to a temporary variable ---
        //Dereference is nessisary
        std::string stateInputVarDeref = stateInputVar.getCVarName(imag) + (datatype.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars));
        
        std::string inputAssign = stateInputVarDeref + " = " + inputExprDeref + ";";
        cStatementQueue.push_back(inputAssign);

        termToAssign = stateInputVarDeref;
    }else{
        termToAssign = inputExprDeref;
    }

    //--- Assign to ContextVariable ---
    std::string contextVariableAssign = contextVariable.getCVarName(imag) +
            (datatype.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars)) +
            " = " + termToAssign + ";";
    cStatementQueue.push_back(contextVariableAssign);

    // --- Close For Loop ---
    if(!datatype.isScalar()){
        cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
    }

    if(emitTemporary) {
        //Return the temporary var
        //TODO: Currently, circular buffers are not supported for Context Variables
        return CExpr(stateInputVar.getCVarName(imag), datatype.isScalar() ? CExpr::ExprType::SCALAR_VAR : CExpr::ExprType::ARRAY);
    }else{
        //Return the state var
        //TODO: Currently, circular buffers are not supported for Context Variables
        return CExpr(contextVariable.getCVarName(imag), datatype.isScalar() ? CExpr::ExprType::SCALAR_VAR : CExpr::ExprType::ARRAY);
    }
}

std::string ContextVariableUpdate::typeNameStr(){
    return "ContextVariableUpdate";
}

std::string ContextVariableUpdate::labelStr(){
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr();

    return label;
}