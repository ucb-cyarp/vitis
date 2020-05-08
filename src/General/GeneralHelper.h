//
// Created by Christopher Yarp on 7/11/18.
//

#ifndef VITIS_GENERALHELPER_H
#define VITIS_GENERALHELPER_H

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <algorithm>

/**
 * \addtogroup General General Helper Classes
 *
 * @brief A set of general helper classes.
 * @{
*/

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

    static std::string replaceAll(std::string src, char orig, char repl);

    /**
     * @brief Return true if string is only whitespace, otherwise return false
     * @param str string to check
     * @return true if string is only whitespace, otherwise return false
     */
    static bool isWhiteSpace(std::string str);


    /**
     * @brief Remove all of the values from toRemove from orig
     * @tparam origT type of the original container - must be an STL container
     * @tparam removeT type of the container which contains the elements to remove - must be an STL container
     * @param orig the container to remove from
     * @param toRemove the container of elements to remove
     */
    template<typename origT, typename removeT>
    static void removeAll(origT &orig, removeT &toRemove){
        for(auto removeIt = toRemove.begin(); removeIt != toRemove.end(); removeIt++){
            orig.erase(std::remove(orig.begin(), orig.end(), *removeIt), orig.end());
        }
    }

    /**
     * @brief Repeat a given string a given number of times
     * @param str string to repeat
     * @param reps the number of times to repeat
     * @return str repeated reps times.
     */
    static std::string repString(std::string str, unsigned long reps);

    /**
     * @brief Get base^exp with integers (integer version of pow)
     * @param base the base
     * @param exp the exponent
     * @return base^exp
     */
    static unsigned long intPow(unsigned long base, unsigned long exp);

    static bool parseBool(std::string boolStr);

    /**
     * @brief Finds the longest prefix shared by vectors a and b (starting from index 0)
     *
     * For example:
     * a = {1, 2, 3, 4, 5, 6}
     * b = {1, 2, 3, 9, 8}
     *
     * p = {1, 2, 3}
     *
     * @tparam T
     * @param a
     * @param b
     * @return
     */
    template <typename T>
    static std::vector<T> longestPrefix(std::vector<T> a, std::vector<T> b){
        std::vector<T> prefix;

        int longestPossiblePrefix = std::min(a.size(), b.size());

        for(int i = 0; i<longestPossiblePrefix; i++){
            if(a[i] == b[i]){
                prefix.push_back(a[i]);
            }else{
                //found a difference, stop looking
                break;
            }
        }

        return prefix;
    }

    /**
     * @brief Finds the longest postfix shared by vectors a and b (starting from indexs n-1, arrays are aligned with their largest index)
     *
     * For example:
     * a = {5, 4, 3, 2, 1}
     * b = {6, 7, 2, 1}
     *
     * p = {2, 1}
     *
     * @tparam T
     * @param a
     * @param b
     * @return
     */
    template <typename T>
    static std::vector<T> longestPostfix(std::vector<T> a, std::vector<T> b){
        std::vector<T> postfix;

        int longestPossiblePrefix = std::min(a.size(), b.size());

        for(int i = 0; i<longestPossiblePrefix; i++){
            if(a[a.size()-1-i] == b[b.size()-1-i]){
                postfix.insert(postfix.begin(), a[a.size()-1-i]);
            }else{
                //found a difference, stop looking
                break;
            }
        }

        return postfix;
    }

    /**
     * @brief Get a string with a specified number of spaces
     * @param numSpaces number of spaces to include
     * @return
     */
    static std::string getSpaces(int numSpaces);

    static int gcd(int a, int b);
};

/*! @} */


#endif //VITIS_GENERALHELPER_H
