//
// Created by Christopher Yarp on 6/26/18.
//

#include "General/GeneralHelper.h"
#include "General/GraphAlgs.h"
#include "EnabledSubSystem.h"

#include "GraphMLTools/GraphMLHelper.h"
#include "NodeFactory.h"

#include <map>

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

std::shared_ptr<OutputPort> EnabledSubSystem::getEnableSrc() {
    std::shared_ptr<OutputPort> srcPort = nullptr;

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
            (*it)->shallowCloneWithChildren(clonedNode, nodeCopies, origToCopyNode, copyToOrigNode); //Use the copied node as the parent
        }
    }
}

void EnabledSubSystem::extendContext(std::vector<std::shared_ptr<Node>> &new_nodes,
                                     std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                     std::vector<std::shared_ptr<Arc>> &new_arcs,
                                     std::vector<std::shared_ptr<Arc>> &deleted_arcs) {

    //==== Get the enable src before removing enabled inputs (in case all are removed) ====
    std::shared_ptr<OutputPort> enabledSystemDriver = getEnableSrc();

    //==== Get set of nodes to expand context to at inputs ====
    std::set<std::shared_ptr<Node>> extendedContext;

    //Iterate though all of the input node's input ports
    std::map<std::shared_ptr<Arc>, bool> inputMarks;
    for(unsigned long i = 0; i<enabledInputs.size(); i++){
        //Enabled inputs should only have 1 standard input.  Should be caught durring validation if otherwise.
        std::set<std::shared_ptr<Node>> moreExtendedContextNodes =  GraphAlgs::scopedTraceBackAndMark(enabledInputs[i]->getInputPort(0), inputMarks); //Reuse the same inputMarks map.  See docs for scopedTraceBackAndMark
        extendedContext.insert(moreExtendedContextNodes.begin(), moreExtendedContextNodes.end());
    }

    std::vector<std::shared_ptr<EnableInput>> enableInputsToRemove;
    //==== Remove input nodes connected to a node in the extended context, rewire ====
    for(unsigned long i = 0; i<enabledInputs.size(); i++){
        std::set<std::shared_ptr<Arc>> inputArcs = enabledInputs[i]->getInputPort(0)->getArcs(); //There should only be 1 input arc
        if(inputArcs.size() != 1){
            throw std::runtime_error("Enable Input Port with " + GeneralHelper::to_string(inputArcs.size()) + " incoming arcs found, expected 1");
        }
        std::shared_ptr<Arc> inputArc = *inputArcs.begin();

        std::shared_ptr<OutputPort> srcPort = inputArc->getSrcPort();

        std::shared_ptr<Node> srcNode = srcPort->getParent();

        if(extendedContext.find(srcNode) != extendedContext.end()){
            //The src node is in the extended context
            //Rewire the outputs of the enabled input to the output port of the src node
            std::shared_ptr<OutputPort> sourcePort = inputArc->getSrcPort();

            std::set<std::shared_ptr<Arc>> outputArcs = enabledInputs[i]->getOutputPort(0)->getArcs();
            for(auto it = outputArcs.begin(); it != outputArcs.end(); it++){
                (*it)->setSrcPortUpdateNewUpdatePrev(srcPort);
            }

            //Remove this enable input port from the enabled subsystem.
            enableInputsToRemove.push_back(enabledInputs[i]);
        }
    }

    for(unsigned long i = 0; i<enableInputsToRemove.size(); i++){
        removeEnableInput(enableInputsToRemove[i]);
        removeChild(enableInputsToRemove[i]);
        deleted_nodes.push_back(enableInputsToRemove[i]);
    }

    //==== Move nodes into enabled subsystem, replicating subsystem hierarchy ====
    //Replicating subsystem hierarchy involves finding the parent of the enabled subsystem and tracing up from the node to be moved
    //Replicate subsystems (or find if they do not already exist)
    //Move node

    //==== Create new input nodes for arcs between a node not in the extended context to a node in the extended context ====

    //==== Get set of nodes to expand context to at outputs ====


}