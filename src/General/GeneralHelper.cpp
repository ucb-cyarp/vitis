//
// Created by Christopher Yarp on 7/11/18.
//

#include "GeneralHelper.h"
#include "ErrorHelpers.h"

#include <cmath>
#include <locale>

std::string
GeneralHelper::vectorToString(std::vector<bool> vec, std::string trueStr, std::string falseStr, std::string separator,
                              bool includeBrackets) {
    std::string str = "";

    if (includeBrackets) {
        str = "[";
    }

    if(!vec.empty()){
        str += (vec[0] ? trueStr : falseStr);
    }

    unsigned long vecLen = vec.size();
    for(unsigned long i = 1; i<vecLen; i++){
        str += separator + (vec[i] ? trueStr : falseStr);
    }

    if(includeBrackets){
        str += "]";
    }

    return str;
}

unsigned long GeneralHelper::numIntegerBits(double num, bool forceSigned) {
    double working;

    if(num>0){
        //Round up if positive
        working = std::ceil(num);
    }else{
        //Round down if negative
        working = std::floor(num);
    }

    double numBits;

    if(num>0){
        //Need to add 1 before taking log base 2
        numBits = std::ceil(std::log2(working + 1));

        if(forceSigned){
            //Add an extra bit if this is forced to be signed
            numBits += 1;
        }
    }else{
        //If negative, do not add a +1 before taking log base 1 but do add a 1 to the result (it is forced to be signed)
        numBits = std::ceil(std::log2(-working))+1;
        numBits = numBits > 2 ? numBits : 2; //For signed, need a min width of 2
    }

    //cast to an unsigned long
    return static_cast<unsigned long>(numBits);
}

unsigned long GeneralHelper::roundUpToCPUBits(unsigned long bits) {
    if(bits <= 1){
        return 1;
    }else if(bits <= 8){
        return 8;
    }else if(bits <= 16){
        return 16;
    }else if(bits <= 32){
        return 32;
    }else if(bits <= 64){
        return 64;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Cannot find a standard CPU integer type for a number of " + std::to_string(bits) + " bits"));
    }
}

bool GeneralHelper::isStandardNumberOfCPUBits(unsigned long bits){
    return bits == 1 || bits == 8 || bits == 16 || bits == 32 || bits == 64;

}

std::string GeneralHelper::toUpper(std::string str) {
    unsigned long strLen = str.length();

    std::string upperCaseStr = "";

    for(unsigned long i = 0; i<strLen; i++){
        upperCaseStr.push_back(std::toupper(str[i], std::locale::classic()));
    }

    return upperCaseStr;
}

unsigned long GeneralHelper::twoPow(unsigned long exp){
    unsigned long tmp = 1;
    return tmp << exp;
}

std::string GeneralHelper::replaceAll(std::string src, char orig, char repl){
    std::string replaced = src;

    for(unsigned long i = 0; i < replaced.size(); i++){
        if(replaced[i] == orig){
            replaced.replace(i, 1, std::string(1,repl));
        }
    }

    return replaced;
}

bool GeneralHelper::isWhiteSpace(std::string str){
    for(unsigned long i = 0; i<str.size(); i++){
        if(!isblank(str[i])){
            return false;
        }
    }

    return true;
}

std::string GeneralHelper::repString(std::string str, unsigned long reps) {
    std::string repeatedStr = "";

    for(unsigned long i = 0; i<reps; i++){
        repeatedStr+=str;
    }

    return repeatedStr;
}

unsigned long GeneralHelper::intPow(unsigned long base, unsigned long exp){
    if(exp == 0){
        return 1;
    }

    unsigned long tmp = base;

    for(unsigned long i = 1; i<exp; i++){
        tmp *= base;
    }

    return tmp;
}

bool GeneralHelper::parseBool(std::string boolStr) {
    if(boolStr == "true" || boolStr == "True" || boolStr == "TRUE" || boolStr == "t" || boolStr == "T") {
        return true;
    }else if(boolStr == "false" || boolStr == "False" || boolStr == "FALSE" || boolStr == "f" || boolStr == "F") {
        return false;
    }else{
        throw std::runtime_error("Could not parse bool: " + boolStr);
    }
}

std::string GeneralHelper::getSpaces(int numSpaces){
    std::string str;

    for(int i = 0; i<numSpaces; i++){
        str += " ";
    }

    return str;
}
