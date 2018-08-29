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
    int64_t realInt;
    int64_t imagInt;
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
     * @param realDouble the real double
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
    NumericValue(int64_t realInt, int64_t imagInt, std::complex<double> complexDouble, bool complex, bool fractional);

    /**
     * @brief Check if the numeric value is signed
     * @return true if signed, false otherwise
     */
    bool isSigned();

    /**
     * @brief Get the number of integer bits required to represent the number.
     *
     * Does not specify the number of fractional bits required.
     *
     * @note Number of bits is for either the real or imag component.  If complex, double this number for thr total number of integer bits required.
     * @return number of integer bits required to represent the number
     */
    unsigned long numIntegerBits();

    /**
     * @brief Returns the numeric value as a string.  If a complex, it will output a + bi.  Will output numbers in accordance with their stored datatypes
     * @return string representation of the numeric value
     */
    std::string toString() const;

    /**
     * @brief Returns the real or imagionary component of the numeric value as a string.  Will output numbers in accordance with their stored datatypes
     * @param imag If true, the imagionary component is returned.  If false, the real component is returned.
     * @return string representation of the real or imagionary component of the numeric value
     */
    std::string toStringComponent(bool imag);

    /**
     * @brief Returns the real or imagionary component of the numeric value as a string.  The component is converted to the target data type
     * @param imag If true, the imagionary component is returned.  If false, the real component is returned.
     * @param typeToConvertTo The datatype to convert the numeric value to
     * @return string representation of the real or imagionary component of the numeric value
     */
    std::string toStringComponent(bool imag, DataType typeToConvertTo);

    /**
     * @brief Constructs a string from an array of numeric values.
     *
     * Has the form "[vector[0], vector[1], ...]"
     *
     * @param vector vector to construct string from
     * @param startStr the string used to start the returned string
     * @param endStr the string used to end the returned string
     * @param delimStr the string used as the deliminator between items in the returned string
     * @return string representation of vector
     */
    static std::string toString(std::vector<NumericValue> vector, std::string startStr = "[", std::string endStr = "]", std::string delimStr = ", ");

    /**
     * @brief Constructs a string from an array of numeric values.
     *
     * It uses the @ref toStringComponent method to print the numeric value
     *
     * Has the form "[vector[0], vector[1], ...]"
     *
     * @param imag If true, the imagionary component is returned.  If false, the real component is returned.
     * @param typeToConvertTo The datatype to convert the numeric value to
     * @param vector vector to construct string from
     * @param startStr the string used to start the returned string
     * @param endStr the string used to end the returned string
     * @param delimStr the string used as the deliminator between items in the returned string
     * @return string representation of vector
     */
    static std::string toStringComponent(bool imag, DataType typeToConvertTo, std::vector<NumericValue> vector, std::string startStr = "[", std::string endStr = "]", std::string delimStr = ", ");

    /*
     * @brief Returns the numeric value as a string.  If a complex, it will output a + bi.  Will output numbers in accordance with the specified datatype
     * @param tgtDataType the datatype which determines the format of the output.  The numeric number will be cast to this type
     * @return string representation of the numeric value
     */
//    std::string toString(DataType tgtDataType);

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
    int64_t getRealInt() const;

    void setRealInt(int64_t realInt);

    int64_t getImagInt() const;

    void setImagInt(int64_t imagInt);

    std::complex<double> getComplexDouble() const;

    void setComplexDouble(std::complex<double> realDouble);

    bool isComplex() const;

    void setComplex(bool complex);

    bool isFractional() const;

    void setFractional(bool fractional);

    bool operator==(const NumericValue &rhs) const;

    bool operator!=(const NumericValue &rhs) const;

    NumericValue operator-(const NumericValue &rhs) const;

    double magnitude() const;

    friend std::ostream &operator<<(std::ostream &os, const NumericValue &value);
};

/*@}*/

#endif //VITIS_NUMERICVALUE_H
