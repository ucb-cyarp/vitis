//
// Created by Christopher Yarp on 6/26/18.
//

#include "EnabledSubSystem.h"

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
            throw std::runtime_error("EnabledInput Found with " + std::to_string(enableArcs.size()) + " Enable arcs");
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
            throw std::runtime_error("EnabledOutput Found with " + std::to_string(enableArcs.size()) + " Enable arcs");
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

