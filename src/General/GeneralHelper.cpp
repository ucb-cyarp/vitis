//
// Created by Christopher Yarp on 7/11/18.
//

#include "GeneralHelper.h"

#include <cmath>

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
        //If positive, need to add 1 before taking log base 2
        numBits = std::ceil(std::log2(working + 1));

        if(forceSigned){
            //Add an extra bit if this is forced to be signed
            numBits += 1;
        }
    }else{
        //If negative, do not add a +1 before taking log base 1 but do add a 1 to the result (it is forced to be signed)
        numBits = std::ceil(std::log2(-working))+1;
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
        throw std::runtime_error("Cannot find a standard CPU integer type for a number of " + std::to_string(bits) + " bits");
    }
}
