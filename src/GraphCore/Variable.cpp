//
// Created by Christopher Yarp on 8/8/18.
//

#include <stdexcept>
#include "Variable.h"
#include <stdexcept>

Variable::Variable() : name("") {

}

Variable::Variable(std::string name, DataType dataType) : name(name), dataType(dataType) {

}

std::string Variable::getCVarName(bool imag) {
    if(imag && !dataType.isComplex()){
        throw std::runtime_error("Trying to generate imaginary component declaration for DataType that is not complex");
    }

    return name + (imag ? VITIS_C_VAR_NAME_IM_SUFFIX : VITIS_C_VAR_NAME_RE_SUFFIX);
}

std::string Variable::getCVarDecl(bool imag) {

    return dataType.toString(DataType::StringStyle::C) + " " + getCVarName(imag);
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
