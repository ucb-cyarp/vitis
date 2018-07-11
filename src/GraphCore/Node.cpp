//
// Created by Christopher Yarp on 6/25/18.
//

#include <vector>
#include <memory>
#include <string>

#include "SubSystem.h"
#include "Port.h"
#include "Node.h"

Node::Node() : id(-1), partitionNum(0)
{
    parent = std::shared_ptr<SubSystem>(nullptr);

    //NOTE: CANNOT init ports here since we need a shared pointer to this object
}

Node::Node(std::shared_ptr<SubSystem> parent) : id(-1), partitionNum(0), parent(parent) { }

void Node::init() {
    //Nothing required for this case since ports are the only thing that require this and generic nodes are initialized with no ports
}

void Node::addInArcUpdatePrevUpdateArc(int portNum, std::shared_ptr<Arc> arc) {
    //Create the requested port if it does not exist yet
    //TODO: it is assumed that if port n exists, that there are ports 0-n with no holes.  Re-evaluate this assumption
    unsigned long inputPortLen = inputPorts.size();
    for(unsigned long i = inputPortLen; i <= portNum; i++)
    {
        inputPorts.push_back(std::unique_ptr<Port>(new Port(this, Port::PortType::INPUT, i)));
    }

    //Set the dst port of the arc, updating the previous port and this one
    arc->setDstPortUpdateNewUpdatePrev(inputPorts[portNum]->getSharedPointer());
}

void Node::addOutArcUpdatePrevUpdateArc(int portNum, std::shared_ptr<Arc> arc) {
    //Create the requested port if it does not exist yet
    //TODO: it is assumed that if port n exists, that there are ports 0-n with no holes.  Re-evaluate this assumption
    unsigned long outputPortLen = outputPorts.size();
    for (unsigned long i = outputPortLen; i <= portNum; i++) {
        outputPorts.push_back(std::unique_ptr<Port>(new Port(this, Port::PortType::OUTPUT, i)));
    }

    //Set the src port of the arc, updating the previous port and this one
    arc->setSrcPortUpdateNewUpdatePrev(outputPorts[portNum]->getSharedPointer());
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

std::shared_ptr<Port> Node::getInputPort(int portNum) {
    if(portNum >= inputPorts.size()) {
        return std::shared_ptr<Port>(nullptr);
    }

    return inputPorts[portNum]->getSharedPointer();
}

std::shared_ptr<Port> Node::getOutputPort(int portNum) {
    if(portNum >= outputPorts.size()) {
        return std::shared_ptr<Port>(nullptr);
    }

    return outputPorts[portNum]->getSharedPointer();
}

std::vector<std::shared_ptr<Port>> Node::getInputPorts() {
    std::vector<std::shared_ptr<Port>> inputPortPtrs;

    unsigned long inputPortLen = inputPorts.size();
    for(unsigned long i = 0; i<inputPortLen; i++)
    {
        inputPortPtrs.push_back(inputPorts[i]->getSharedPointer());
    }

    return inputPortPtrs;
}

std::vector<std::shared_ptr<Port>> Node::getOutputPorts() {
    std::vector<std::shared_ptr<Port>> outputPortPtrs;

    unsigned long outputPortLen = outputPorts.size();
    for (unsigned long i = 0; i < outputPortLen; i++)
    {
        outputPortPtrs.push_back(outputPorts[i]->getSharedPointer());
    }

    return outputPortPtrs;
}

std::string Node::getFullGraphMLPath() {
    std::string path = "n" + std::to_string(id);

    for(std::shared_ptr<SubSystem> parentPtr = parent; parentPtr != nullptr; parentPtr = parentPtr->parent)
    {
        path = "n" + std::to_string(parentPtr->getId()) + "::" + path;
    }

    return path;
}

int Node::getId() const {
    return id;
}

void Node::setId(int id) {
    Node::id = id;
}

int Node::getIDFromGraphMLFullPath(std::string fullPath)
{
    //Find the location of the last n
    int nIndex = -1;

    int pathLen = (int) fullPath.size(); //Casting to signed to ensure loop termination
    for(int i = pathLen-1; i >= 0; i--){
        if(fullPath[i] == 'n'){
            nIndex = i;
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
