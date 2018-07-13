//
// Created by Christopher Yarp on 7/11/18.
//

#include "GeneralHelper.h"

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
