//
// Created by Christopher Yarp on 6/25/18.
//

#include "Port.h"
#include "Node.h"

Port::Port() : portNum(0), name(""), parent(nullptr)  {

}

Port::Port(Node* parent, int portNum) : portNum(portNum), name(""), parent(parent) {

}

void Port::addArc(std::shared_ptr<Arc> arc) {
    arcs.insert(arc);
}

void Port::removeArc(std::shared_ptr<Arc> arc) {

    //It looks like the less operator provided by owner_less is enough to show equivalence (neither object is less than the other)
    //https://en.cppreference.com/w/cpp/container/set
    //Therefore, I think using the builtin erase function with owner_less should work for weak_ptrs to the same object (the shared pointer is converted to a weak_ptr)
    arcs.erase(arc);
}

void Port::removeArc(std::weak_ptr<Arc> arc) {

    //It looks like the less operator provided by owner_less is enough to show equivalence (neither object is less than the other)
    //https://en.cppreference.com/w/cpp/container/set
    //Therefore, I think using the builtin erase function with owner_less should work for weak_ptrs to the same object (the shared pointer is converted to a weak_ptr)
    arcs.erase(arc);
}

std::set<std::weak_ptr<Arc>, std::owner_less<std::weak_ptr<Arc>>> Port::getArcsRaw() {
    return arcs;
}

std::set<std::shared_ptr<Arc>> Port::getArcs() {
    std::set<std::shared_ptr<Arc>> convertedPtrs;

    for(auto arc = arcs.begin(); arc != arcs.end(); arc++){
        //Stale pointers in the port should not occur since the arc destructor removes pointers from the port
        if(arc->expired()){
            throw std::runtime_error("A weak pointer to an Arc in a port expired.  This should not occur");
        }
        {
            convertedPtrs.insert(arc->lock());
        }
    }
    return convertedPtrs;
}

void Port::setArcs(std::set<std::weak_ptr<Arc>, std::owner_less<std::weak_ptr<Arc>>> arcs) {
    Port::arcs = arcs;
}

std::set<std::weak_ptr<Arc>, std::owner_less<std::weak_ptr<Arc>>>::iterator Port::arcsBegin() {
    return arcs.begin();
}

std::set<std::weak_ptr<Arc>, std::owner_less<std::weak_ptr<Arc>>>::iterator Port::arcsEnd() {
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

std::shared_ptr<Port> Port::getSharedPointer() {
    if(parent != nullptr) {
        return std::shared_ptr<Port>(parent->getSharedPointer(), this);
    }
    else {
        //Hopefully should not happen but just in case
        //return std::shared_ptr<Port>(nullptr);
        throw std::runtime_error("Pointer requested from port that has no parent");
    }
}

unsigned long Port::numArcs() {
    return arcs.size();
}

const std::string Port::getName() {
    return name;
}

void Port::setName(const std::string &name) {
    Port::name = name;
}
