//
// Created by Christopher Yarp on 6/26/18.
//

#include "DataType.h"
#include "General/GeneralHelper.h"
#include <regex>
#include <iostream>

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

int DataType::getWidth() const {
    return width;
}

void DataType::setWidth(int width) {
    DataType::width = width;
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

DataType::DataType() : signedType(false), complex(false), floatingPt(false), totalBits(0), fractionalBits(0), width(1){
}

DataType::DataType(bool floatingPt, bool signedType, bool complex, int totalBits, int fractionalBits, int width) : floatingPt(floatingPt), signedType(signedType), complex(complex), totalBits(totalBits), fractionalBits(fractionalBits), width(width){

}

DataType::DataType(std::string str, bool complex, int width) : complex(complex), width(width) {
    //Note: complex is handled seperatly from the string

    //Floating Point
    if(str == "single" || str == "float"){
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
    else if(str == "boolean" || str == "logical" || str == "bool"){
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

        std::string fixdtRegex = "fixdt[(]\\s*([0-9]+)\\s*,\\s*([0-9]+)\\s*,\\s*([0-9]+)\\s*[)]";
        std::string fixRegex = "([su])fix([0-9]+)_En([0-9]+)";
        std::string fixRegexAbrv = "([su])fix([0-9]+)";

        std::regex fixdtRegexExpr(fixdtRegex);

        std::smatch matches;
        bool fixdtMatched = std::regex_match(str, matches, fixdtRegexExpr);

        bool fixMatched = false;
        bool fixAbrvMatched = false;
        if(!fixdtMatched)
        {
            //It is the other format
            matches = std::smatch(); //Reset
            std::regex fixRegexExpr(fixRegex);
            fixMatched = std::regex_match(str, matches, fixRegexExpr);

            if(!fixMatched)
            {
                matches = std::smatch(); //Reset
                std::regex fixRegexAbrvExpr(fixRegexAbrv);
                fixAbrvMatched = std::regex_match(str, matches, fixRegexAbrvExpr);

                if(!fixAbrvMatched) {
                    throw std::invalid_argument("String does match one of the expected type formats: " + str);
                }
            }
        }

        //Set parameters
        floatingPt = false;

        if ((fixMatched || fixdtMatched) && matches.size() < 4) {
            throw std::invalid_argument("String does match one of the expected type formats: " + str);
        }else if(fixAbrvMatched && matches.size() < 3){
            throw std::invalid_argument("String does match one of the expected type formats: " + str);
        }

        //First match (0) is the whole string.
        //Submatches happen starting at index 1
        //--The reason why the size is 4 not 3
        std::string match1 = matches[1].str();
        if (match1 == "s" || match1 == "1") {
            signedType = true;
        } else if (match1 == "u" || match1 == "0") {
            signedType = false;
        } else {
            throw std::invalid_argument("String does match one of the expected type formats: " + str);
        }

        std::string match2 = matches[2].str();
        totalBits = std::stoi(match2);

        if (fixMatched || fixdtMatched){
            std::string match3 = matches[3].str();
            fractionalBits = std::stoi(match3);
        }else if(fixAbrvMatched){
            fractionalBits = 0;
        }else{
            throw std::invalid_argument("String does match one of the expected type formats: " + str);
        }

    }
}

std::string DataType::toString(StringStyle stringStyle, bool includeWidth) {

    if(floatingPt){
        //Floing point types
        if(totalBits == 32){
            if(stringStyle == StringStyle::SIMULINK) {
                return "single";
            }else if(stringStyle == StringStyle::C){
                std::string str = "float";
                if(width > 1){
                    str += "[";
                    if(includeWidth){
                        str += std::to_string(width);
                    }
                    str += "]";
                }
                return str;
            }else{
                throw std::runtime_error("Unknown DataType String Style");
            }
        }else if(totalBits == 64){
            if(stringStyle == StringStyle::SIMULINK){
                return "double";
            }else if(stringStyle == StringStyle::C){
                std::string str = "double";
                if(width > 1){
                    str += "[";
                    if(includeWidth){
                        str += std::to_string(width);
                    }
                    str += "]";
                }
                return str;
            }else{
                throw std::runtime_error("Unknown DataType String Style");
            }
        }
        else{
            throw std::runtime_error("Floating point type which is not a \"single\" or \"double\"");
        }
    } else if(fractionalBits == 0 && (totalBits == 1 || totalBits == 8 || totalBits == 16 || totalBits == 32 || totalBits == 64)){
        //Integer types
        if(signedType){
            //Signed
            if(totalBits == 1){
                //Special case of fixed which doesn't make sense but is possible
                return "sfix1_En0";
            } else{
                std::string str = "int" + std::to_string(totalBits);
                if(stringStyle == StringStyle::SIMULINK) {
                    return str;
                }else if(stringStyle == StringStyle::C){
                    str += "_t";
                    if(width > 1){
                        str += "[";
                        if(includeWidth){
                            str += std::to_string(width);
                        }
                        str += "]";
                    }
                    return str;
                }else{
                    throw std::runtime_error("Unknown DataType String Style");
                }
            }
        } else{
            //Unsigned
            if(totalBits == 1){
                if(stringStyle == StringStyle::SIMULINK) {
                    return "boolean";
                }else if(stringStyle == StringStyle::C){
                    std::string str =  "bool";
                    if(width > 1){
                        str += "[";
                        if(includeWidth){
                            str += std::to_string(width);
                        }
                        str += "]";
                    }
                    return str;
                }else{
                    throw std::runtime_error("Unknown DataType String Style");
                }
            } else{
                std::string str = "uint" + std::to_string(totalBits);;
                if(stringStyle == StringStyle::SIMULINK) {
                    return str;
                }else if(stringStyle == StringStyle::C){
                    str += "_t";
                    if(width > 1){
                        str += "[";
                        if(includeWidth){
                            str += std::to_string(width);
                        }
                        str += "]";
                    }
                    return str;
                }else{
                    throw std::runtime_error("Unknown DataType String Style");
                }
            }
        }
    } else{
        if(stringStyle == StringStyle::SIMULINK) {
            //Fixed Point
            std::string signedStr;

            if (signedType) {
                signedStr = "s";
            } else {
                signedStr = "u";
            }

            return signedStr + "fix" + std::to_string(totalBits) + "_En" + std::to_string(fractionalBits);
        }else if(stringStyle == StringStyle::C){
            throw std::runtime_error("No C Style Declaration Exists for Fixed Point Types");
            //Call DataType::getCPUStorageType() to get a C datatype then call this function
        } else{
            throw std::runtime_error("Unknown DataType String Style");
        }

    }
}

bool DataType::isBool() {
    return !floatingPt && !signedType && !isComplex() && totalBits == 1 && fractionalBits == 0;
}

DataType DataType::getCPUStorageType() {
    if(floatingPt || (totalBits > 1 && fractionalBits == 0) || (totalBits == 1 && fractionalBits == 0 && !signedType)){
        DataType newType = *this;
        return newType; //Return a copy of this
    }else if(totalBits == 1 && fractionalBits == 0 && signedType){
        //This is the special case of a single bit signed number.  This should not occur but is theoretically possible under simulink
        std::cerr << "Warning: Signed Single Bit Number Converted to bool" << std::endl;
        DataType newType = *this;
        newType.setSignedType(false);
        return newType;
    }else{
        //Get the smallest CPU type

        //Get a copy of this DataType
        DataType newType = *this;
        newType.setFractionalBits(0); //The C storage types are integers for fixed point numbers
        int origTotalBits = newType.getTotalBits();
        int newTotalBits = GeneralHelper::roundUpToCPUBits(origTotalBits);
        newType.setTotalBits(newTotalBits);

        return newType;
    }
}
