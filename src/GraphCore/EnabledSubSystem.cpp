//
// Created by Christopher Yarp on 6/26/18.
//

#include <General/GeneralHelper.h>
#include "EnabledSubSystem.h"

#include "GraphMLTools/GraphMLHelper.h"
#include "NodeFactory.h"

EnabledSubSystem::EnabledSubSystem() {
}

EnabledSubSystem::EnabledSubSystem(std::shared_ptr<SubSystem> parent) : SubSystem(parent) {
}

std::string EnabledSubSystem::labelStr() {
    std::string label = Node::labelStr();

    label += "\nType: Enabled Subsystem";

    return label;
}

void EnabledSubSystem::validate() {
    Node::validate();

    //Check that the src of the EnableNodes all have the same src (and not nullptr)
    std::shared_ptr<Port> srcPort = getEnableSrc();
    if(srcPort == nullptr){
        throw std::runtime_error("EnabledSubsystem could not find an Enable Source");
    }

    unsigned long enableInputsLen = enabledInputs.size();
    for(unsigned long i = 0; i<enableInputsLen; i++){
        std::set<std::shared_ptr<Arc>> enableArcs = enabledInputs[i]->getEnablePort()->getArcs();
        if(enableArcs.size() != 1){
            throw std::runtime_error("EnabledInput Found with " + GeneralHelper::to_string(enableArcs.size()) + " Enable arcs");
        }else{
            if((*enableArcs.begin())->getSrcPort() != srcPort){
                throw std::runtime_error("EnabledInput Found with with different Enable src");
            }
        }
    }

    unsigned long enableOutputLen = enabledOutputs.size();
    for(unsigned long i = 0; i<enableOutputLen; i++){
        std::set<std::shared_ptr<Arc>> enableArcs = enabledOutputs[i]->getEnablePort()->getArcs();
        if(enableArcs.size() != 1){
            throw std::runtime_error("EnabledOutput Found with " + GeneralHelper::to_string(enableArcs.size()) + " Enable arcs");
        }else{
            if((*enableArcs.begin())->getSrcPort() != srcPort){
                throw std::runtime_error("EnabledOutput Found with with different Enable src");
            }
        }
    }
}

std::shared_ptr<Port> EnabledSubSystem::getEnableSrc() {
    std::shared_ptr<Port> srcPort = nullptr;

    if(enabledInputs.size()>0){
        std::set<std::shared_ptr<Arc>> enableArcs = enabledInputs[0]->getEnablePort()->getArcs();
        if(enableArcs.size() != 0){
            srcPort = (*(enableArcs.begin()))->getSrcPort();
            if(srcPort == nullptr){
                //throw std::runtime_error("First EnableInput of Enable Subsystem has no Enable Src Port");
                return nullptr;
            }
        }else{
            //throw std::runtime_error("First EnableInput of Enable Subsystem has no Enable Arc");
            return nullptr;
        }
    }else if(enabledOutputs.size()>0){
        std::set<std::shared_ptr<Arc>> enableArcs = enabledOutputs[0]->getEnablePort()->getArcs();
        if(enableArcs.size() != 0){
            srcPort = (*(enableArcs.begin()))->getSrcPort();
            if(srcPort == nullptr){
                //throw std::runtime_error("First EnableOutput of Enable Subsystem has no Enable Src Port");
                return nullptr;
            }
        }else{
            //throw std::runtime_error("First EnableOutput of Enable Subsystem has no Enable Arc");
            return nullptr;
        }
    }else{
        //throw std::runtime_error("EnableSubsystem does not have any EnableInputs or EnableOutputs");
        return nullptr;
    }

    return srcPort;
}

void EnabledSubSystem::addEnableInput(std::shared_ptr<EnableInput> &input) {
    enabledInputs.push_back(input);
}

void EnabledSubSystem::addEnableOutput(std::shared_ptr<EnableOutput> &output) {
    enabledOutputs.push_back(output);
}

void EnabledSubSystem::removeEnableInput(std::shared_ptr<EnableInput> &input) {
    //Using strategy from https://en.cppreference.com/w/cpp/container/vector/erase

    for (auto enableInput = enabledInputs.begin(); enableInput != enabledInputs.end(); ) {
        if (*enableInput == input) {
            enableInput = enabledInputs.erase(enableInput);
        } else {
            enableInput++;
        }
    }
}

void EnabledSubSystem::removeEnableOutput(std::shared_ptr<EnableOutput> &output) {
    //Using strategy from https://en.cppreference.com/w/cpp/container/vector/erase

    for (auto enableOutput = enabledOutputs.begin(); enableOutput != enabledOutputs.end(); ) {
        if (*enableOutput == output) {
            enableOutput = enabledOutputs.erase(enableOutput);
        } else {
            enableOutput++;
        }
    }
}

std::vector<std::shared_ptr<EnableInput>> EnabledSubSystem::getEnableInputs() const {
    return enabledInputs;
}

std::vector<std::shared_ptr<EnableOutput>> EnabledSubSystem::getEnableOutputs() const {
    return enabledOutputs;
}

xercesc::DOMElement *
EnabledSubSystem::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Performs the same function as the Subsystem emit GraphML except that the block type is changed

    //Emit the basic info for this node
    xercesc::DOMElement *thisNode = emitGraphMLBasics(doc, graphNode);

    if (include_block_node_type){
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Enabled Subsystem");
    }

    emitGramphMLSubgraphAndChildren(doc, thisNode);

    return thisNode;
}

EnabledSubSystem::EnabledSubSystem(std::shared_ptr<SubSystem> parent, EnabledSubSystem* orig) : SubSystem(parent, orig) {
    //Do  not copy the Enable Input and Output Lists
}

std::shared_ptr<Node> EnabledSubSystem::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<EnabledSubSystem>(parent, this);
}

void EnabledSubSystem::shallowCloneWithChildren(std::shared_ptr<SubSystem> parent,
                                                std::vector<std::shared_ptr<Node>> &nodeCopies,
                                                std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode,
                                                std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode) {
    //Copy this node
    std::shared_ptr<EnabledSubSystem> clonedNode = std::dynamic_pointer_cast<EnabledSubSystem>(shallowClone(parent)); //This is a subsystem so we can cast to a subsystem pointer

    //Put into vectors and maps
    nodeCopies.push_back(clonedNode);
    origToCopyNode[shared_from_this()] = clonedNode;
    copyToOrigNode[clonedNode] = shared_from_this();

    //Copy children
    for(auto it = children.begin(); it != children.end(); it++){
        std::shared_ptr<EnableInput> enabledInput = GeneralHelper::isType<Node, EnableInput>(*it);
        std::shared_ptr<EnableOutput> enabledOutput = GeneralHelper::isType<Node, EnableOutput>(*it);

        if(enabledInput != nullptr){
            std::shared_ptr<EnableInput> enableInputCopy = std::static_pointer_cast<EnableInput>(enabledInput->shallowClone(clonedNode));

            nodeCopies.push_back(enableInputCopy);
            origToCopyNode[enabledInput] = enableInputCopy;
            copyToOrigNode[enableInputCopy] = enabledInput;

            clonedNode->enabledInputs.push_back(enableInputCopy);
        }else if(enabledOutput != nullptr){
            std::shared_ptr<EnableOutput> enableOutputCopy = std::static_pointer_cast<EnableOutput>(enabledOutput->shallowClone(clonedNode));

            nodeCopies.push_back(enableOutputCopy);
            origToCopyNode[enabledOutput] = enableOutputCopy;
            copyToOrigNode[enableOutputCopy] = enabledOutput;

            clonedNode->enabledOutputs.push_back(enableOutputCopy);
        }else {
            //Recursive call to this function
            shallowCloneWithChildren(clonedNode, nodeCopies, origToCopyNode, copyToOrigNode); //Use the copied node as the parent
        }
    }
}
