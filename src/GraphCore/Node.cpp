//
// Created by Christopher Yarp on 6/25/18.
//

#include <vector>
#include <memory>
#include <string>
#include <map>

#include "SubSystem.h"
#include "Port.h"
#include "Node.h"
#include "InputPort.h"
#include "OutputPort.h"
#include "NodeFactory.h"

#include "GraphMLTools/GraphMLHelper.h"
#include "General/GeneralHelper.h"

Node::Node() : id(-1), name(""), partitionNum(-1), schedOrder(-1), tmpCount(0)
{
    parent = std::shared_ptr<SubSystem>(nullptr);

    //NOTE: CANNOT init ports here since we need a shared pointer to this object
}

Node::Node(std::shared_ptr<SubSystem> parent) : id(-1), name(""), partitionNum(-1), schedOrder(0), tmpCount(0), parent(parent) { }

void Node::init() {
    //Nothing required for this case since ports are the only thing that require this and generic nodes are initialized with no ports
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

    if(!name.empty()){
        GraphMLHelper::addDataNode(doc, nodeElement, "instance_name", name);
    }

    std::string label = labelStr();
    if(!label.empty()){
        GraphMLHelper::addDataNode(doc, nodeElement, "block_label", label);
    }

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
}

void Node::propagateProperties() {
    //Default behavior will do nothing
}

void Node::setParent(std::shared_ptr<SubSystem> parent) {
    Node::parent = parent;
}

//Default behavior is to not do any expansion and to return false.
bool Node::expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes,
                  std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs) {
    return false;
}

std::string Node::getFullyQualifiedName() {
    std::string fullName = name;

    for(std::shared_ptr<SubSystem> parentPtr = parent; parentPtr != nullptr; parentPtr = parentPtr->parent)
    {
        fullName = parentPtr->getName() + "/" + fullName;
    }

    return fullName;
}

std::shared_ptr<SubSystem> Node::getParent() {
    return parent;
}

std::string
Node::emitC(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag, bool checkFanout, bool forceFanout) {
    std::shared_ptr<OutputPort> outputPort = getOutputPort(outputPortNum);

    //Check if it has already been emitted
    bool emittedBefore = imag ? outputPort->isCEmittedIm() : outputPort->isCEmittedRe();

    //If it has been emitted before and this function was called, then fanout has occurd, go directly to returning the tmp var name
    if(emittedBefore && checkFanout){
        std::string varName = imag ? outputPort->getCEmitImStr() : outputPort->getCEmitReStr();

        if(varName.empty()){
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
            }

        }

        if(fanout){
            //Get Expr Before Declaring/assigning so that items higher on the statement stack (prerequites) are enqueued first
            //Makes the temp var declaration more local to where it is assigned
            CExpr cExpr = emitCExpr(cStatementQueue, outputPortNum, imag);

            if(!cExpr.isOutputVariable()) {
                //An expression was returned, not a variable name ... create one

                //Declare output var and then assign it.  Set var name in port and return it as a string
                Variable outputVar = outputPort->getCOutputVar();

                //Get Output Var Decl
                std::string cVarDecl = outputVar.getCVarDecl(imag);

                //Enqueue decl and assignment
                std::string cVarDeclAssign = cVarDecl + " = " + cExpr.getExpr() + ";";
                cStatementQueue.push_back(cVarDeclAssign);

                //Set the var name in the port
                std::string outputVarName = outputVar.getCVarName(imag);
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
                std::string outputVarName = cExpr.getExpr();
                if (imag) {
                    outputPort->setCEmitImStr(outputVarName);
                } else {
                    outputPort->setCEmitReStr(outputVarName);
                }

                return outputVarName;
            }
        }else{
            //If not fanout, return expression
            return emitCExpr(cStatementQueue, outputPortNum, imag).getExpr();
        }
    }
}

CExpr Node::emitCExpr(std::vector<std::string> &cStatementQueue, int outputPort, bool imag) {
    //TODO: Make abstract

    //For now, the default behavior is to return an error message stating that the emitCExpr has not yet been implemented for this node.
    throw std::runtime_error("emitCExpr not yet implemented for node: \n" + labelStr());

    return CExpr();
}

void Node::emitCExprNextState(std::vector<std::string> &cStatementQueue) {
    //Default is to do nothing (since default is to have no state)
}

bool Node::hasState() {
    //Default is to return false
    return false;
}

bool Node::hasGlobalDecl(){
    //Default is to return false
    return false;
}

bool Node::hasInternalFanout(int inputPort, bool imag){
    //Default is to check if that port has a width >1.  If so, internal fanout is assumed.

    int inputWidth = getInputPort(inputPort)->getDataType().getWidth();

    return inputWidth>1;
}

std::vector<Variable> Node::getCStateVars() {
    //Default behavior is to return an empty vector (no state elements)
    return std::vector<Variable>();
}

void Node::emitCStateUpdate(std::vector<std::string> &cStatementQueue){
    //Default behavior is no action (since default is to have no state)
}

std::string Node::getGlobalDecl(){
    //Default is to return ""
    return "";
}

Node::Node(std::shared_ptr<SubSystem> parent, Node* orig) : parent(parent), name(orig->name), id(orig->id), tmpCount(orig->tmpCount), partitionNum(orig->partitionNum), schedOrder(orig->schedOrder) {

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

    //Itterate throught input ports
    unsigned long numInputPorts = inputPorts.size();
    for(unsigned long i = 0; i<numInputPorts; i++){
        std::set<std::shared_ptr<Arc>> portArcs = inputPorts[i]->getArcs();
        int inputPortNum = inputPorts[i]->getPortNum();

        std::shared_ptr<Node> clonedDstNode = origToCopyNode[shared_from_this()];

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

}

std::set<std::shared_ptr<Arc>> Node::disconnectNode() {
    std::set<std::shared_ptr<Arc>> disconnectedArcs;

    //Iterate through the input ports/arcs
    unsigned long numInputPorts = inputPorts.size();
    for(unsigned long i = 0; i<numInputPorts; i++){
        //Get a copy of the arc set for this port
        std::set<std::shared_ptr<Arc>> inputArcs = inputPorts[i]->getArcs();

        //Disconnect each of the arcs from both ends
        //We can do this without disturbing the port the set since the set here is a copy of the port set
        for(auto it = inputArcs.begin(); it != inputArcs.end(); it++){
            //These functions update the previous endpoints of the arc (ie. removes the arc from them)
            (*it)->setDstPortUpdateNewUpdatePrev(nullptr);
            (*it)->setSrcPortUpdateNewUpdatePrev(nullptr);

            //Add this arc to the list of arcs removed
            disconnectedArcs.insert(*it);
        }
    }

    //Iterate through the output ports/arcs
    unsigned long numOutputPorts = outputPorts.size();
    for(unsigned long i = 0; i<numOutputPorts; i++){
        //Get a copy of the arc set for this port
        std::set<std::shared_ptr<Arc>> outputArcs = outputPorts[i]->getArcs();

        //Disconnect each of the arcs from both ends
        //We can do this without disturbing the port the set since the set here is a copy of the port set
        for(auto it = outputArcs.begin(); it != outputArcs.end(); it++){
            //These functions update the previous endpoints of the arc (ie. removes the arc from them)
            (*it)->setDstPortUpdateNewUpdatePrev(nullptr);
            (*it)->setSrcPortUpdateNewUpdatePrev(nullptr);

            //Add this arc to the list of arcs removed
            disconnectedArcs.insert(*it);
        }
    }

    return disconnectedArcs;
}

std::set<std::shared_ptr<Node>> Node::getConnectedNodes() {
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

unsigned long Node::inDegree(){
    unsigned long count = 0;

    //Iterate through the input ports/arcs
    unsigned long numInputPorts = inputPorts.size();
    for(unsigned long i = 0; i<numInputPorts; i++){
        count += inputPorts[i]->getArcs().size(); //Get the number of arcs connected to this port
    }

    return count;
}

unsigned long Node::outDegree() {
    unsigned long count = 0;

    //Iterate through the output ports/arcs
    unsigned long numOutputPorts = outputPorts.size();
    for(unsigned long i = 0; i<numOutputPorts; i++){
        count += outputPorts[i]->getArcs().size(); //Get the number of arcs connected to this port
    }

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
            std::shared_ptr<Node> connectedNode = (*it)->getDstPort()->getParent(); //The connected node for output arcs is the dst node.

            if(ignoreSet.find(connectedNode) != ignoreSet.end()){
                //If the dst node is not in the ignore set, include it in the count
                count++;
            }
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