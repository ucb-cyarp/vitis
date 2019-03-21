//
// Created by Christopher Yarp on 7/24/18.
//

#include "VectorFan.h"
#include "VectorFanIn.h"
#include "VectorFanOut.h"
#include "General/ErrorHelpers.h"

VectorFan::VectorFan() {

}

VectorFan::VectorFan(std::shared_ptr<SubSystem> parent) : Node(parent) {

}

std::shared_ptr<VectorFan>
VectorFan::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                             std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    if(dialect == GraphMLDialect::VITIS){
        throw std::runtime_error(ErrorHelpers::genErrorStr("VITIS importer should not call the VectorFan factory, it should call a subclass factory - VectorFan"));
    } else if(dialect == GraphMLDialect::SIMULINK_EXPORT) {
        std::string vectorFanDirection = dataKeyValueMap.at("Direction");
        if(vectorFanDirection == "Fan-In"){
            return VectorFanIn::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
        }else if(vectorFanDirection == "Fan-Out"){
            return VectorFanOut::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown VectorFan Direction - VectorFan"));
        }
    } else
    {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - VectorFan"));
    }


}

bool VectorFan::canExpand() {
    return false;
}

VectorFan::VectorFan(std::shared_ptr<SubSystem> parent, VectorFan* orig) : Node(parent, orig){
    //Nothing else to copy
}
