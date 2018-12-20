//
// Created by Christopher Yarp on 2018-12-19.
//

#include "SchedParams.h"
#include <stdexcept>

SchedParams::SchedType SchedParams::parseSchedTypeStr(std::string str) {
    if(str == "BOTTOM_UP" || str == "bottomUp" || str == "bottomup"){
        return SchedType::BOTTOM_UP;
    }else if(str == "TOPOLOGICAL" || str == "topological"){
        return SchedType::TOPOLOGICAL;
    }else if(str == "TOPOLOGICAL_CONTEXT" || str == "topological_context") {
        return SchedType::TOPOLOGICAL_CONTEXT;
    }else{
        throw std::runtime_error("Unable to parse Scheduler: " + str);
    }
}

std::string SchedParams::schedTypeToString(SchedParams::SchedType schedType) {
    if(schedType == SchedType::BOTTOM_UP){
        return "BOTTOM_UP";
    }else if(schedType == SchedType::TOPOLOGICAL) {
        return "TOPOLOGICAL";
    }else if(schedType == SchedType::TOPOLOGICAL_CONTEXT){
        return "TOPOLOGICAL_CONTEXT";
    }else{
        throw std::runtime_error("Unknown scheduler");
    }
}