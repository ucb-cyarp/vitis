//
// Created by Christopher Yarp on 6/26/18.
//

#include "DataType.h"

bool DataType::isFloatingPt() const {
    return floatingPt;
}

void DataType::setFloatingPt(bool floatingPt) {
    DataType::floatingPt = floatingPt;
}

bool DataType::isSignedType() const {
    return signedType;
}

void DataType::setSignedType(bool signedType) {
    DataType::signedType = signedType;
}

bool DataType::isComplex() const {
    return complex;
}

void DataType::setComplex(bool complex) {
    DataType::complex = complex;
}

int DataType::getTotalBits() const {
    return totalBits;
}

void DataType::setTotalBits(int totalBits) {
    DataType::totalBits = totalBits;
}

int DataType::getFractionalBits() const {
    return fractionalBits;
}

void DataType::setFractionalBits(int fractionalBits) {
    DataType::fractionalBits = fractionalBits;
}

bool DataType::operator==(const DataType &rhs) const {
    return floatingPt == rhs.floatingPt &&
           signedType == rhs.signedType &&
           complex == rhs.complex &&
           totalBits == rhs.totalBits &&
           fractionalBits == rhs.fractionalBits;
}

bool DataType::operator!=(const DataType &rhs) const {
    return !(rhs == *this);
}

DataType::DataType() : signedType(false), complex(false), floatingPt(false), totalBits(0), fractionalBits(0){
}

DataType::DataType(bool floatingPt, bool signedType, bool complex, int totalBits, int fractionalBits) : floatingPt(floatingPt), signedType(signedType), complex(complex), totalBits(totalBits), fractionalBits(fractionalBits){

}
