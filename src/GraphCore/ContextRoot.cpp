//
// Created by Christopher Yarp on 10/23/18.
//

#include "ContextRoot.h"
#include <algorithm>

void ContextRoot::addSubContextNode(unsigned long subContext, std::shared_ptr<Node> node) {
    //Create sub-vectors if needed
    while(nodesInSubContexts.size() < (subContext+1)){
        nodesInSubContexts.push_back(std::vector<std::shared_ptr<Node>>());
    }

    nodesInSubContexts[subContext].push_back(node);
}

std::vector<std::shared_ptr<Node>> ContextRoot::getSubContextNodes(unsigned long subContext) {
    return nodesInSubContexts[subContext];
}

void ContextRoot::removeSubContextNode(unsigned long subContext, std::shared_ptr<Node> node) {
    nodesInSubContexts[subContext].erase(std::remove(nodesInSubContexts[subContext].begin(), nodesInSubContexts[subContext].end(), node), nodesInSubContexts[subContext].end());
}

std::vector<std::shared_ptr<ContextVariableUpdate>> ContextRoot::getContextVariableUpdateNodes() const {
    return contextVariableUpdateNodes;
}

void ContextRoot::setContextVariableUpdateNodes(
        const std::vector<std::shared_ptr<ContextVariableUpdate>> &contextVariableUpdateNodes) {
    ContextRoot::contextVariableUpdateNodes = contextVariableUpdateNodes;
}

bool ContextRoot::createContextVariableUpdateNodes(std::vector<std::shared_ptr<Node>> &new_nodes,
                                                   std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                   std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                   std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                   bool setContext) {
    //By default, does nothing

    return false;
}

void ContextRoot::addContextVariableUpdateNode(std::shared_ptr<ContextVariableUpdate> contextVariableUpdateNode){
    contextVariableUpdateNodes.push_back(contextVariableUpdateNode);
}

std::map<int, std::shared_ptr<ContextFamilyContainer>> ContextRoot::getContextFamilyContainers() const {
    return contextFamilyContainers;
}

void ContextRoot::setContextFamilyContainers(const std::map<int, std::shared_ptr<ContextFamilyContainer>> &contextFamilyContainers) {
    ContextRoot::contextFamilyContainers = contextFamilyContainers;
}

std::map<int, std::vector<std::shared_ptr<Arc>>> ContextRoot::getContextDriversPerPartition() const {
    return contextDriversPerPartition;
}

void ContextRoot::setContextDriversPerPartition(
        const std::map<int, std::vector<std::shared_ptr<Arc>>> &contextDriversPerPartition) {
    ContextRoot::contextDriversPerPartition = contextDriversPerPartition;
}

std::vector<std::shared_ptr<Arc>> ContextRoot::getContextDriversForPartition(int partitionNum) const {
    auto contextDrivers = contextDriversPerPartition.find(partitionNum);
    if(contextDrivers == contextDriversPerPartition.end()){
        return std::vector<std::shared_ptr<Arc>>();
    }else{
        return contextDrivers->second;
    }
}

void ContextRoot::addContextDriverArcsForPartition(std::vector<std::shared_ptr<Arc>> drivers, int partitionNum){
    //Note that map[] returns a reference so direct insertion should be possible
    contextDriversPerPartition[partitionNum].insert(contextDriversPerPartition[partitionNum].end(), drivers.begin(), drivers.end());
}