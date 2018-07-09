//
// Created by Christopher Yarp on 7/6/18.
//

#include <regex>
#include <iostream>

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
