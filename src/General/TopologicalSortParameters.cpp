//
// Created by Christopher Yarp on 2019-03-29.
//

#include "TopologicalSortParameters.h"
#include <stdexcept>

TopologicalSortParameters::Heuristic TopologicalSortParameters::getHeuristic() const {
    return heuristic;
}

void TopologicalSortParameters::setHeuristic(TopologicalSortParameters::Heuristic heuristic) {
    TopologicalSortParameters::heuristic = heuristic;
}

int TopologicalSortParameters::getRandSeed() const {
    return randSeed;
}

void TopologicalSortParameters::setRandSeed(int randSeed) {
    TopologicalSortParameters::randSeed = randSeed;
}

TopologicalSortParameters::TopologicalSortParameters() : heuristic(Heuristic::BFS), randSeed(0) {

}

TopologicalSortParameters::TopologicalSortParameters(TopologicalSortParameters::Heuristic heuristic) : heuristic(heuristic), randSeed(0){

}

TopologicalSortParameters::TopologicalSortParameters(TopologicalSortParameters::Heuristic heuristic, unsigned long randSeed) : heuristic(heuristic), randSeed(randSeed){

}

TopologicalSortParameters::Heuristic TopologicalSortParameters::parseHeuristicStr(std::string str) {
    if(str == "BFS" || str == "bfs"){
        return Heuristic::BFS;
    }else if(str == "DFS" || str == "dfs"){
        return Heuristic::DFS;
    }else if(str == "RANDOM" || str == "random" || str == "RAND" || str == "rand") {
        return Heuristic::RANDOM;
    }else{
        throw std::runtime_error("Unable to parse Heuristic: " + str);
    }
}

std::string TopologicalSortParameters::heuristicToString(TopologicalSortParameters::Heuristic schedType) {
    if(schedType == Heuristic::BFS){
        return "BFS";
    }else if(schedType == Heuristic::DFS) {
        return "DFS";
    }else if(schedType == Heuristic::RANDOM){
        return "RANDOM";
    }else{
        throw std::runtime_error("Unknown heuristic");
    }
}