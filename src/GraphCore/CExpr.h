//
// Created by Christopher Yarp on 8/9/18.
//

#ifndef VITIS_CEXPR_H
#define VITIS_CEXPR_H

#include <string>

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief A container class to represent C expressions returned by the @ref Node::emitCExpr function
 */
class CExpr {
private:
    std::string expr; ///<The C expression
    bool outputVariable; ///<True if the C expression is an output variable name, false otherwise (used to signal to @ref Node::emitC to not create a redundant temporary var)

public:
    CExpr();
    CExpr(std::string expr, bool outputVariable);

    std::string getExpr() const;
    void setExpr(const std::string &expr);
    bool isOutputVariable() const;
    void setOutputVariable(bool outputVariable);
};

/*@}*/

#endif //VITIS_CEXPR_H
