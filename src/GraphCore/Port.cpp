//
// Created by Christopher Yarp on 6/25/18.
//

#include "Port.h"
#include "Node.h"

Port::Port() : portNum(0), type(Port::PortType::INPUT), parent(nullptr)  {

}

Port::Port(Node* parent, Port::PortType type, int portNum) : portNum(portNum), type(type), parent(parent) {

}

void Port::addArc(std::shared_ptr<Arc> arc) {
    arcs.emplace(arc);
}

void Port::removeArc(std::shared_ptr<Arc> arc) {
    arcs.erase(arc);
}

std::set<std::shared_ptr<Arc>> Port::getArcs() {
    return arcs;
}

void Port::setArcs(std::set<std::shared_ptr<Arc>> arcs) {
    Port::arcs = arcs;
}

std::set<std::shared_ptr<Arc>>::iterator Port::arcsBegin() {
    return arcs.begin();
}

std::set<std::shared_ptr<Arc>>::iterator Port::arcsEnd() {
    return arcs.end();
}

std::shared_ptr<Node> Port::getParent() {
    return parent->getSharedPointer();
}

int Port::getPortNum() const {
    return portNum;
}

void Port::setPortNum(int portNum) {
    Port::portNum = portNum;
}

Port::PortType Port::getType() const {
    return type;
}

void Port::setType(Port::PortType type) {
    Port::type = type;
}

std::shared_ptr<Port> Port::getSharedPointer() {
    return std::shared_ptr<Port>(parent->getSharedPointer(), this);
}
