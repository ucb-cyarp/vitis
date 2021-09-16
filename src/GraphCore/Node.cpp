//
// Created by Christopher Yarp on 6/25/18.
//

#include <vector>
#include <memory>
#include <string>
#include <map>
#include <algorithm>
#include <iostream>

#include "SubSystem.h"
#include "Port.h"
#include "Node.h"
#include "InputPort.h"
#include "OutputPort.h"
#include "NodeFactory.h"
#include "Context.h"

#include "GraphMLTools/GraphMLHelper.h"
#include "General/GeneralHelper.h"
#include "General/ErrorHelpers.h"
#include "MasterNodes/MasterUnconnected.h"
#include "Blocking/BlockingDomain.h"
#include "Blocking/BlockingInput.h"
#include "Blocking/BlockingOutput.h"
#include "Blocking/BlockingHelpers.h"

Node::Node() : id(-1), name(""), partitionNum(-1), schedOrder(-1), tmpCount(0)
{
    parent = std::shared_ptr<SubSystem>(nullptr);

    //NOTE: CANNOT init ports here since we need a shared pointer to this object
}

Node::Node(std::shared_ptr<SubSystem> parent) : id(-1), name(""), partitionNum(-1), schedOrder(-1), tmpCount(0), parent(parent) { }

void Node::init() {
    //Init the order constraint ports.  It is ok for them to have no connected arcs
    orderConstraintInputPort = std::unique_ptr<OrderConstraintInputPort>(new OrderConstraintInputPort(this));
    orderConstraintOutputPort = std::unique_ptr<OrderConstraintOutputPort>(new OrderConstraintOutputPort(this));
}

void Node::addInArcUpdatePrevUpdateArc(int portNum, std::shared_ptr<Arc> arc) {
    std::shared_ptr<InputPort> inputPort = getInputPortCreateIfNot(portNum);

    //Set the dst port of the arc, updating the previous port and this one
    arc->setDstPortUpdateNewUpdatePrev(inputPort->getSharedPointerInputPort());
}

void Node::addOutArcUpdatePrevUpdateArc(int portNum, std::shared_ptr<Arc> arc) {
    std::shared_ptr<OutputPort> outputPort = getOutputPortCreateIfNot(portNum);

    //Set the src port of the arc, updating the previous port and this one
    arc->setSrcPortUpdateNewUpdatePrev(outputPort->getSharedPointerOutputPort());
}

std::shared_ptr<InputPort> Node::getInputPortCreateIfNot(int portNum) {
    //Create the requested port if it does not exist yet
    //TODO: it is assumed that if port n exists, that there are ports 0-n with no holes.  Re-evaluate this assumption
    unsigned long inputPortLen = inputPorts.size();
    for(unsigned long i = inputPortLen; i <= portNum; i++)
    {
        inputPorts.push_back(std::unique_ptr<InputPort>(new InputPort(this, i)));
    }

    return inputPorts[portNum]->getSharedPointerInputPort();
}

std::shared_ptr<OutputPort> Node::getOutputPortCreateIfNot(int portNum) {
    //Create the requested port if it does not exist yet
    //TODO: it is assumed that if port n exists, that there are ports 0-n with no holes.  Re-evaluate this assumption
    unsigned long outputPortLen = outputPorts.size();
    for (unsigned long i = outputPortLen; i <= portNum; i++) {
        outputPorts.push_back(std::unique_ptr<OutputPort>(new OutputPort(this, i)));
    }

    return outputPorts[portNum]->getSharedPointerOutputPort();
}


std::shared_ptr<Node> Node::getSharedPointer() {
    return shared_from_this();
}

void Node::removeInArc(std::shared_ptr<Arc> arc) {
    unsigned long inputPortLen = inputPorts.size();

    for(unsigned long i = 0; i<inputPortLen; i++)
    {
        inputPorts[i]->removeArc(arc);
    }
}

void Node::removeOutArc(std::shared_ptr<Arc> arc) {
    unsigned long outputPortLen = outputPorts.size();

    for(unsigned long i = 0; i<outputPortLen; i++)
    {
        outputPorts[i]->removeArc(arc);
    }
}

std::shared_ptr<InputPort> Node::getInputPort(int portNum) {
    if(portNum >= inputPorts.size()) {
        return std::shared_ptr<InputPort>(nullptr);
    }

    return inputPorts[portNum]->getSharedPointerInputPort();
}

std::shared_ptr<OutputPort> Node::getOutputPort(int portNum) {
    if(portNum >= outputPorts.size()) {
        return std::shared_ptr<OutputPort>(nullptr);
    }

    return outputPorts[portNum]->getSharedPointerOutputPort();
}

std::vector<std::shared_ptr<InputPort>> Node::getInputPorts() {
    std::vector<std::shared_ptr<InputPort>> inputPortPtrs;

    unsigned long inputPortLen = inputPorts.size();
    for(unsigned long i = 0; i<inputPortLen; i++)
    {
        inputPortPtrs.push_back(inputPorts[i]->getSharedPointerInputPort());
    }

    return inputPortPtrs;
}

std::vector<std::shared_ptr<OutputPort>> Node::getOutputPorts() {
    std::vector<std::shared_ptr<OutputPort>> outputPortPtrs;

    unsigned long outputPortLen = outputPorts.size();
    for (unsigned long i = 0; i < outputPortLen; i++)
    {
        outputPortPtrs.push_back(outputPorts[i]->getSharedPointerOutputPort());
    }

    return outputPortPtrs;
}

std::vector<std::shared_ptr<OutputPort>> Node::getOutputPortsIncludingOrderConstraint() {
    std::vector<std::shared_ptr<OutputPort>> outputPortPtrs = getOutputPorts();

    outputPortPtrs.push_back(orderConstraintOutputPort->getSharedPointerOrderConstraintPort());

    return outputPortPtrs;
}

std::string Node::getFullGraphMLPath() {
    std::string path = "n" + GeneralHelper::to_string(id);

    for(std::shared_ptr<SubSystem> cursor = parent; cursor != nullptr; cursor = cursor->getParent())
    {
        path = "n" + GeneralHelper::to_string(cursor->getId()) + "::" + path;
    }

    return path;
}

int Node::getId() const {
    return id;
}

void Node::setId(int id) {
    Node::id = id;
}

const std::string &Node::getName() const {
    return name;
}

void Node::setName(const std::string &name) {
    Node::name = name;
}

int Node::getIDFromGraphMLFullPath(std::string fullPath)
{
    //Find the location of the last n
    int nIndex = -1;

    int pathLen = (int) fullPath.size(); //Casting to signed to ensure loop termination
    for(int i = pathLen-1; i >= 0; i--){
        if(fullPath[i] == 'n'){
            nIndex = i;
            break;
        }
    }

    if(nIndex == -1){
        throw std::runtime_error("Could not find node ID in full GraphML path: " + fullPath);
    }

    std::string localIDStr = fullPath.substr(nIndex+1, std::string::npos);

    int localId = std::stoi(localIDStr);

    return localId;
}

//Default behavior is to return an empty set
std::set<GraphMLParameter> Node::graphMLParameters() {
    return std::set<GraphMLParameter>();
}


xercesc::DOMElement *Node::emitGraphMLBasics(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode) {
    //Used CreateDOMDocument.cpp example from Xerces as a guide

    //Will not insert newlines under the assumption that format-pretty-print will be enabled in the serializer

    xercesc::DOMElement* nodeElement = GraphMLHelper::createNode(doc, "node");
    GraphMLHelper::setAttribute(nodeElement, "id", getFullGraphMLPath());
    GraphMLHelper::addDataNode(doc, nodeElement, "node_id", GeneralHelper::to_string(getId()));

    if(!name.empty()){
        GraphMLHelper::addDataNode(doc, nodeElement, "instance_name", name);
    }

    std::string label = labelStr();
    if(!label.empty()){
        GraphMLHelper::addDataNode(doc, nodeElement, "block_label", label);
    }

    GraphMLHelper::addDataNode(doc, nodeElement, "block_partition_num", GeneralHelper::to_string(partitionNum));
    GraphMLHelper::addDataNode(doc, nodeElement, "block_sched_order", GeneralHelper::to_string(schedOrder));
    GraphMLHelper::addDataNode(doc, nodeElement, "orig_location", getFullyQualifiedOrigName());

    //Add to graph node
    graphNode->appendChild(nodeElement);

    return nodeElement;
}

std::string Node::labelStr() {
    std::string label = "";

    if(!name.empty()){
        label += name + "\n";
    }

    label +=  "ID: " + getFullGraphMLPath();
    label +=  "\nPartition: " + GeneralHelper::to_string(partitionNum);
    label +=  "\nSched: " + GeneralHelper::to_string(schedOrder);

    return label;
}

void Node::validate() {
    //Check each port
    for(auto port = inputPorts.begin(); port != inputPorts.end(); port++){
        (*port)->validate();
    }

    for(auto port = outputPorts.begin(); port != outputPorts.end(); port++){
        (*port)->validate();
    }

    if(orderConstraintInputPort){
        orderConstraintInputPort->validate();
    }

    if(orderConstraintOutputPort){
        orderConstraintOutputPort->validate();
    }
}

void Node::propagateProperties() {
    //Default behavior will do nothing
}

void Node::setParent(std::shared_ptr<SubSystem> parent) {
    Node::parent = parent;
}

//Default behavior is to not do any expansion and to return false.
std::shared_ptr<ExpandedNode> Node::expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                           std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                           std::shared_ptr<MasterUnconnected> &unconnected_master) {
    return nullptr;
}

std::string Node::getFullyQualifiedName(bool sanitize, std::string delim) {
    std::string fullName;

    if(sanitize) {
        fullName = GeneralHelper::replaceAll(name, '\n', ' ');
    }else{
        fullName = name;
    }

    for(std::shared_ptr<SubSystem> parentPtr = parent; parentPtr != nullptr; parentPtr = parentPtr->parent)
    {
        std::string componentName = parentPtr->getName();

        if(sanitize){
            componentName = GeneralHelper::replaceAll(componentName, '\n', ' ');
        }

        fullName = componentName + delim + fullName;
    }

    return fullName;
}

std::shared_ptr<SubSystem> Node::getParent() {
    return parent;
}

void Node::setParentUpdateNewUpdatePrev(std::shared_ptr<SubSystem> newParent){
    if(parent != nullptr){
        parent->removeChild(getSharedPointer());
    }

    parent = newParent;

    if(newParent != nullptr){
        newParent->addChild(getSharedPointer());
    }
}

void Node::setContextUpdateNewUpdatePrev(std::vector<Context> newContext){
    if(!context.empty()){
        //Should be in lowest level context
        std::shared_ptr<ContextRoot> oldContextRoot = context[context.size()-1].getContextRoot();
        int oldSubcontext = context[context.size()-1].getSubContext();
        oldContextRoot->removeSubContextNode(oldSubcontext, getSharedPointer());

        context = newContext;
        if(!newContext.empty()){
            std::shared_ptr<ContextRoot> newContextRoot = newContext[newContext.size()-1].getContextRoot();
            int newSubcontext = newContext[newContext.size()-1].getSubContext();
            newContextRoot->addSubContextNode(newSubcontext, getSharedPointer());
        }
    }
}

void Node::removeKnownReferences(){
    //Remove from parent
    if(parent != nullptr){
        parent->removeChild(getSharedPointer());
    }

    //Remove from context root
//    for(int i = 0; i<context.size(); i++){
//        std::shared_ptr<ContextRoot> contextRoot = context[i].getContextRoot();
//        int subcontext = context[i].getSubContext();
//        contextRoot->removeSubContextNode(subcontext, getSharedPointer());
//    }

    //Should only be in lowest level
    if(context.size()>0){
        std::shared_ptr<ContextRoot> contextRoot = context[context.size()-1].getContextRoot();
        int subcontext = context[context.size()-1].getSubContext();
        contextRoot->removeSubContextNode(subcontext, getSharedPointer());
    }
}

CExpr
Node::emitC(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag,
            bool checkFanout, bool forceFanout) {
    std::shared_ptr<OutputPort> outputPort = getOutputPort(outputPortNum);

    //Check if it has already been emitted
    bool emittedBefore = imag ? outputPort->isCEmittedIm() : outputPort->isCEmittedRe();

    //If it has been emitted before and this function was called, then fanout has occurd, go directly to returning the tmp var name
    if(emittedBefore && checkFanout){
        CExpr varName = imag ? outputPort->getCEmitImStr() : outputPort->getCEmitReStr();

        if(varName.getExpr().empty()){
            //This should not happen if fanout is properly reported
            throw std::runtime_error("Tried to emit a port which has previously been emitted but did not create an output variable: " + getFullyQualifiedName() + " Port " + GeneralHelper::to_string(outputPortNum) + (imag ? " Imag" : " Real") + " Arcs: " + GeneralHelper::to_string(outputPort->getArcsRaw().size()));
        }

        //Return the stored temp var name
        return varName;
    }else{
        //This output has not been emitted before, check if should fanout

        //It will be emitted by the end of the call to this fctn
        if(imag){
            outputPort->setCEmittedIm(true);
        }else{
            outputPort->setCEmittedRe(true);
        }

        bool fanout = false;
        if(forceFanout){
            fanout = true;
        }else if(!checkFanout){
            //If not forcing fanout and not checking, default to no fanout
            fanout = false;
        }else{
            //Check for fanout.  Either through more than 1 output arc or the single output arc having internal fanout

            //Will also force fanout on nodes with state since they can be scheduled after their dependent nodes
            //since they break the dependency chain.  This is a workaround for forceFanout not being passed through
            //to emitC calls in individual node's emitCExpr functions.  This force characteristic is used when nodes
            //are scheduled so that their outputs are saved to a variable.  The default behavior (to preserve the old
            //bottom-up emitter was to make this default to false).
            //Fortunatly, for previous versions of Laminar without this fix, the error condition was caught that
            //the node had been emitted but the fanout logic to save the cExpr had not been executed.  This resulted
            //in an empty string being returned on the second time the node was emitted which was caught.  Previous
            //outputs that did not throw this error should not have an error because of this issue.
            //TODO: Can remove fanout for state nodes if the forceFanout parameter is properly passsed to emitC from
            //      node emitCExpr function.  This requires re-factoring
            int numOutArcs = outputPort->getArcs().size();
            if(numOutArcs == 0){
                throw std::runtime_error("Tried to emit C for output port that is unconnected");
            }else if(numOutArcs > 1){
                //Multiple output arcs
                fanout = true;
            }else{
                //Single arc, check for internal fanout
                std::shared_ptr<InputPort> dstInput = (*outputPort->getArcs().begin())->getDstPort();
                fanout = dstInput->hasInternalFanout(imag);
                fanout |= hasState();
            }

        }

        if(fanout){
            //Get Expr Before Declaring/assigning so that items higher on the statement stack (prerequites) are enqueued first
            //Makes the temp var declaration more local to where it is assigned
            CExpr cExpr = emitCExpr(cStatementQueue, schedType, outputPortNum, imag);

            if(!cExpr.isOutputVariable()) {
                //For the CExpr to not be an output variable, it needs to be a scalar expression.
                //It will then be stored in a scalar variable
                //TODO: Remove this santiy check or correct if other CExprTypes are later implemented
                if(cExpr.getExprType() != CExpr::ExprType::SCALAR_EXPR){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Error when emitting C expression.  Unexpected CExpr type.", getSharedPointer()));
                }

                //An expression was returned, not a variable name ... create one

                //Declare output var and then assign it.  Set var name in port and return it as a string
                Variable outputVar = outputPort->getCOutputVar();

                //Get Output Var Decl
                std::string cVarDecl = outputVar.getCVarDecl(imag);

                //Enqueue decl and assignment
                std::string cVarDeclAssign = cVarDecl + " = " + cExpr.getExpr() + ";";
                cStatementQueue.push_back(cVarDeclAssign);

                //Set the var name in the port
                //TODO: If vectors later supported, change this
                CExpr outputVarName = CExpr(outputVar.getCVarName(imag), CExpr::ExprType::SCALAR_VAR);
                if (imag) {
                    outputPort->setCEmitImStr(outputVarName);
                } else {
                    outputPort->setCEmitReStr(outputVarName);
                }

                //Return output var name
                return outputVarName;
            }else{
                //A variable name was returned ... set the var name in the port and return the variable name in the expr
                //Avoid making a duplicate output variable
                if (imag) {
                    outputPort->setCEmitImStr(cExpr);
                } else {
                    outputPort->setCEmitReStr(cExpr);
                }

                return cExpr;
            }
        }else{
            //If not fanout, return expression
            return emitCExpr(cStatementQueue, schedType, outputPortNum, imag);
        }
    }
}

CExpr Node::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPort, bool imag) {
    //For now, the default behavior is to return an error message stating that the emitCExpr has not yet been implemented for this node.
    throw std::runtime_error("emitCExpr not yet implemented for node: " + getFullyQualifiedName() + " \n" + labelStr());

    return CExpr();
}

void Node::emitCExprNextState(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType) {
    //Default is to do nothing (since default is to have no state)
}

bool Node::hasState() {
    //Default is to return false
    return false;
}

bool Node::hasCombinationalPath(){
    //Default is to return true
    return true;
}

bool Node::canBreakBlockingDependency(int localSubBlockingLength){
    //Default is to return false
    return false;
}

void Node::specializeForBlocking(int localBlockingLength,
                                 int localSubBlockingLength,
                                 std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                 std::vector<std::shared_ptr<Node>> &nodesToRemove,
                                 std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                 std::vector<std::shared_ptr<Arc>> &arcsToRemove,
                                 std::vector<std::shared_ptr<Node>> &nodesToRemoveFromTopLevel,
                                 std::map<std::shared_ptr<Arc>, int> &arcsWithDeferredBlockingExpansion) {

    //For now, error out if there are order constraint arcs
    //TODO: Possibly re-evaluate this decision
    if(!getOrderConstraintInputArcs().empty() || !getOrderConstraintOutputArcs().empty()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Encountered order constraint arcs when specializing for sub-blocking", getSharedPointer()));
    }

    std::set<std::shared_ptr<Arc>> directInputArcs = getDirectInputArcs();
    std::set<std::shared_ptr<Arc>> directOutputArcs = getDirectOutputArcs();

    BlockingHelpers::createBlockingDomainHelper({getSharedPointer()},
            directInputArcs,
            directOutputArcs,
            parent,
            localBlockingLength,
            localSubBlockingLength,
            "BlockingDomainFor_" + name + "_n" + GeneralHelper::to_string(id),
            nodesToAdd,
            arcsToAdd,
            nodesToRemoveFromTopLevel,
            arcsWithDeferredBlockingExpansion);
}
/*
    //For now, error out if there are order constraint arcs
    //TODO: Possibly re-evaluate this decision
    if(!getOrderConstraintInputArcs().empty() || !getOrderConstraintOutputArcs().empty()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Encountered order constraint arcs when specializing for sub-blocking", getSharedPointer()));
    }

    std::shared_ptr<BlockingDomain> blockingDomain = NodeFactory::createNode<BlockingDomain>(parent);
    nodesToAdd.push_back(blockingDomain);
    blockingDomain->setName("BlockingDomainFor_" + name + "_n" + GeneralHelper::to_string(id));
    blockingDomain->setBlockingLen(localBlockingLength);
    blockingDomain->setSubBlockingLen(localSubBlockingLength);
    if(parent == nullptr){
        nodesToRemoveFromTopLevel.push_back(getSharedPointer());
    }
    setParentUpdateNewUpdatePrev(blockingDomain);

    //Place the node in a blocking domain
    if(contextDiscoveryAlreadyHappened){
        std::vector<Context> contextStack = getContext();
        contextStack.emplace_back(blockingDomain, 0);
        setContextUpdateNewUpdatePrev(contextStack);
    }

    std::set<std::shared_ptr<Arc>> directInputArcs = getDirectInputArcs();
    std::set<std::shared_ptr<Arc>> directOutputArcs = getDirectOutputArcs();

    //Create a blocking inputs for the domain
    //There should only be 1 input arc per input port
    for(const std::shared_ptr<Arc> &inputArc : directInputArcs){
        std::shared_ptr<BlockingInput> blockingInput = NodeFactory::createNode<BlockingInput>(blockingDomain);
        nodesToAdd.push_back(blockingInput);
        blockingDomain->addBlockInput(blockingInput);
        blockingInput->setBlockingLen(localBlockingLength);
        blockingInput->setSubBlockingLen(localSubBlockingLength);

        std::shared_ptr<InputPort> origInputPort = inputArc->getDstPort();

        blockingInput->setName("BlockingDomainInputFor_" + name + "_n" + GeneralHelper::to_string(id) + "_p" + GeneralHelper::to_string(origInputPort->getPortNum()));
        if(contextDiscoveryAlreadyHappened){
            std::vector<Context> contextStack = getContext();
            blockingInput->setContextUpdateNewUpdatePrev(contextStack);
        }

        inputArc->setDstPortUpdateNewUpdatePrev(blockingInput->getInputPortCreateIfNot(0));

        std::shared_ptr<Arc> blockingInputConnection = Arc::connectNodes(blockingInput->getOutputPortCreateIfNot(0), origInputPort, inputArc->getDataType(), inputArc->getSampleTime());
        arcsToAdd.push_back(blockingInputConnection);
    }

    std::map<std::shared_ptr<OutputPort>, std::shared_ptr<BlockingOutput>> outputPortsToBlockingOutputs;
    for(const std::shared_ptr<Arc> &outputArc : directOutputArcs){
        //Check if we have already created a blocking output for this output port
        std::shared_ptr<OutputPort> origOutputPort = outputArc->getSrcPort();
        std::shared_ptr<BlockingOutput> blockingOutput;
        if(GeneralHelper::contains(origOutputPort, outputPortsToBlockingOutputs)){
            blockingOutput = outputPortsToBlockingOutputs[origOutputPort];
        }else{
            //Create a new blocking output
            blockingOutput = NodeFactory::createNode<BlockingOutput>(blockingDomain);
            nodesToAdd.push_back(blockingOutput);
            outputPortsToBlockingOutputs[origOutputPort] = blockingOutput;
            blockingDomain->addBlockOutput(blockingOutput);
            blockingOutput->setBlockingLen(localBlockingLength);
            blockingOutput->setSubBlockingLen(localSubBlockingLength);
            blockingOutput->setName("BlockingDomainOutputFor" + name + "_n" + GeneralHelper::to_string(id) + "_p" + GeneralHelper::to_string(origOutputPort->getPortNum()));

            //Connect the blocking output
            std::shared_ptr<Arc> blockingOutputConnection = Arc::connectNodes(origOutputPort, blockingOutput->getInputPortCreateIfNot(0), outputArc->getDataType(), outputArc->getSampleTime());
            arcsToAdd.push_back(blockingOutputConnection);
        }

        outputArc->setSrcPortUpdateNewUpdatePrev(blockingOutput->getOutputPortCreateIfNot(0));
    }
}
*/

std::vector<std::shared_ptr<StateUpdate>> Node::getStateUpdateNodes(){
    //default has no state
    return stateUpdateNodes;
}

void Node::setStateUpdateNodes(std::vector<std::shared_ptr<StateUpdate>> stateUpdates){
    stateUpdateNodes = stateUpdates;
}

void Node::addStateUpdateNode(std::shared_ptr<StateUpdate> stateUpdate){
    stateUpdateNodes.push_back(stateUpdate);
}

void Node::removeStateUpdateNode(std::shared_ptr<StateUpdate> stateUpdate){
    stateUpdateNodes.erase(std::remove(stateUpdateNodes.begin(), stateUpdateNodes.end(), stateUpdate), stateUpdateNodes.end());
}

bool Node::hasGlobalDecl(){
    //Default is to return false
    return false;
}

bool Node::hasInternalFanout(int inputPort, bool imag){
    //Default is to check if that port is not a scalar.  If so, internal fanout is assumed.

    return !getInputPort(inputPort)->getDataType().isScalar();
}

std::vector<Variable> Node::getCStateVars() {
    //Default behavior is to return an empty vector (no state elements)
    return std::vector<Variable>();
}

void Node::emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, std::shared_ptr<StateUpdate> stateUpdateSrc) {
    //Default behavior is no action (since default is to have no state)
}

std::string Node::getGlobalDecl(){
    //Default is to return ""
    return "";
}

std::set<std::string> Node::getExternalIncludes(){
    //Default is to return nothing
    return std::set<std::string>();
}

Node::Node(std::shared_ptr<SubSystem> parent, Node* orig) : parent(parent), name(orig->name), id(orig->id), tmpCount(orig->tmpCount), partitionNum(orig->partitionNum), schedOrder(orig->schedOrder), origLocation(orig->origLocation) {

}

std::shared_ptr<Node> Node::shallowClone(std::shared_ptr<SubSystem> parent) {
    throw std::runtime_error("Called shallowClone on a class that has not provided a clone function");
}

void Node::shallowCloneWithChildren(std::shared_ptr<SubSystem> parent, std::vector<std::shared_ptr<Node>> &nodeCopies, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode, std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode){
    //By default, only copy current node (recursive copy is done in Subsystems and Expanded Nodes)

    std::shared_ptr<Node> clonedNode = shallowClone(parent);

    //Put into vectors and maps
    nodeCopies.push_back(clonedNode);
    origToCopyNode[shared_from_this()] = clonedNode;
    copyToOrigNode[clonedNode] = shared_from_this();
}

void Node::cloneInputArcs(std::vector<std::shared_ptr<Arc>> &arcCopies,
                          std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode,
                          std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &origToCopyArc,
                          std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &copyToOrigArc) {

    std::shared_ptr<Node> clonedDstNode = origToCopyNode[shared_from_this()];

    if(clonedDstNode == nullptr){
        throw std::runtime_error("Attempting to Clone Arc when Dst Node has Not Been Cloned");
    }

    //Iterate through input ports
    unsigned long numInputPorts = inputPorts.size();
    for(unsigned long i = 0; i<numInputPorts; i++){
        std::set<std::shared_ptr<Arc>> portArcs = inputPorts[i]->getArcs();
        int inputPortNum = inputPorts[i]->getPortNum();

        //Itterate through the arcs and duplicate
        for(auto arcIt = portArcs.begin(); arcIt != portArcs.end(); arcIt++){
            std::shared_ptr<Arc> origArc = (*arcIt);
            //Get Src Output Port Number and Src Node (as of now, all arcs origionate at an output port)
            std::shared_ptr<OutputPort> srcPort = origArc->getSrcPort();
            int srcPortNumber = srcPort->getPortNum();

            std::shared_ptr<Node> origSrcNode = srcPort->getParent();
            std::shared_ptr<Node> clonedSrcNode = origToCopyNode[origSrcNode];

            //Create the Cloned Arc
            std::shared_ptr<Arc> clonedArc = Arc::connectNodes(clonedSrcNode, srcPortNumber, clonedDstNode, inputPortNum, (*arcIt)->getDataType()); //This creates a new arc and connects them to the referenced node ports.
            clonedArc->shallowCopyPrameters(origArc.get());

            //Add to arc list and maps
            arcCopies.push_back(clonedArc);
            origToCopyArc[origArc] = clonedArc;
            copyToOrigArc[clonedArc] = origArc;

        }

    }

    //Iterate through arcs into OrderConstraintPort
    if(orderConstraintInputPort){
        std::set<std::shared_ptr<Arc>> portArcs = orderConstraintInputPort->getArcs();

        for(auto arcIt = portArcs.begin(); arcIt != portArcs.end(); arcIt++){
            std::shared_ptr<Arc> origArc = (*arcIt);
            //Get Src Output Port Number and Src Node (as of now, all arcs origionate at an output port)
            std::shared_ptr<OutputPort> srcPort = origArc->getSrcPort();

            std::shared_ptr<Node> origSrcNode = srcPort->getParent();
            std::shared_ptr<Node> clonedSrcNode = origToCopyNode[origSrcNode];

            std::shared_ptr<Arc> clonedArc;
            if(GeneralHelper::isType<OutputPort, OrderConstraintOutputPort>(srcPort) == nullptr) { //The output port is a real port
                int srcPortNumber = srcPort->getPortNum();

                //Create the Cloned Arc
                clonedArc = Arc::connectNodesOrderConstraint(clonedSrcNode, srcPortNumber, clonedDstNode, (*arcIt)->getDataType()); //This creates a new arc and connects them to the referenced node ports.
            }else{ //The output port is an order constraint port
                clonedArc = Arc::connectNodesOrderConstraint(clonedSrcNode, clonedDstNode, (*arcIt)->getDataType()); //This creates a new arc and connects them to the referenced node ports.
            }

            clonedArc->shallowCopyPrameters(origArc.get());

            //Add to arc list and maps
            arcCopies.push_back(clonedArc);
            origToCopyArc[origArc] = clonedArc;
            copyToOrigArc[clonedArc] = origArc;
        }
    }
}

std::set<std::shared_ptr<Arc>> Node::disconnectNode() {
    std::set<std::shared_ptr<Arc>> disconnectedArcs = disconnectInputs();

    std::set<std::shared_ptr<Arc>> moreDisconnectedArcs = disconnectOutputs();
    disconnectedArcs.insert(moreDisconnectedArcs.begin(), moreDisconnectedArcs.end());

    return disconnectedArcs;
}

std::set<std::shared_ptr<Node>> Node::getConnectedNodes() {
    std::set<std::shared_ptr<Node>> connectedNodes = getConnectedInputNodes();

    std::set<std::shared_ptr<Node>> moreConnectedNodes = getConnectedOutputNodes();

    connectedNodes.insert(moreConnectedNodes.begin(), moreConnectedNodes.end());

    return connectedNodes;
}

std::set<std::shared_ptr<Node>> Node::getConnectedInputNodes(){
    //Get directly connected input nodes
    std::set<std::shared_ptr<Node>> connectedNodes = getConnectedDirectInputNodes();

    //Get order constrained (virtually connected) input nodes
    std::set<std::shared_ptr<Node>> moreConnectedNodes = getConnectedOrderConstraintInNodes();
    connectedNodes.insert(moreConnectedNodes.begin(), moreConnectedNodes.end());

    return connectedNodes;
}

std::set<std::shared_ptr<Node>> Node::getConnectedDirectInputNodes() {
    std::set<std::shared_ptr<Node>> connectedNodes;

    //Iterate through the input ports/arcs
    unsigned long numInputPorts = inputPorts.size();
    for(unsigned long i = 0; i<numInputPorts; i++){
        //Get a copy of the arc set for this port
        std::set<std::shared_ptr<Arc>> inputArcs = inputPorts[i]->getArcs();

        for(auto it = inputArcs.begin(); it != inputArcs.end(); it++){
            connectedNodes.insert((*it)->getSrcPort()->getParent()); //The node connected to the input arc is the src node of the arc
        }
    }

    return connectedNodes;
}

std::set<std::shared_ptr<Node>> Node::getConnectedDirectOutputNodes(){
    std::set<std::shared_ptr<Node>> connectedNodes;

    //Iterate through the output ports/arcs
    unsigned long numOutputPorts = outputPorts.size();
    for(unsigned long i = 0; i<numOutputPorts; i++){
        //Get a copy of the arc set for this port
        std::set<std::shared_ptr<Arc>> outputArcs = outputPorts[i]->getArcs();

        for(auto it = outputArcs.begin(); it != outputArcs.end(); it++){
            connectedNodes.insert((*it)->getDstPort()->getParent()); //The node connected to the output arc is the dst node of the arc
        }
    }

    return connectedNodes;
}

std::set<std::shared_ptr<Node>> Node::getConnectedOutputNodes(){
    //Get directly connected input nodes
    std::set<std::shared_ptr<Node>> connectedNodes = getConnectedDirectOutputNodes();

    //Get order constrained (virtually connected) input nodes
    std::set<std::shared_ptr<Node>> moreConnectedNodes = getConnectedOrderConstraintOutNodes();
    connectedNodes.insert(moreConnectedNodes.begin(), moreConnectedNodes.end());

    return connectedNodes;
}

unsigned long Node::inDegree(){
    unsigned long count = directInDegree();

    count += orderConstraintInDegree();

    return count;
}

unsigned long Node::directInDegree() {
    unsigned long count = 0;

    //Iterate through the input ports/arcs
    unsigned long numInputPorts = inputPorts.size();
    for(unsigned long i = 0; i<numInputPorts; i++){
        count += inputPorts[i]->getArcs().size(); //Get the number of arcs connected to this port
    }

    return count;
}

unsigned long Node::directOutDegree() {
    unsigned long count = 0;

    //Iterate through the output ports/arcs
    unsigned long numOutputPorts = outputPorts.size();
    for(unsigned long i = 0; i<numOutputPorts; i++){
        count += outputPorts[i]->getArcs().size(); //Get the number of arcs connected to this port
    }

    return count;
}

unsigned long Node::outDegree(){
    unsigned long count = directOutDegree();

    count += orderConstraintOutDegree();

    return count;
}

unsigned long Node::outDegreeExclusingConnectionsTo(std::set<std::shared_ptr<Node>> ignoreSet) {
    unsigned long count = 0;

    //Iterate through the output ports/arcs
    unsigned long numOutputPorts = outputPorts.size();
    for(unsigned long i = 0; i<numOutputPorts; i++){
        //Get a copy of the arc set for this port
        std::set<std::shared_ptr<Arc>> outputArcs = outputPorts[i]->getArcs();

        for(auto it = outputArcs.begin(); it != outputArcs.end(); it++){
            std::shared_ptr<Arc> arc = *it;
            std::shared_ptr<Port> dstPort = arc->getDstPort();
            std::shared_ptr<Node> connectedNode = dstPort->getParent(); //The connected node for output arcs is the dst node.

            if(ignoreSet.find(connectedNode) == ignoreSet.end()){
                //If the dst node is not in the ignore set, include it in the count
                count++;
            }
        }
    }

    //Get a copy of the arc set for this port's order constraint output
    std::set<std::shared_ptr<Arc>> outputArcs = orderConstraintOutputPort->getArcs();

    for(auto it = outputArcs.begin(); it != outputArcs.end(); it++){
        std::shared_ptr<Arc> arc = *it;
        std::shared_ptr<Port> dstPort = arc->getDstPort();
        std::shared_ptr<Node> connectedNode = dstPort->getParent(); //The connected node for output arcs is the dst node.

        if(ignoreSet.find(connectedNode) == ignoreSet.end()){
            //If the dst node is not in the ignore set, include it in the count
            count++;
        }
    }

    return count;
}

std::vector<std::shared_ptr<Arc>> Node::connectUnconnectedPortsToNode(std::shared_ptr<Node> connectToSrc, std::shared_ptr<Node> connectToSink, int srcPortNum, int sinkPortNum){
    std::vector<std::shared_ptr<Arc>> newArcs;

    //Iterate through the input ports/arcs
    unsigned long numInputPorts = inputPorts.size();
    for(unsigned long i = 0; i<numInputPorts; i++){
        //Get a copy of the arc set for this port
        unsigned long numInputArcs = inputPorts[i]->getArcs().size();

        if(numInputArcs == 0){
            //This port is unconnected, connect it to the given node.
            int inputPortNumber = inputPorts[i]->getPortNum();

            //Connect from given node to this node's input port
            //TODO: Use default datatype for now.  Perhaps change later?
            std::shared_ptr<Arc> arc = Arc::connectNodes(connectToSrc, srcPortNum, shared_from_this(), inputPortNumber, DataType());
            newArcs.push_back(arc);
        }
    }

    //Iterate through the output ports/arcs
    unsigned long numOutputPorts = outputPorts.size();
    for(unsigned long i = 0; i<numOutputPorts; i++){
        //Get a copy of the arc set for this port
        unsigned long numOutputArcs = outputPorts[i]->getArcs().size();

        if(numOutputArcs == 0){
            //This port is unconnected, connect it to the given node.
            int outputPortNumber = outputPorts[i]->getPortNum();

            //Connect from this node's output port to the given node's port
            //TODO: Use default datatype for now.  Perhaps change later?
            std::shared_ptr<Arc> arc = Arc::connectNodes(shared_from_this(), outputPortNumber, connectToSink, sinkPortNum, DataType());
            newArcs.push_back(arc);
        }
    }

    return newArcs;
}

int Node::getPartitionNum() const {
    return partitionNum;
}

void Node::setPartitionNum(int partitionNum) {
    Node::partitionNum = partitionNum;
}

int Node::getSchedOrder() const {
    return schedOrder;
}

void Node::setSchedOrder(int schedOrder) {
    Node::schedOrder = schedOrder;
}

std::set<std::shared_ptr<Arc>> Node::disconnectInputs() {
    //Disconnect the direct inputs
    std::set<std::shared_ptr<Arc>> disconnectedArcs = disconnectDirectInputs();

    //Disconnect the OrderConstraintIn arcs
    std::set<std::shared_ptr<Arc>> moreDisconnectedArcs = disconnectOrderConstraintInArcs();
    disconnectedArcs.insert(moreDisconnectedArcs.begin(), moreDisconnectedArcs.end());

    return  disconnectedArcs;
}

std::set<std::shared_ptr<Arc>> Node::disconnectDirectInputs() {
    std::set<std::shared_ptr<Arc>> disconnectedArcs;

    //Iterate through the input ports/arcs
    unsigned long numInputPorts = inputPorts.size();
    for(unsigned long i = 0; i<numInputPorts; i++){
        //Get a copy of the arc set for this port
        std::set<std::shared_ptr<Arc>> inputArcs = inputPorts[i]->getArcs();

        //Disconnect each of the arcs from both ends
        //We can do this without disturbing the port the set since the set here is a copy of the port set
        for(auto it = inputArcs.begin(); it != inputArcs.end(); it++){
            (*it)->disconnect();

            //Add this arc to the list of arcs removed
            disconnectedArcs.insert(*it);
        }
    }

    return disconnectedArcs;
}

std::set<std::shared_ptr<Arc>> Node::disconnectOutputs() {
    //Disconnect the direct outputs
    std::set<std::shared_ptr<Arc>> disconnectedArcs = disconnectDirectOutputs();

    //Disconnect the OrderConstraintOut arcs
    std::set<std::shared_ptr<Arc>> moreDisconnectedArcs = disconnectOrderConstraintOutArcs();
    disconnectedArcs.insert(moreDisconnectedArcs.begin(), moreDisconnectedArcs.end());

    return  disconnectedArcs;
}

std::set<std::shared_ptr<Arc>> Node::disconnectDirectOutputs() {
    std::set<std::shared_ptr<Arc>> disconnectedArcs;

    //Iterate through the output ports/arcs
    unsigned long numOutputPorts = outputPorts.size();
    for(unsigned long i = 0; i<numOutputPorts; i++){
        //Get a copy of the arc set for this port
        std::set<std::shared_ptr<Arc>> outputArcs = outputPorts[i]->getArcs();

        //Disconnect each of the arcs from both ends
        //We can do this without disturbing the port the set since the set here is a copy of the port set
        for(auto it = outputArcs.begin(); it != outputArcs.end(); it++){
            (*it)->disconnect();

            //Add this arc to the list of arcs removed
            disconnectedArcs.insert(*it);
        }
    }

    return disconnectedArcs;
}

bool Node::lessThanSchedOrder(std::shared_ptr<Node> a, std::shared_ptr<Node> b){
    return a->schedOrder < b->schedOrder;
}

void Node::addOrderConstraintInArcUpdatePrevUpdateArc(std::shared_ptr<Arc> arc) {
    if(!orderConstraintInputPort){
        //Order Constraint Port not yet created ... create it
        orderConstraintInputPort = std::unique_ptr<OrderConstraintInputPort>(new OrderConstraintInputPort(this));
    }

    //Set the dst port of the arc, updating the previous port and this one
    arc->setDstPortUpdateNewUpdatePrev(orderConstraintInputPort->getSharedPointerInputPort());
}

void Node::addOrderConstraintOutArcUpdatePrevUpdateArc(std::shared_ptr<Arc> arc) {
    if(!orderConstraintOutputPort){
        //Order Constraint Port not yet created ... create it
        orderConstraintOutputPort = std::unique_ptr<OrderConstraintOutputPort>(new OrderConstraintOutputPort(this));
    }

    //Set the dst port of the arc, updating the previous port and this one
    arc->setSrcPortUpdateNewUpdatePrev(orderConstraintOutputPort->getSharedPointerOutputPort());
}

void Node::removeOrderConstraintInArc(std::shared_ptr<Arc> arc) {
    if(orderConstraintInputPort) {
        orderConstraintInputPort->removeArc(arc);
    }
}

void Node::removeOrderConstraintOutArc(std::shared_ptr<Arc> arc) {
    if(orderConstraintOutputPort) {
        orderConstraintOutputPort->removeArc(arc);
    }
}

std::set<std::shared_ptr<Arc>> Node::disconnectOrderConstraintInArcs() {
    std::set<std::shared_ptr<Arc>> disconnectedArcs;

    if(orderConstraintInputPort) {
        //Get a copy of the arc set for this port
        std::set<std::shared_ptr<Arc>> constraintArcs = orderConstraintInputPort->getArcs();

        //Disconnect each of the arcs from both ends
        //We can do this without disturbing the port the set since the set here is a copy of the port set
        for (auto it = constraintArcs.begin(); it != constraintArcs.end(); it++) {
            (*it)->disconnect();

            //Add this arc to the list of arcs removed
            disconnectedArcs.insert(*it);
        }
    }

    return disconnectedArcs;
}

std::set<std::shared_ptr<Arc>> Node::disconnectOrderConstraintOutArcs() {
    std::set<std::shared_ptr<Arc>> disconnectedArcs;

    if(orderConstraintOutputPort) {
        //Get a copy of the arc set for this port
        std::set<std::shared_ptr<Arc>> constraintArcs = orderConstraintOutputPort->getArcs();

        //Disconnect each of the arcs from both ends
        //We can do this without disturbing the port the set since the set here is a copy of the port set
        for (auto it = constraintArcs.begin(); it != constraintArcs.end(); it++) {
            (*it)->disconnect();

            //Add this arc to the list of arcs removed
            disconnectedArcs.insert(*it);
        }
    }

    return disconnectedArcs;
}

std::set<std::shared_ptr<Node>> Node::getConnectedOrderConstraintInNodes() {
    std::set<std::shared_ptr<Node>> connectedNodes;

    if (orderConstraintInputPort){
        //Get a copy of the arc set for this port
        std::set<std::shared_ptr<Arc>> constraintArcs = orderConstraintInputPort->getArcs();

        for (auto it = constraintArcs.begin(); it != constraintArcs.end(); it++) {
            connectedNodes.insert(
                    (*it)->getSrcPort()->getParent()); //The node connected to the input arc is the src node of the arc
        }
    }

    return connectedNodes;
}

std::set<std::shared_ptr<Node>> Node::getConnectedOrderConstraintOutNodes() {
    std::set<std::shared_ptr<Node>> connectedNodes;

    if (orderConstraintOutputPort){
        //Get a copy of the arc set for this port
        std::set<std::shared_ptr<Arc>> constraintArcs = orderConstraintOutputPort->getArcs();

        for (auto it = constraintArcs.begin(); it != constraintArcs.end(); it++) {
            connectedNodes.insert(
                    (*it)->getDstPort()->getParent()); //The node connected to the output arc is the dst node of the arc
        }
    }

    return connectedNodes;
}

unsigned long Node::orderConstraintInDegree() {
    unsigned long count = 0;

    if(orderConstraintInputPort) {
        count = orderConstraintInputPort->getArcs().size(); //Get the number of arcs connected to this port
    }

    return count;
}

unsigned long Node::orderConstraintOutDegree() {
    unsigned long count = 0;

    if(orderConstraintOutputPort) {
        count = orderConstraintOutputPort->getArcs().size(); //Get the number of arcs connected to this port
    }

    return count;
}

std::shared_ptr<OrderConstraintInputPort> Node::getOrderConstraintInputPort() {
    if(!orderConstraintInputPort) {
        return std::shared_ptr<OrderConstraintInputPort>(nullptr);
    }

    return orderConstraintInputPort->getSharedPointerOrderConstraintPort();
}

std::shared_ptr<OrderConstraintOutputPort> Node::getOrderConstraintOutputPort() {
    if(!orderConstraintOutputPort) {
        return std::shared_ptr<OrderConstraintOutputPort>(nullptr);
    }

    return orderConstraintOutputPort->getSharedPointerOrderConstraintPort();
}

std::shared_ptr<OrderConstraintInputPort> Node::getOrderConstraintInputPortCreateIfNot() {
    if(!orderConstraintInputPort) {
        orderConstraintInputPort = std::unique_ptr<OrderConstraintInputPort>(new OrderConstraintInputPort(this));
    }

    return orderConstraintInputPort->getSharedPointerOrderConstraintPort();
}

std::shared_ptr<OrderConstraintOutputPort> Node::getOrderConstraintOutputPortCreateIfNot() {
    if(!orderConstraintOutputPort) {
        orderConstraintOutputPort = std::unique_ptr<OrderConstraintOutputPort>(new OrderConstraintOutputPort(this));
    }

    return orderConstraintOutputPort->getSharedPointerOrderConstraintPort();
}

std::vector<std::shared_ptr<InputPort>> Node::getInputPortsIncludingSpecial() {
    //By default, just return the standard input ports
    return getInputPorts();
}


std::vector<std::shared_ptr<InputPort>> Node::getInputPortsIncludingSpecialAndOrderConstraint(){
    std::vector<std::shared_ptr<InputPort>> inputPortList = getInputPortsIncludingSpecial();

    inputPortList.push_back(orderConstraintInputPort->getSharedPointerOrderConstraintPort());

    return inputPortList;
}

std::vector<Context> Node::getContext() const {
    return context;
}

void Node::setContext(std::vector<Context> &context) {
    Node::context = context;
}

Context Node::getContext(unsigned long i) const {
    return context[i];
}

void Node::setContext(unsigned long i, Context &context) {
    Node::context[i] = context;
}

void Node::pushBackContext(Context &context) {
    Node::context.push_back(context);
}

void Node::removeContext(Context &context) {
    Node::context.erase(std::remove(Node::context.begin(), Node::context.end(), context), Node::context.end());
}

void Node::removeContext(unsigned long i){
    context.erase(context.begin()+i);
}

bool Node::createStateUpdateNode(std::vector<std::shared_ptr<Node>> &new_nodes,
                                 std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                 std::vector<std::shared_ptr<Arc>> &new_arcs,
                                 std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                 bool includeContext){
    //By default, returns false since the default assumption is that the node does not have state.
    //Override if the node has state

    return false;
}

void Node::copyPortNames(std::shared_ptr<Node> copyFrom) {
    std::vector<std::shared_ptr<InputPort>> origInputPorts = copyFrom->getInputPorts();

    for(unsigned long i = 0; i<origInputPorts.size(); i++){
        int portNum = origInputPorts[i]->getPortNum();
        std::string name = origInputPorts[i]->getName();

        std::shared_ptr<InputPort> port = getInputPortCreateIfNot(portNum);
        port->setName(name);
    }

    std::vector<std::shared_ptr<OutputPort>> origOutputPorts = copyFrom->getOutputPorts();
    for(unsigned long i = 0; i<origOutputPorts.size(); i++){
        int portNum = origOutputPorts[i]->getPortNum();
        std::string name = origOutputPorts[i]->getName();

        std::shared_ptr<OutputPort> port = getOutputPortCreateIfNot(portNum);
        port->setName(name);
    }
}

std::set<std::shared_ptr<Arc>> Node::getDirectInputArcs(){
    std::set<std::shared_ptr<Arc>> arcs;

    //Get the input ports (including any special ports - ie. select)
    std::vector<std::shared_ptr<InputPort>> inputPortSet = getInputPortsIncludingSpecial();

    //Iterate through the output ports/arcs
    unsigned long numInputPorts = inputPortSet.size();
    for(unsigned long i = 0; i<numInputPorts; i++){
        //Get a copy of the arc set for this port
        std::set<std::shared_ptr<Arc>> inputArcs = inputPortSet[i]->getArcs();
        arcs.insert(inputArcs.begin(), inputArcs.end());
    }

    return arcs;
}

std::set<std::shared_ptr<Arc>> Node::getOrderConstraintInputArcs(){
    //Get a copy of the arc set for this port
    std::set<std::shared_ptr<Arc>> arcs = orderConstraintInputPort->getArcs();

    return arcs;
}

std::set<std::shared_ptr<Arc>> Node::getInputArcs(){
    std::set<std::shared_ptr<Arc>> arcs = getDirectInputArcs();

    std::set<std::shared_ptr<Arc>> moreArcs = getOrderConstraintInputArcs();
    arcs.insert(moreArcs.begin(), moreArcs.end());

    return arcs;
}

std::set<std::shared_ptr<Arc>> Node::getDirectOutputArcs(){
    std::set<std::shared_ptr<Arc>> arcs;

    //Iterate through the output ports/arcs
    unsigned long numOutputPorts = outputPorts.size();
    for(unsigned long i = 0; i<numOutputPorts; i++){
        //Get a copy of the arc set for this port
        std::set<std::shared_ptr<Arc>> outputArcs = outputPorts[i]->getArcs();
        arcs.insert(outputArcs.begin(), outputArcs.end());

    }

    return arcs;
}

std::set<std::shared_ptr<Arc>> Node::getOrderConstraintOutputArcs(){
    //Get a copy of the arc set for this port
    std::set<std::shared_ptr<Arc>> arcs = orderConstraintOutputPort->getArcs();

    return arcs;
}

std::set<std::shared_ptr<Arc>> Node::getOutputArcs(){
    std::set<std::shared_ptr<Arc>> arcs = getDirectOutputArcs();

    std::set<std::shared_ptr<Arc>> moreArcs = getOrderConstraintOutputArcs();
    arcs.insert(moreArcs.begin(), moreArcs.end());

    return arcs;
}

std::set<std::shared_ptr<Node>> Node::getDependentNodes1Degree(){
    std::set<std::shared_ptr<Node>> nodes;
    std::set<std::shared_ptr<Arc>> outputArcs = getOutputArcs();

    for(const std::shared_ptr<Arc> &arc : outputArcs){
        nodes.insert(arc->getDstPort()->getParent());
    }

    return nodes;
}

std::string Node::getErrorReportContextStr(){
    return getFullyQualifiedName(true, "/");
}

bool Node::PtrID_Compare::operator() (const std::shared_ptr<Node>& lhs, const std::shared_ptr<Node>& rhs) const{
    if(lhs->getId() != rhs->getId()){
        return lhs->getId() < rhs->getId();
    }else{
        return lhs<rhs;
    }
}

bool Node::outputsInSameContext(){
    if(outputPorts.size() == 0 && orderConstraintOutputPort == nullptr){
        return true;
    }

    bool foundContext = false;
    std::vector<Context> dstContext;

    //standard ports
    for(int i = 0; i<outputPorts.size(); i++){
        std::set<std::shared_ptr<Arc>> arcs = outputPorts[i]->getArcs();
        for(auto it = arcs.begin(); it != arcs.end(); it++){
            std::shared_ptr<Node> dstNode = (*it)->getDstPort()->getParent();
            if(!foundContext){
                dstContext = (*it)->getDstPort()->getParent()->getContext();
                foundContext = true;
            }else{
                if(!Context::isEqContext(dstContext, (*it)->getDstPort()->getParent()->getContext())){
                    return false;
                }
            }
        }
    }

    //Order constraint port
    if(orderConstraintOutputPort != nullptr) {
        std::set<std::shared_ptr<Arc>> arcs = orderConstraintOutputPort->getArcs();
        for (auto it = arcs.begin(); it != arcs.end(); it++) {
            std::shared_ptr<Node> dstNode = (*it)->getDstPort()->getParent();
            if (!foundContext) {
                context = (*it)->getDstPort()->getParent()->getContext();
                foundContext = true;
            } else {
                if (!Context::isEqContext(context, (*it)->getDstPort()->getParent()->getContext())) {
                    return false;
                }
            }
        }
    }

    return true;
}

std::vector<Context> Node::getOutputContext(){
    if(outputPorts.size() == 0 && orderConstraintOutputPort == nullptr){
        return {};
    }

    bool foundContext = false;
    std::vector<Context> dstContext;

    //standard ports
    for(int i = 0; i<outputPorts.size(); i++){
        std::set<std::shared_ptr<Arc>> arcs = outputPorts[i]->getArcs();
        for(auto it = arcs.begin(); it != arcs.end(); it++){
            std::shared_ptr<Node> dstNode = (*it)->getDstPort()->getParent();
            if(!foundContext){
                dstContext = (*it)->getDstPort()->getParent()->getContext();
                foundContext = true;
            }else{
                if(!Context::isEqContext(dstContext, (*it)->getDstPort()->getParent()->getContext())){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Attempted to get a unified output context for a node which has arcs to different contexts"));
                }
            }
        }
    }

    //Order constraint port
    if(orderConstraintOutputPort != nullptr) {
        std::set<std::shared_ptr<Arc>> arcs = orderConstraintOutputPort->getArcs();
        for (auto it = arcs.begin(); it != arcs.end(); it++) {
            std::shared_ptr<Node> dstNode = (*it)->getDstPort()->getParent();
            if (!foundContext) {
                context = (*it)->getDstPort()->getParent()->getContext();
                foundContext = true;
            } else {
                if (!Context::isEqContext(context, (*it)->getDstPort()->getParent()->getContext())) {
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Attempted to get a unified output context for a node which has arcs to different contexts"));
                }
            }
        }
    }

    return dstContext;
}

bool Node::outputsInSamePartition(){
    if(outputPorts.size() == 0 && orderConstraintOutputPort == nullptr){
        return true;
    }

    bool foundPartition = false;
    int dstPartition;

    //standard ports
    for(int i = 0; i<outputPorts.size(); i++){
        std::set<std::shared_ptr<Arc>> arcs = outputPorts[i]->getArcs();
        for(auto it = arcs.begin(); it != arcs.end(); it++){
            std::shared_ptr<Node> dstNode = (*it)->getDstPort()->getParent();
            if(!foundPartition){
                dstPartition = (*it)->getDstPort()->getParent()->getPartitionNum();
                foundPartition = true;
            }else{
                if(dstPartition != (*it)->getDstPort()->getParent()->getPartitionNum()){
                    return false;
                }
            }
        }
    }

    //Order constraint port
    if(orderConstraintOutputPort != nullptr) {
        std::set<std::shared_ptr<Arc>> arcs = orderConstraintOutputPort->getArcs();
        for (auto it = arcs.begin(); it != arcs.end(); it++) {
            std::shared_ptr<Node> dstNode = (*it)->getDstPort()->getParent();
            if (!foundPartition) {
                dstPartition = (*it)->getDstPort()->getParent()->getPartitionNum();
                foundPartition = true;
            } else {
                if(dstPartition != (*it)->getDstPort()->getParent()->getPartitionNum()){
                    return false;
                }
            }
        }
    }

    return true;
}

int Node::getOutputPartition(){
    if(outputPorts.size() == 0 && orderConstraintOutputPort == nullptr){
        return -1;
    }

    bool foundPartition = false;
    int dstPartition = -1;

    //standard ports
    for(int i = 0; i<outputPorts.size(); i++){
        std::set<std::shared_ptr<Arc>> arcs = outputPorts[i]->getArcs();
        for(auto it = arcs.begin(); it != arcs.end(); it++){
            std::shared_ptr<Node> dstNode = (*it)->getDstPort()->getParent();
            if(!foundPartition){
                dstPartition = (*it)->getDstPort()->getParent()->getPartitionNum();
                foundPartition = true;
            }else{
                if(dstPartition != (*it)->getDstPort()->getParent()->getPartitionNum()){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Attempted to get a unified output partition for a node which has arcs to different partitions"));
                }
            }
        }
    }

    //Order constraint port
    if(orderConstraintOutputPort != nullptr) {
        std::set<std::shared_ptr<Arc>> arcs = orderConstraintOutputPort->getArcs();
        for (auto it = arcs.begin(); it != arcs.end(); it++) {
            std::shared_ptr<Node> dstNode = (*it)->getDstPort()->getParent();
            if (!foundPartition) {
                dstPartition = (*it)->getDstPort()->getParent()->getPartitionNum();
                foundPartition = true;
            } else {
                if(dstPartition != (*it)->getDstPort()->getParent()->getPartitionNum()){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Attempted to get a unified output partition for a node which has arcs to different partitions"));
                }
            }
        }
    }

    return dstPartition;
}

std::vector<std::string> Node::getOrigLocation() const {
    return origLocation;
}

void Node::setOrigLocation(const std::vector<std::string> &origLocation) {
    Node::origLocation = origLocation;
}

void Node::setOrigLocation(){
    origLocation = std::vector<std::string>();

    std::shared_ptr<Node> cursor = parent;
    while(cursor){
        origLocation.insert(origLocation.begin(), cursor->getName());
        cursor = cursor->parent;
    }
}

std::string Node::getFullyQualifiedOrigName(bool sanitize, std::string delim) {
    std::string fullName;

    for(int i = 0; i<origLocation.size(); i++)
    {
        std::string componentName = origLocation[i];

        if(sanitize){
            componentName = GeneralHelper::replaceAll(componentName, '\n', ' ');
        }

        fullName += componentName;
        fullName += delim;
    }

    std::string componentName = getName();

    if(sanitize){
        componentName = GeneralHelper::replaceAll(componentName, '\n', ' ');
    }

    fullName += componentName;

    return fullName;
}