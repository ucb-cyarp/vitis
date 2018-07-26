//
// Created by Christopher Yarp on 7/6/18.
//

#include <regex>
#include <iostream>

#include "NumericValue.h"
#include "General/GeneralHelper.h"

NumericValue::NumericValue() : realInt(0), imagInt(0), complexDouble(std::complex<double>(0, 0)), complex(false), fractional(true){

}

NumericValue::NumericValue(long int realInt, long int imagInt, std::complex<double> complexDouble, bool complex, bool fractional) :
realInt(realInt), imagInt(imagInt), complexDouble(complexDouble), complex(complex), fractional(fractional) {

}

NumericValue::NumericValue(double realDouble) : realInt(0), imagInt(0), complexDouble(std::complex<double>(realDouble, 0)), complex(false), fractional(true) {

}


NumericValue::NumericValue(long int realInt) : realInt(realInt), imagInt(0), complexDouble(std::complex<double>(0, 0)), complex(false), fractional(false) {

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

std::string NumericValue::toString() const{
    std::string val;
    if(fractional){
        val = std::to_string(complexDouble.real());

        if(complex){
            if(complexDouble.imag() >= 0) {
                val = val + " + " + std::to_string(complexDouble.imag()) + "i";
            }else{
                val = val + " - " + std::to_string(-complexDouble.imag()) + "i";
            }
        }
    } else {
        val = std::to_string(realInt);

        if(complex){
            if(imagInt>=0) {
                val = val + " + " + std::to_string(imagInt) + "i";
            }else{
                val = val + " - " + std::to_string(-imagInt) + "i";
            }
        }
    }

    return val;
}

std::vector<NumericValue> NumericValue::parseXMLString(std::string str) {

    std::vector<NumericValue> rtnVal;

    bool array = str.find("[") != std::string::npos;
    bool fractional = str.find(".") != std::string::npos;
    bool complex = str.find("i") != std::string::npos || str.find("j") != std::string::npos || str.find("I") != std::string::npos || str.find("J") != std::string::npos;

    //Setup Regex
    std::string complexStdForm = "\\s*([+-]?[0-9]+[.]?[0-9]*)\\s*([+-])\\s*([0-9]+[.]?[0-9]*)[ijIJ]\\s*";
    std::string complexNonStdForm = "\\s*([+-]?[0-9]+[.]?[0-9]*)[ijIJ]\\s*([+-])\\s*(-?[0-9]+[.]?[0-9]*)\\s*";
//    std::string complexStdFormTranspose = "\\s*([+-]?[0-9]+[.]?[0-9]*)\\s*([+-])\\s*[ijIJ]([0-9]+[.]?[0-9]*)\\s*";
//    std::string complexNonStdFormTranspose = "\\s*([+-]?)[ijIJ]([0-9]+[.]?[0-9]*)\\s*([+-])\\s*(-?[0-9]+[.]?[0-9]*)\\s*";
    std::string complexNoReal = "\\s*([+-]?[0-9]+[.]?[0-9]*)[ijIJ]\\s*";
//    std::string complexNoRealTransposed = "\\s*([+-]?)[ijIJ]([0-9]+[.]?[0-9]*)\\s*";
    std::string real = "\\s*([+-]?[0-9]+[.]?[0-9]*)\\s*";

    std::regex complexStdFormRegex(complexStdForm);
    std::regex complexNonStdFormRegex(complexNonStdForm);
//    std::regex complexStdFormTransposeRegex(complexStdFormTranspose);
//    std::regex complexNonStdFormTransposeRegex(complexNonStdFormTranspose);
    std::regex complexNoRealRegex(complexNoReal);
//    std::regex complexNoRealTransposedRegex(complexNoRealTransposed);
    std::regex realRegex(real);

    //Get Tokens
    unsigned long startPos = str.find("[")+1;
    unsigned long strLen = str.size();

    bool done = false;

    while(!done){
        unsigned long tokenEnd = str.find(',', startPos);
        if(tokenEnd == std::string::npos){
            done = true; //This is the last token
            tokenEnd = str.find(']', startPos);
            if(tokenEnd == std::string::npos){
                tokenEnd = strLen-1;
            }else
            {
                tokenEnd = tokenEnd-1;
            }

        }
        else{
            tokenEnd = tokenEnd-1; //Get character before ','
        }

        std::string subStr = str.substr(startPos, tokenEnd-startPos+1);

        //Create NumericValue object to add to vector
        NumericValue val;
        val.fractional = fractional;
        val.complex = complex;

        //Increment start for next round
        startPos = tokenEnd + 2; //2 to get over the ','

        //Match regex
        std::smatch matches;

        if(complex) {
            bool complexStdFormMatched = std::regex_match(subStr, matches, complexStdFormRegex);

            if (complexStdFormMatched) {
                if(fractional){
                    double realComponent = std::stod(matches[1].str());
                    bool complexNeg = matches[2].str() == "-";
                    double imagComponent = std::stod(matches[3].str());
                    if(complexNeg){
                        imagComponent*=-1;
                    }

                    val.complexDouble = std::complex<double>(realComponent, imagComponent);
                }
                else{
                    int realComponent = std::stoi(matches[1].str());
                    bool complexNeg = matches[2].str() == "-";
                    int imagComponent = std::stoi(matches[3].str());
                    if(complexNeg){
                        imagComponent*=-1;
                    }

                    val.realInt = realComponent;
                    val.imagInt = imagComponent;
                }

            }
            else{
                matches = std::smatch();
                bool complexNoRealMatched = std::regex_match(subStr, matches, complexNoRealRegex);

                if(complexNoRealMatched){
                    if(fractional){
                        double realComponent = 0;
                        double imagComponent = std::stod(matches[1].str());

                        val.complexDouble = std::complex<double>(realComponent, imagComponent);
                    }else{
                        int realComponent = 0;
                        int imagComponent = std::stoi(matches[1].str());

                        val.realInt = realComponent;
                        val.imagInt = imagComponent;
                    }
                }
                else{
                    matches = std::smatch();
                    bool realMatched = std::regex_match(subStr, matches, realRegex);

                    if(realMatched){
                        if(fractional){
                            double realComponent = std::stod(matches[1].str());
                            double imagComponent = 0;

                            val.complexDouble = std::complex<double>(realComponent, imagComponent);
                        }else{
                            int realComponent = std::stoi(matches[1].str());
                            int imagComponent = 0;

                            val.realInt = realComponent;
                            val.imagInt = imagComponent;
                        }
                    }else{
                        matches = std::smatch();
                        bool complexNonStdFormMatched = std::regex_match(subStr, matches, complexNonStdFormRegex);

                        if(complexNonStdFormMatched){
                            if(fractional){
                                double imagComponent = std::stod(matches[1].str());
                                bool realNegative = matches[2].str() == "-";
                                double realComponent = std::stod(matches[3].str());
                                if(realNegative){
                                    realComponent *= -1;
                                }

                                val.complexDouble = std::complex<double>(realComponent, imagComponent);
                            }
                            else{
                                int imagComponent = std::stoi(matches[1].str());
                                bool realNegative = matches[2].str() == "-";
                                int realComponent = std::stoi(matches[3].str());
                                if(realNegative){
                                    realComponent *= -1;
                                }

                                val.realInt = realComponent;
                                val.imagInt = imagComponent;
                            }
                        }else{
                            throw std::runtime_error("Error parsing complex literal: " + subStr);
                        }
                    }
                }
            }
        }else{
            matches = std::smatch();
            bool realMatched = std::regex_match(subStr, matches, realRegex);

            if(realMatched){
                if(fractional){
                    double realComponent = std::stod(matches[1].str());
                    double imagComponent = 0;

                    val.complexDouble = std::complex<double>(realComponent, imagComponent);
                }else{
                    int realComponent = std::stoi(matches[1].str());
                    int imagComponent = 0;

                    val.realInt = realComponent;
                    val.imagInt = imagComponent;
                }
            }
            else{
                throw std::runtime_error("Error parsing real literal: \"" + subStr + "\"");
            }
        }

        rtnVal.push_back(val);
    }

    return rtnVal;
}

bool NumericValue::operator==(const NumericValue &rhs) const {
    return realInt == rhs.realInt &&
           imagInt == rhs.imagInt &&
           complexDouble == rhs.complexDouble &&
           complex == rhs.complex &&
           fractional == rhs.fractional;
}

bool NumericValue::operator!=(const NumericValue &rhs) const {
    return !(rhs == *this);
}

std::ostream &operator<<(std::ostream &os, const NumericValue &value) {
    os << value.toString();
    return os;
}

std::string NumericValue::toString(std::vector<NumericValue> vector) {
    std::string val = "[";

    //insert 1st element if it exists
    if(!vector.empty()){
        val += vector[0].toString();
    }

    unsigned long vectorLen = vector.size();

    for(unsigned long i = 1; i < vectorLen; i++){
       val += ", " + vector[i].toString();
    }

    val += "]";

    return val;
}

bool NumericValue::isSigned() {
    if(fractional){
        if(complex){
            return complexDouble.real() < 0 || complexDouble.imag() < 0;
        }else{
            return complexDouble.real() < 0;
        }
    }else{
        if(complex){
            return realInt < 0 || imagInt < 0;
        }else{
            return realInt < 0;
        }
    }
}

unsigned long NumericValue::numIntegerBits() {
    bool sign = isSigned();

    if(fractional){
        if(complex){
            unsigned long realBits = GeneralHelper::numIntegerBits(complexDouble.real(), sign);
            unsigned long imagBits = GeneralHelper::numIntegerBits(complexDouble.imag(), sign);

            return realBits > imagBits ? realBits : imagBits;
        }else{
            return GeneralHelper::numIntegerBits(complexDouble.real(), sign);
        }
    }else{
        if(complex){
            unsigned long realBits = GeneralHelper::numIntegerBits(realInt, sign);
            unsigned long imagBits = GeneralHelper::numIntegerBits(imagInt, sign);

            return realBits > imagBits ? realBits : imagBits;
        }else{
            return GeneralHelper::numIntegerBits(realInt, sign);
        }
    }
}

NumericValue NumericValue::operator-(const NumericValue &rhs) const {
    NumericValue rtnValue;

    if(!fractional && !rhs.fractional){
        //Both values are ints, return an int
        rtnValue.complex = complex || rhs.complex;
        rtnValue.fractional = false;
        rtnValue.complexDouble = std::complex<double>(0, 0);
        rtnValue.realInt = realInt - rhs.realInt;
        rtnValue.imagInt = imagInt - rhs.imagInt;
    }else if(fractional && !rhs.fractional){
        //Need to promote rhs to double
        rtnValue.complex = complex || rhs.complex;
        rtnValue.fractional = true;
        rtnValue.complexDouble = std::complex<double>(complexDouble.real() - rhs.realInt, complexDouble.imag() - rhs.imagInt);
        rtnValue.realInt = 0;
        rtnValue.imagInt = 0;
    }else if(!fractional && rhs.fractional){
        //Need to promote this to double
        rtnValue.complex = complex || rhs.complex;
        rtnValue.fractional = true;
        rtnValue.complexDouble = std::complex<double>(realInt - rhs.complexDouble.real(), imagInt - rhs.complexDouble.imag());
        rtnValue.realInt = 0;
        rtnValue.imagInt = 0;
    }else{
        //Both are double, return a double
        rtnValue.complex = complex || rhs.complex;
        rtnValue.fractional = true;
        rtnValue.complexDouble = std::complex<double>(complexDouble.real() - rhs.complexDouble.real(), complexDouble.imag() - rhs.complexDouble.imag());
        rtnValue.realInt = 0;
        rtnValue.imagInt = 0;
    }

    return rtnValue;
}

double NumericValue::magnitude() const {
    if(fractional){
        //Get the vectors from the complexDouble
        return std::sqrt(complexDouble.real()*complexDouble.real() + complexDouble.imag()*complexDouble.imag());
    }else{
        //Get the vectors from the integer variables
        return std::sqrt(realInt*realInt + imagInt*imagInt);
    }
}
