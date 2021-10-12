//
// Created by Christopher Yarp on 8/9/18.
//

#ifndef VITIS_CEXPR_H
#define VITIS_CEXPR_H

#include <string>
#include <vector>

/**
 * \addtogroup GraphCore Graph Core
 * @{
*/

/**
 * @brief A container class to represent C expressions returned by the @ref Node::emitCExpr function
 */
class CExpr {
public:
    enum class ExprType{
        SCALAR_EXPR, ///<The expression is a scalar expression which is not a simple variable name (ex. a+b)
        SCALAR_VAR, ///<The expression is a scalar variable name.  It does not need to be dereferenced to be used
        ARRAY, ///<The expression is a pointer to the head of an array.  Pointers to single elements are arrays with length 1
        ARRAY_HANKEL_COMPRESSED, ///< A generalization of the Hankel Matrix (elements along skew diagonal of matrix are the same) so that >2 dimensions can be supported.  Effectively generalizes ARRAY for sub-blocking.
        CIRCULAR_BUFFER_ARRAY, ///<The expression is a pointer to the head of a circular buffer.  An offset the current front of the buffer is provided with all indexing being referenced from that point.  The length is used to handle wraparound when indexing
        CIRCULAR_BUFFER_HANKEL_COMPRESSED, ///< A generalization of the Hankel Matrix (elements along skew diagonal of matrix are the same) so that >2 dimensions can be supported.  Effectively generalizes CIRCULAR_BUFFER_ARRAY for sub-blocking.
        ARRAY_REPEAT, ///< An expression where the outer dimension are replicas of the inner dimensions.  If repeatStride == vecLen, the outer index of getExprIndexed is simply ignored and has no effect.
        SCALAR_EXPR_REPEAT, ///< An expression where the outer dimension are replicas of the given scalar expression.  In this case, no indexing is performed even when getExprIndexed is supplied with an index
        SCALAR_VAR_REPEAT ///< An expression where the outer dimension are replicas of the given scalar expression. In this case, no indexing is performed even when getExprIndexed is supplied with an index
    };

private:
    std::string expr; ///<The C expression, or variable name if
    ExprType exprType; ///<Indicates the type of expression.  If it is a SCALAR_EXPR, Node::cEmit will create a temporary for it if required for fanout
    int vecLen; ///<The vector length if the expression is a circular buffer.  The total outer dimension length of the ArrayRepeat
    std::string offsetVar; ///<The variable representing the offset in a circular buffer or hankel array.  If blank, the hankel array is not part of a larger buffer for which an offset is required.

    int repeatStride; ///<The number of outer dimensions between elements


public:
    CExpr();
    CExpr(std::string expr, ExprType exprType);
    /**
     * @brief When defining circular buffers, this allows the vector length and offset variable to be specified
     * @param expr pointer to the front of the buffer
     * @param vecLen the length of the buffer (in elements not necessarily bytes)
     * @param offsetVar the offset variable used to indicate the current head of the buffer
     */
    CExpr(std::string expr, int vecLen, std::string offsetVar); //This is for circular buffer

    /**
     * @brief When defining hankel array, this allows the offset variable to be specified
     * @param expr pointer to the front of the buffer
     * @param offsetVar the offset variable used to indicate the current head of the buffer
     */
    CExpr(std::string expr, std::string offsetVar); //This is for ARRAY_HANKEL_COMPRESSED

    /**
     * @brief When defining a repeat type
     * @param expr pointer to the front of the buffer
     * @param offsetVar the offset variable used to indicate the current head of the buffer
     */
    CExpr(std::string expr, int vecLen, int repeatStride, ExprType exprType); //This is for ARRAY_REPEAT, SCALAR_EXPR_REPEAT, SCALAR_VAR_REPEAT

    std::string getExpr() const;
    void setExpr(const std::string &expr);
    ExprType getExprType() const;
    void setExprType(ExprType exprType);
    int getVecLen() const;
    void setVecLen(int vecLen);
    std::string getOffsetVar() const;
    void setOffsetVar(const std::string &offsetVar);
    int getRepeatStride() const;
    void setRepeatStride(int repeatStride);

    /**
     * @brief Check if the CExpr is a variable (ie. not a scalar expression)
     * @return
     */
    bool isOutputVariable() const;

    bool isArrayOrBuffer() const;

    /**
     * @brief Get the expression but indexed if it is an array or circular buffer
     *
     * When the CExpr is a Scalar Expression or Scalar Variable, indexExprs and deref have no effect
     *
     * When indexing into a circular buffer, the current buffer head and the buffer length are taken into account when indexing to handle the wraparound
     *
     * @param indexExprs an expression representing the indexes in the array or circular buffer to access.  Must have size 1 for circular buffers
     * @param deref if true, the the array value is indexed and dereferenced.  If false, indexing just takes the form of offsetting from the base address
     * @return
     */
    std::string getExprIndexed(std::vector<std::string> &indexExprs, bool deref);

    bool isRepeatType();
};

/*! @} */

#endif //VITIS_CEXPR_H
