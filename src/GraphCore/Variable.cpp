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

Variable::Variable() : name("") {

}

Variable::Variable(std::string name, DataType dataType) : name(name), dataType(dataType), atomicVar(false) {

}

Variable::Variable(std::string name, DataType dataType, std::vector<NumericValue> initValue) : name(name), dataType(dataType), initValue(initValue), atomicVar(false){

}

Variable::Variable(std::string name, DataType dataType, std::vector<NumericValue> initValue, bool volatileVar) : name(name), dataType(dataType), initValue(initValue), atomicVar(volatileVar){

}

std::string Variable::getCVarName(bool imag) {
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



    return nameReplaceSpace + (imag ? VITIS_C_VAR_NAME_IM_SUFFIX : VITIS_C_VAR_NAME_RE_SUFFIX);
}

std::string Variable::getCVarDecl(bool imag, bool includeDimensions, bool includeInit, bool includeArray, bool includeRef) {

    DataType cpuStorageType = dataType.getCPUStorageType();
    std::string decl = (atomicVar ? "_Atomic " : "") + cpuStorageType.toString(DataType::StringStyle::C, false, false) + (includeRef ? " &" : " ") + getCVarName(imag);

    if(!dataType.isScalar() && includeArray){
        decl += dataType.dimensionsToString(includeDimensions);
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
    return (atomicVar ? "_Atomic " : "") + cpuStorageType.toString(DataType::StringStyle::C, false, false) + " *" + getCVarName(imag);
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

