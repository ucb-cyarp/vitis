//
// Created by Christopher Yarp on 6/27/18.
//

#include "MasterUnconnected.h"
#include "GraphCore/NodeFactory.h"

MasterUnconnected::MasterUnconnected() {

}

MasterUnconnected::MasterUnconnected(std::shared_ptr<SubSystem> parent, MasterNode* orig) : MasterNode(parent, orig){

};

std::shared_ptr<Node> MasterUnconnected::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<MasterUnconnected>(parent, this);
}

std::string MasterUnconnected::typeNameStr(){
    return "Master Unconnected";
}