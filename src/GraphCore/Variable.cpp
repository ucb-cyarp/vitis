//
// Created by Christopher Yarp on 8/8/18.
//

#include <stdexcept>
#include "Variable.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include "General/GeneralHelper.h"
#include "General/EmitterHelpers.h"

Variable::Variable() : name(""), atomicVar(false), inStateStructure(false), overrideType(""){

}

Variable::Variable(std::string name, DataType dataType) : name(name), dataType(dataType), atomicVar(false), inStateStructure(false), overrideType("") {

}

Variable::Variable(std::string name, DataType dataType, std::vector<NumericValue> initValue) : name(name), dataType(dataType), initValue(initValue), atomicVar(false), inStateStructure(false), overrideType(""){

}

Variable::Variable(std::string name, DataType dataType, std::vector<NumericValue> initValue, bool volatileVar) : name(name), dataType(dataType), initValue(initValue), atomicVar(volatileVar), inStateStructure(false), overrideType(""){

}

Variable::Variable(std::string name, DataType dataType, std::vector<NumericValue> initValue, bool volatileVar, bool inStateStructure) : name(name), dataType(dataType), initValue(initValue), atomicVar(volatileVar), inStateStructure(inStateStructure), overrideType(""){

}

std::string Variable::getCVarName(bool imag, bool includeStateStructureDeref, bool stateStructureIsPtr) {
    if(imag && !dataType.isComplex()){
        throw std::runtime_error("Trying to generate imaginary component declaration for DataType that is not complex");
    }

    std::string nameReplaceSpace = name;

    if(name == ""){
        nameReplaceSpace = "v";
    }else if(isdigit(name[0])){
        //Cannot start variable name with a digit
        nameReplaceSpace = "_" + nameReplaceSpace;
    }

    //Replace spaces with underscores
    //shortcut for doing this from https://stackoverflow.com/questions/2896600/how-to-replace-all-occurrences-of-a-character-in-string
    std::replace(nameReplaceSpace.begin(), nameReplaceSpace.end(), ' ', '_');
    std::replace(nameReplaceSpace.begin(), nameReplaceSpace.end(), '\n', '_');
    std::replace(nameReplaceSpace.begin(), nameReplaceSpace.end(), '-', '_');

    std::string structAccess = "";
    if(includeStateStructureDeref && inStateStructure){
        structAccess = VITIS_STATE_STRUCT_NAME + (stateStructureIsPtr ? std::string("->") : std::string("."));
    }

    return structAccess + nameReplaceSpace + (imag ? VITIS_C_VAR_NAME_IM_SUFFIX : VITIS_C_VAR_NAME_RE_SUFFIX);
}

std::string Variable::getCVarDecl(bool imag, bool includeDimensions, bool includeInit, bool includeArray, bool includeRef,
                                  bool alignTo, std::string alignment) {

    DataType cpuStorageType = dataType.getCPUStorageType();
    std::string typeName;
    if(overrideType.empty()){
        //Use the standard datatype obj
        typeName = cpuStorageType.toString(DataType::StringStyle::C, false, false);
    }else{
        typeName = overrideType;
    }

    //Do not include a structure name when declaring as, if a variable is part of the state structure, its declaration will be inside the structure defn
    std::string decl = (atomicVar ? "_Atomic " : "") + typeName + (includeRef ? " &" : " ") + getCVarName(imag, false, false);

    if(!dataType.isScalar() && includeArray && overrideType.empty()){
        decl += dataType.dimensionsToString(includeDimensions);
    }

    if(alignTo){
        decl += " __attribute__ ((aligned (" + alignment + ")))";
    }

    if(includeInit){
        if(initValue.size() == 0) {
            std::cerr << "Warning: Variable without Initial Value Emitted: " + name << std::endl;
        }else if(initValue.size() == 1) {
            decl += " = ";

            if(dataType.isScalar()) {
                //TODO: Refactor with Constant Node Emit (Same Logic)
                decl += "(";

                //Emit datatype (the CPU type used for storage)
                decl += "(" + dataType.toString(DataType::StringStyle::C) + ") ";
                //Emit value
                decl += initValue[0].toStringComponent(imag, dataType);
                decl += ")";
            }else{
                //If a vector/matrix, broadcast the value
                std::vector<int> dimensions = dataType.getDimensions();

                decl += EmitterHelpers::arrayLiteral(dimensions, initValue[0].toStringComponent(imag, dataType));
            }
        }else{
            //Emit an array
            decl += " = ";

            std::vector<int> dimensions = dataType.getDimensions();
            decl += EmitterHelpers::arrayLiteral(dimensions, initValue, imag, dataType, dataType);
        }
    }

    return decl;
}

std::string Variable::getCPtrDecl(bool imag) {
    DataType cpuStorageType = dataType.getCPUStorageType();

    std::string typeName;
    if(overrideType.empty()){
        //Use the standard datatype obj
        typeName = cpuStorageType.toString(DataType::StringStyle::C, false, false);
    }else{
        typeName = overrideType;
    }

    return (atomicVar ? "_Atomic " : "") + typeName + " *" + getCVarName(imag, false, false);
}

std::string Variable::getName() const {
    return name;
}

void Variable::setName(const std::string &name) {
    Variable::name = name;
}

DataType Variable::getDataType() const {
    return dataType;
}

void Variable::setDataType(const DataType &dataType) {
    Variable::dataType = dataType;
}

std::vector<NumericValue> Variable::getInitValue() const {
    return initValue;
}

void Variable::setInitValue(const std::vector<NumericValue> &initValue) {
    Variable::initValue = initValue;
}

bool Variable::isAtomicVar() const {
    return atomicVar;
}

void Variable::setAtomicVar(bool atomicVar) {
    Variable::atomicVar = atomicVar;
}

bool Variable::operator==(const Variable &rhs) const {
    return name == rhs.name &&
           dataType == rhs.dataType &&
           initValue == rhs.initValue &&
           atomicVar == rhs.atomicVar;
}

bool Variable::operator!=(const Variable &rhs) const {
    return !(rhs == *this);
}

const std::string Variable::getOverrideType() const {
    return overrideType;
}

void Variable::setOverrideType(const std::string &overrideType) {
    Variable::overrideType = overrideType;
}

