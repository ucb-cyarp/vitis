//
// Created by Christopher Yarp on 6/25/18.
//

#include <vector>
#include <memory>

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
        inputPorts.push_back(Port(this, Port::PortType::INPUT, i));
    }

    //Set the dst port of the arc, updating the previous port and this one
    arc->setDstPortUpdateNewUpdatePrev(inputPorts[portNum].getSharedPointer());
}

void Node::addOutArcUpdatePrevUpdateArc(int portNum, std::shared_ptr<Arc> arc) {
    //Create the requested port if it does not exist yet
    //TODO: it is assumed that if port n exists, that there are ports 0-n with no holes.  Re-evaluate this assumption
    unsigned long outputPortLen = outputPorts.size();
    for (unsigned long i = outputPortLen; i <= portNum; i++) {
        outputPorts.push_back(Port(this, Port::PortType::OUTPUT, i));
    }

    //Set the src port of the arc, updating the previous port and this one
    arc->setSrcPortUpdateNewUpdatePrev(outputPorts[portNum].getSharedPointer());
}

std::shared_ptr<Node> Node::getSharedPointer() {
    return shared_from_this();
}