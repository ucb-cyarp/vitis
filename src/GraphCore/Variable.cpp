//
// Created by Christopher Yarp on 8/8/18.
//

#include <stdexcept>
#include "Variable.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>

Variable::Variable() : name("") {

}

Variable::Variable(std::string name, DataType dataType) : name(name), dataType(dataType) {

}

Variable::Variable(std::string name, DataType dataType, std::vector<NumericValue> initValue) : name(name), dataType(dataType), initValue(initValue){

}

std::string Variable::getCVarName(bool imag) {
    if(imag && !dataType.isComplex()){
        throw std::runtime_error("Trying to generate imaginary component declaration for DataType that is not complex");
    }

    std::string nameReplaceSpace = name;

    //Replace spaces with underscores
    //shortcut for doing this from https://stackoverflow.com/questions/2896600/how-to-replace-all-occurrences-of-a-character-in-string
    std::replace(nameReplaceSpace.begin(), nameReplaceSpace.end(), ' ', '_');

    return nameReplaceSpace + (imag ? VITIS_C_VAR_NAME_IM_SUFFIX : VITIS_C_VAR_NAME_RE_SUFFIX);
}

std::string Variable::getCVarDecl(bool imag, bool includeWidth, bool includeInit) {

    std::string decl = dataType.getCPUStorageType().toString(DataType::StringStyle::C, includeWidth) + " " + getCVarName(imag);

    if(includeInit){
        if(initValue.size() == 0) {
            std::cerr << "Warning: Variable without Initial Value Emitted: " + name << std::endl;
        }else if(initValue.size() == 1) {
            decl += " = ";

            //TODO: Refactor with Constant Node Emit (Same Logic)
            decl += "(";

            //Emit datatype (the CPU type used for storage)
            decl += "(" + dataType.toString(DataType::StringStyle::C) + ") ";
            //Emit value
            decl += initValue[0].toStringComponent(imag, dataType);
            decl += ")";
        }else{
            //Emit an array
            decl += " = ";

            std::string storageTypeStr = dataType.toString(DataType::StringStyle::C);

            decl += "{";

            //Emit datatype (the CPU type used for storage)
            decl += "(" + storageTypeStr + ") ";
            //Emit Value
            decl += initValue[0].toStringComponent(imag, dataType); //Convert to the real type, not the CPU storage type

            for(unsigned long i = 0; i<initValue.size(); i++){
                //Emit datatype (the CPU type used for storage)
                decl += ", (" + storageTypeStr + ") ";
                //Emit Value
                decl += initValue[i].toStringComponent(imag, dataType); //Convert to the real type, not the CPU storage type
            }

            decl += "}";
        }
    }

    return decl;
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

