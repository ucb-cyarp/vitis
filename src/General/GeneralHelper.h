//
// Created by Christopher Yarp on 7/11/18.
//

#ifndef VITIS_GENERALHELPER_H
#define VITIS_GENERALHELPER_H

#include <string>
#include <vector>
#include <memory>
#include <sstream>

/**
 * \addtogroup General General Helper Classes
 *
 * @brief A set of general helper classes.
 */
/*@{*/

/**
 * @brief Provides general helper functions
 */
class GeneralHelper {

public:

    /**
     * @brief Replaces the std::to_string method with one which is capable of outputting higher prevision floating point
     * values (not just a fixed number of fractional bits)
     *
     * Uses the ostream operators which do this.
     *
     * Use of ostringstream to get a string and realization that precision of std::to_string cannot be changed was informed
     * by (https://stackoverflow.com/questions/16605967/set-precision-of-stdto-string-when-converting-floating-point-values)
     *
     * @tparam T
     * @param val
     * @return
     */
    template<typename T>
    static std::string to_string(const T val){
        std::ostringstream stringStream;
        stringStream << val;
        return stringStream.str();
    }

    /**
     * @brief Outputs a string representation of a vector of types for which std::to_string is provided
     * @tparam T type of vector, must be a type for which std::to_string is provided
     * @param vec vector to convert to string
     * @return string representation of vector
     */
    template<typename T>
    static std::string vectorToString(std::vector<T> vec){
        std::string str = "[";

        if(!vec.empty()){
            str += GeneralHelper::to_string(vec[0]);
        }

        unsigned long vecLen = vec.size();
        for(unsigned long i = 1; i<vecLen; i++){
            str += ", " + GeneralHelper::to_string(vec[i]);
        }

        str += "]";

        return str;
    }


    /**
     * @brief Outputs a string representation of a vector of bools
     *
     * To allow re-use of Simulink parsers, this function allows the user to specify the string which represents true,
     * the string that represents false, the separator string, and if brackets are included
     *
     * @param vec vector of bools
     * @param trueStr string which represents true
     * @param falseStr string which represents false
     * @param separator the separator between vector element
     * @param includeBrackets whether or not square brackets should sorround the string
     * @return a string representation of the vector
     */
    static std::string vectorToString(std::vector<bool> vec, std::string trueStr, std::string falseStr, std::string separator = ", ",  bool includeBrackets = false);

    /**
     * @brief Checks if a given pointer type can be cast to another pointer type.
     * @tparam fromT the origional type of the pointer
     * @tparam toT the target type of the pointer
     * @param ptr the pointer to inspect
     * @return a pointer to the new type if possible, nullptr if it is not possible
     */
    template<typename fromT, typename toT>
    static std::shared_ptr<toT> isType(std::shared_ptr<fromT> ptr){
        std::shared_ptr<toT> castPtr;

        try {
            castPtr = std::dynamic_pointer_cast<toT>(ptr);
        }catch(std::bad_cast &e){
            //The cast was not possible, return a nullptr
            castPtr = nullptr;
        }

        return castPtr;
    };

    /**
     * @brief Find the number of integer bits required to represent this number (if rounded up if positive or down if negative)
     *
     * If signed, 2's complement is assumed
     *
     * @param num number to inspect
     * @param forceSigned if true, forces the number of bits to support a signed number, even if num is unsigned
     * @return number of integer bits required to represent the number
     */
    static unsigned long numIntegerBits(double num, bool forceSigned);

    /**
     * @brief Find the width of the smallest standard CPU datatype that can accommodate a number of the given number of bits.
     *
     * For example:
     *   - 1 bit number can be stored in a bool (1)
     *   - 2 bit number can be stored in a char (8)
     *   - 9 but number can be stored in a short (16)
     *
     *   The largest number supported, currently is 64 bits wide
     *
     * @param bits The number of bits which need to be represented.
     * @return bit width of the smallest standard CPU datatype that can accommodate the number
     */
    static unsigned long roundUpToCPUBits(unsigned long bits);

    /**
     * @brief Find if the number of bits corresponds to a standard CPU type
     *
     * @param bits The number of bits which need to be represented.
     * @return true if the number of bits corresponds to a standard CPU type, false otherwise
     */
    static bool isStandardNumberOfCPUBits(unsigned long bits);

    /**
     * @brief Converts an ASCII string to all upper case
     * @param str string to convert to upper case
     * @return all upper case case copy of the given string
     */
    static std::string toUpper(std::string str);

    /**
     * @brief Get 2^exp as an integer
     * @param exp The exponent
     * @return 2^exp as an integer
     */
    static unsigned long twoPow(unsigned long exp);
};

/*@}*/


#endif //VITIS_GENERALHELPER_H
