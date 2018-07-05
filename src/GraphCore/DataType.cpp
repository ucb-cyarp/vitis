//
// Created by Christopher Yarp on 6/26/18.
//

#include "DataType.h"
#include <regex>

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

DataType::DataType(std::string str, bool complex) : complex(complex) {
    //Note: complex is handled seperatly from the string

    //Floating Point
    if(str == "single"){
        totalBits = 32;
        fractionalBits = 0;
        signedType = true;
        floatingPt = true;
    }else if(str == "double"){
        totalBits = 64;
        fractionalBits = 0;
        signedType = true;
        floatingPt = true;
    }
    //Logical
    else if(str == "boolean"){
        totalBits = 1;
        fractionalBits = 0;
        signedType = false;
        floatingPt = false;
    }else if(str == "logical"){
        totalBits = 1;
        fractionalBits = 0;
        signedType = false;
        floatingPt = false;
    }
    //Unsigned Ints
    else if(str == "uint8"){
        totalBits = 8;
        fractionalBits = 0;
        signedType = false;
        floatingPt = false;
    }else if(str == "uint16"){
        totalBits = 16;
        fractionalBits = 0;
        signedType = false;
        floatingPt = false;
    }else if(str == "uint32"){
        totalBits = 32;
        fractionalBits = 0;
        signedType = false;
        floatingPt = false;
    }else if(str == "uint64"){
        totalBits = 64;
        fractionalBits = 0;
        signedType = false;
        floatingPt = false;
    }
    //Signed Ints
    else if(str == "int8"){
        totalBits = 8;
        fractionalBits = 0;
        signedType = true;
        floatingPt = false;
    }else if(str == "int16"){
        totalBits = 16;
        fractionalBits = 0;
        signedType = true;
        floatingPt = false;
    }else if(str == "int32"){
        totalBits = 32;
        fractionalBits = 0;
        signedType = true;
        floatingPt = false;
    }else if(str == "int64"){
        totalBits = 64;
        fractionalBits = 0;
        signedType = true;
        floatingPt = false;
    }
    //Fixed point type
    else{
        //2 basic forms we may see:
        //fixdt(1,16,13) or sfix16_En13
        //fixdt(0,16,13) or ufix16_En13
        //fixdt(signed, bits, fractionalBits) or [s,u]fix{bits}_En{fractionalBits}

        //The first form is common when declaring types while the
        //second is more commonly reported by simulink.

        std::string fixdtRegex = "fixdt[(]([0-9]+),([0-9]+),([0-9]+)[)]";
        std::string fixRegex = "([su])fix([0-9]+)_En([0-9]+)";

        std::regex fixdtRegexExpr(fixdtRegex);

        std::smatch matches;
        bool fixdtMatched = std::regex_match(str, matches, fixdtRegexExpr);

        if(!fixdtMatched)
        {
            //It is the other format
            matches = std::smatch(); //Reset
            bool fixMatched = std::regex_match(str, matches, fixdtRegexExpr);

            if(!fixMatched)
            {
                throw std::invalid_argument("String does match one of the expected type formats");
            }
        }

        //Set parameters
        floatingPt = false;

        if(matches.size() < 4) {
            throw std::invalid_argument("String does match one of the expected type formats");
        }

        //First match (0) is the whole string.
        //Submatches happen starting at index 1
        //--The reason why the size is 4 not 3
        std::string match1 = matches[1].str();
        if(match1 == "s" || match1 == "1") {
            signedType = true;
        }else if(match1 == "u" || match1 == "0"){
            signedType = false;
        } else {
            throw std::invalid_argument("String does match one of the expected type formats");
        }

        std::string match2 = matches[2].str();
        totalBits = std::stoi(match2);

        std::string match3 = matches[3].str();
        fractionalBits = std::stoi(match3);
    }
}
