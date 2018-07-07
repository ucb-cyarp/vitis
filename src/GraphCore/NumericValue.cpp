//
// Created by Christopher Yarp on 7/6/18.
//

#include "NumericValue.h"

NumericValue::NumericValue() : realInt(0), imagInt(0), complexDouble(std::complex<double>(0, 0)), complex(false), fractional(true){

}

NumericValue::NumericValue(long int realInt, long int imagInt, std::complex<double> complexDouble, bool complex, bool fractional) :
realInt(realInt), imagInt(imagInt), complexDouble(complexDouble), complex(complex), fractional(fractional) {

}

long NumericValue::getRealInt() const {
    return realInt;
}

void NumericValue::setRealInt(long realInt) {
    NumericValue::realInt = realInt;
}

long NumericValue::getImagInt() const {
    return imagInt;
}

void NumericValue::setImagInt(long imagInt) {
    NumericValue::imagInt = imagInt;
}

std::complex<double> NumericValue::getComplexDouble() const {
    return complexDouble;
}

void NumericValue::setComplexDouble(std::complex<double> complexDouble) {
    NumericValue::complexDouble = complexDouble;
}

bool NumericValue::isComplex() const {
    return complex;
}

void NumericValue::setComplex(bool complex) {
    NumericValue::complex = complex;
}

bool NumericValue::isFractional() const {
    return fractional;
}

void NumericValue::setFractional(bool fractional) {
    NumericValue::fractional = fractional;
}

std::string NumericValue::toString() {
    std::string val;
    if(fractional){
        val = std::to_string(complexDouble.real());

        if(complex){
            val = val + " + " + std::to_string(complexDouble.imag()) + "i";
        }
    } else {
        val = std::to_string(realInt);

        if(complex){
            val = val + " + " + std::to_string(imagInt) + "i";
        }
    }

    return val;
}

std::vector<NumericValue> NumericValue::parseXMLString(std::string str) {
    bool array = str.find("[") != std::string::npos;
    bool fractional = str.find(".") != std::string::npos;

    bool complex = str.find("i") != std::string::npos || str.find("j") != std::string::npos;

    //TODO: Finish
    return std::vector<NumericValue>();
}
