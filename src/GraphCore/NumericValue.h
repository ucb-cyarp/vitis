//
// Created by Christopher Yarp on 7/6/18.
//

#ifndef VITIS_NUMERICVALUE_H
#define VITIS_NUMERICVALUE_H

#include <vector>
#include <complex>
#include <ostream>
#include "DataType.h"

/**
* \addtogroup GraphCore Graph Core
*/
/*@{*/

/**
 * @brief Represents a numeric value/constant in the design.  Provides method to parse strings from XML file.
 */
class NumericValue {
private:
    long int realInt;
    long int imagInt;
    std::complex<double> complexDouble;

    bool complex; ///<True if complex, false if real
    bool fractional; ///<True if fractional (double), false if Int

    //TODO: Re-evaluate if this should be refactored into subclasses.  One possible reason is to save space by not having unused member variables.
    //TODO: Also re-evaluate if numeric values should simply be stored as double / complex double (like Matlab).  An advantage to the class is that we can store long ints which may be used for things like ID numbers.
    //TODO: It may be the case that complex integers should not be consider

public:
    /**
     * @brief Construct a real double value of 0
     */
    NumericValue();

    /**
     * @brief Construct a real integer
     * @param realInt the real integer
     */
    NumericValue(long int realInt);

    /**
     * @brief Construct a real double
     * @param realInt the real double
     */
    NumericValue(double realDouble);

    /**
     * @brief Construct a numeric double with the specified values
     * @param realInt
     * @param imagInt
     * @param complexDouble
     * @param complex
     * @param fractional
     */
    NumericValue(long int realInt, long int imagInt, std::complex<double> complexDouble, bool complex, bool fractional);

    /**
     * @brief Returns the numeric value as a string.  If a complex, it will output a + bi.  Will output numbers in accordance with their stored datatypes
     * @return string representation of the numeric value
     */
    std::string toString() const;

    /**
     * @brief Constructs a string from an array of numeric values.
     *
     * Has the form "[vector[0], vector[1], ...]"
     *
     * @param vector vector to construct string from
     * @return string representation of vector
     */
    static std::string toString(std::vector<NumericValue> vector);

//    /**
//     * @brief Returns the numeric value as a string.  If a complex, it will output a + bi.  Will output numbers in accordance with the specified datatype
//     * @param dataType the datatype which determines the format of the output.  The numeric number will be cast to this type
//     * @return string representation of the numeric value
//     */
//    std::string toString(DataType dataType);

    /**
     * @brief Parse an XML string (which may be an array) and produce NumericValue objects
     *
     * @note All NumericValue objects will be configured in the same way (ie. all complex, all real, all int, all double)
     *
     * @param str XML string to parse
     *
     * @return A vector of numericValue objects
     */
    static std::vector<NumericValue> parseXMLString(std::string str);

    //====Getters/Setters====
    long getRealInt() const;

    void setRealInt(long realInt);

    long getImagInt() const;

    void setImagInt(long imagInt);

    std::complex<double> getComplexDouble() const;

    void setComplexDouble(std::complex<double> realDouble);

    bool isComplex() const;

    void setComplex(bool complex);

    bool isFractional() const;

    void setFractional(bool fractional);

    bool operator==(const NumericValue &rhs) const;

    bool operator!=(const NumericValue &rhs) const;

    friend std::ostream &operator<<(std::ostream &os, const NumericValue &value);
};

/*@}*/

#endif //VITIS_NUMERICVALUE_H
