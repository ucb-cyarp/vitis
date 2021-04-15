//
// Created by Christopher Yarp on 10/23/18.
//

#include "ContextRoot.h"
#include "Node.h"
#include <algorithm>
#include "DummyReplica.h"

void ContextRoot::addSubContextNode(unsigned long subContext, std::shared_ptr<Node> node) {
    //Create sub-vectors if needed
    while(nodesInSubContexts.size() < (subContext+1)){
        nodesInSubContexts.push_back(std::vector<std::shared_ptr<Node>>());
    }

    nodesInSubContexts[subContext].push_back(node);
}

std::vector<std::shared_ptr<Node>> ContextRoot::getSubContextNodes(unsigned long subContext) {
    //Create sub-vectors if needed (may happen if no nodes are in the requested subcontext)
    while(nodesInSubContexts.size() < (subContext+1)){
        nodesInSubContexts.push_back(std::vector<std::shared_ptr<Node>>());
    }

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

std::set<int> ContextRoot::partitionsInContext(){
    std::set<int> partitions;
    for(auto subcontextNodes = nodesInSubContexts.begin(); subcontextNodes != nodesInSubContexts.end(); subcontextNodes++){
        for(auto node = subcontextNodes->begin(); node != subcontextNodes->end(); node++) {
            partitions.insert((*node)->getPartitionNum());
        }
    }

    return partitions;
}

std::map<int, std::shared_ptr<DummyReplica>> ContextRoot::getDummyReplicas() const {
    return dummyReplicas;
}

void ContextRoot::setDummyReplicas(const std::map<int, std::shared_ptr<DummyReplica>> &dummyReplicas) {
    ContextRoot::dummyReplicas = dummyReplicas;
}

std::shared_ptr<DummyReplica> ContextRoot::getDummyReplica(int partition) {
    auto dummyReplica = dummyReplicas.find(partition);

    if(dummyReplica != dummyReplicas.end()){
        return dummyReplica->second;
    }
    return nullptr;
}

void ContextRoot::setDummyReplica(int partition, std::shared_ptr<DummyReplica> dummyReplica){
    dummyReplicas[partition] = dummyReplica;
}

bool ContextRoot::allowFIFOAbsorption(){
    return false;
}