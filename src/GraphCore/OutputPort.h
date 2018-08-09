//
// Created by Christopher Yarp on 7/17/18.
//

#ifndef VITIS_OUTPUTPORT_H
#define VITIS_OUTPUTPORT_H

#include "Port.h"
#include "Variable.h"

/**
* \addtogroup GraphCore Graph Core
*/
/*@{*/

/**
 * @brief The representation of a node's output port
 *
 * Primarily serves as a container for pointers to arcs connected to the given port.
 */
class OutputPort : public Port{
protected:
    bool cEmittedRe; ///<Denotes if C code for the real component has already been emitted (used when fanout exists)
    bool cEmittedIm; ///<Denotes if C code for the real component has already been emitted (used when fanout exists)
    std::string cEmitReStr; ///<Contains the variable name used for the output variable's real component (used when fanout exists)
    std::string cEmitImStr; ///<Contains the variable name used for the output variable's imaginary component (used when fanout exists)

public:
    /**
     * @brief Construct an empty output port object
     *
     * The port object has number 0, no parent, and no arcs.
     */
    OutputPort();

    /**
     * @brief Construct a port output object with the specified parent and port numbers.
     *
     * The port object is initialized with an empty arc list
     *
     * @param parent The node this port belongs to
     * @param portNum The port number
     */
    OutputPort(Node* parent, int portNum);

    /**
     * @brief Validate if the connections to this port are correct.
     *
     *   - For Output Ports, verify that all output arcs have the same type.  Also check that there is at least 1 arc (unconnected ports should have an arc to the unconnected master)
     *
     * If an invalid configuration is detected, the function will throw an exception
     */
    void validate() override;

    /**
     * @brief Get an aliased shared pointer to this OutputPort
     *
     * This is a more specialized version of @ref Port::getSharedPointer
     *
     * @note Does not override @ref Port::getSharedPointer due to the different return type
     *
     * @return an aliased shared pointer to the port
     */
    std::shared_ptr<OutputPort> getSharedPointerOutputPort(); //NOTE: should never return a bare pointer or an unaliased shared pointer to a port

    /**
     * @brief Gets the base name of the C variable for this output
     *
     * The base name takes the form nodeName_n<id>_outputPortNum
     *
     * @return the base name as a string
     */
    std::string getCOutputVarNameBase();

    /**
     * @brief Get the C variable name for the output (used for fanout)
     *
     * The variable name is taken from the base name (see @ref getCOutputVarNameBase)
     *
     * @note This is less expensive than calling getCOutputVar which needs to determine the datatype
     *
     * @warning This does not check if the DataType is complex when imag == true.  If that functionality is desired, @ref getCOutputVar should be used
     *
     * @param imag
     * @return the C variable name for the output
     */
    std::string getCOutputVarName(bool imag = false);

    /**
     * @brief Returns the C variable for the output (used for fanout)
     *
     * The name takes the form nodeName_n<id>_outputPortNum_<re/im>
     *
     * The type is converted to a standard CPU datatype
     *
     * @warning Validation should occur before this function is called to confirm all out arcs have the same type
     * and that the port is connected
     *
     * @return the C variable for the output
     */
    Variable getCOutputVar();

    //==== Getters/Setters ====
    bool isCEmittedRe() const;
    void setCEmittedRe(bool cEmittedRe);
    bool isCEmittedIm() const;
    void setCEmittedIm(bool cEmittedIm);
    std::string getCEmitReStr() const;
    void setCEmitReStr(const std::string &cEmitReStr);
    std::string getCEmitImStr() const;
    void setCEmitImStr(const std::string &cEmitImStr);

};

/*@}*/

#endif //VITIS_OUTPUTPORT_H
