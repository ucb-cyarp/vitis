//
// Created by Christopher Yarp on 8/9/18.
//

#include "CExpr.h"
#include "General/ErrorHelpers.h"
#include "General/GeneralHelper.h"
#include "General/EmitterHelpers.h"

CExpr::CExpr() : expr(""), exprType(ExprType::SCALAR_EXPR), vecLen(0), repeatStride(0), referenceExpr(false){

}

CExpr::CExpr(std::string expr, ExprType exprType) : expr(expr), exprType(exprType), vecLen(0), repeatStride(0), referenceExpr(false){

}

CExpr::CExpr(std::string expr, ExprType exprType, bool referenceExpr) : expr(expr), exprType(exprType), vecLen(0), repeatStride(0), referenceExpr(referenceExpr){

}

CExpr::CExpr(std::string expr, int vecLen, std::string offsetVar) : expr(expr), exprType(ExprType::CIRCULAR_BUFFER_ARRAY), vecLen(vecLen), offsetVar(offsetVar), repeatStride(0), referenceExpr(false) {

}

CExpr::CExpr(std::string expr, std::string offsetVar) : expr(expr), exprType(ExprType::ARRAY_HANKEL_COMPRESSED), offsetVar(offsetVar), repeatStride(0), referenceExpr(false) {

}

CExpr::CExpr(std::string expr, int vecLen, int repeatStride, ExprType exprType) : expr(expr), exprType(exprType), vecLen(vecLen), repeatStride(repeatStride), referenceExpr(false) {
    if(!isRepeatType()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Expected Repeat Type"));
    }
}

std::string CExpr::getExpr() const {
    return expr;
}

void CExpr::setExpr(const std::string &expr) {
    CExpr::expr = expr;
}

bool CExpr::isOutputVariable() const {
    return exprType==ExprType::SCALAR_VAR ||
    exprType==ExprType::ARRAY ||
    exprType==ExprType::CIRCULAR_BUFFER_ARRAY ||
    exprType==ExprType::CIRCULAR_BUFFER_HANKEL_COMPRESSED ||
    exprType==ExprType::ARRAY_HANKEL_COMPRESSED ||
    exprType==ExprType::ARRAY_REPEAT ||
    exprType==ExprType::SCALAR_VAR_REPEAT;
}

CExpr::ExprType CExpr::getExprType() const {
    return exprType;
}

void CExpr::setExprType(CExpr::ExprType exprType) {
    CExpr::exprType = exprType;
}

int CExpr::getVecLen() const {
    return vecLen;
}

void CExpr::setVecLen(int vecLen) {
    CExpr::vecLen = vecLen;
}

std::string CExpr::getOffsetVar() const {
    return offsetVar;
}

void CExpr::setOffsetVar(const std::string &offsetVar) {
    CExpr::offsetVar = offsetVar;
}

std::string CExpr::getExprIndexed(std::vector<std::string> &indexExprs, bool deref) {
    if(exprType==ExprType::SCALAR_EXPR || exprType==ExprType::SCALAR_VAR){
        if(!indexExprs.empty()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Tried to index into a scalar expression or variable"));
        }

        return expr;
    }else if(exprType==ExprType::ARRAY){
        std::string str = expr;
        if(deref){
            str += EmitterHelpers::generateIndexOperation(indexExprs);
        }else{
            str += EmitterHelpers::generateIndexOperationWODereference(indexExprs);
        }
        return str;
    }else if(exprType==ExprType::CIRCULAR_BUFFER_ARRAY) {
        if (indexExprs.size() != 1) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "When indexing in to a circular buffer, a single index expression is expected"));
        }

        std::string indStr = "(" + offsetVar + "+" + indexExprs[0] + ")%" + GeneralHelper::to_string(vecLen);
        std::vector<std::string> indexStrVec= {indStr};

        std::string str = expr;
        if (deref) {
            str += EmitterHelpers::generateIndexOperation(indexStrVec);
        } else {
            str += EmitterHelpers::generateIndexOperationWODereference(indexStrVec);
        }
        return str;
    }else if(exprType==ExprType::CIRCULAR_BUFFER_HANKEL_COMPRESSED) {
        //For the CIRCULAR_BUFFER_HANKEL_COMPRESSED, the outer dimension indexes into the buffer (relative to the circular buffer offset variable)
        //In essence, it is a second offset.
        //@warning The result is only valid for a single value of the outer index.  ie. this does not create a new array with elements sliced off

        //Because of this, the first index in indexExprs is handled differently than the rest.

        //As an example, take tapped delay operating on scalar samples.  It produces a vector which is accessed via the offset from the head of the circular buffer
        //If the TappedDelay is sub-blocked, a matrix is produced with [row][col].  Each row is a vector representing the tapped delay at that moment (defined by the index of the row)
        //The matrix has a symmetry where each cell along the skew-diagonal is the same - called a Hankel Matrix.
        //Instead of storing the result of the TappedDelay as a 2D matrix with the redundant elements, it is stored as a
        //single vector with the row index serving as an offset.  This returns a vector for the given row with the correct
        //starting element.  The column index returns the element at the specified row,col in the matrix.

        //To get an individual element of the matrix, supply the full index to this function.
        //To get just the row of the vector, this function should be called with just 1 index.
        std::vector<std::string> indexStrVec = indexExprs;
        std::string offset = "(" + offsetVar + "+" + indexExprs[0] + ")%" + GeneralHelper::to_string(vecLen);
        indexStrVec.erase(indexStrVec.begin()); //Erase the 0th index, it is part of the offset

        std::string str = "((" + expr + ") + (" + offset + "))";
        if (deref) {
            str += EmitterHelpers::generateIndexOperation(indexStrVec);
        } else {
            str += EmitterHelpers::generateIndexOperationWODereference(indexStrVec);
        }
        return str;
    }else if(exprType==ExprType::ARRAY_HANKEL_COMPRESSED){
        //Same as CIRCULAR_BUFFER_HANKEL_COMPRESSED except does not mod by the vector length
        std::vector<std::string> indexStrVec = indexExprs;
        std::string offset = offsetVar + "+" + indexExprs[0];
        indexStrVec.erase(indexStrVec.begin()); //Erase the 0th index, it is part of the offset

        std::string str = "((" + expr + ") + (" + offset + "))";
        if (deref) {
            str += EmitterHelpers::generateIndexOperation(indexStrVec);
        } else {
            str += EmitterHelpers::generateIndexOperationWODereference(indexStrVec);
        }
        return str;
    }else if(exprType==ExprType::ARRAY_REPEAT){
        if(repeatStride == vecLen){
            //We just ignore the outer index
            std::vector<std::string> indexExprReduced = indexExprs;
            indexExprReduced.erase(indexExprReduced.begin());
            if(indexExprReduced.empty() && deref){
                throw std::runtime_error(ErrorHelpers::genErrorStr("ARRAY_REPEAT expects, >1 dimension being indexed into"));
            }//Allow fewer than expected dimensions if not de-referencing

            std::string str = expr;
            if(deref){
                str += EmitterHelpers::generateIndexOperation(indexExprReduced);
            }else{
                str += EmitterHelpers::generateIndexOperationWODereference(indexExprReduced);
            }
            return str;
        }else if(repeatStride < vecLen && vecLen % repeatStride == 0){
            //Integer Divide the first index by the repeat stride
            std::vector<std::string> indexExprReduced = indexExprs;
            indexExprReduced[0] = "((" + indexExprReduced[0] + ")/" + GeneralHelper::to_string(repeatStride) + ")";

            std::string str = expr;
            if(deref){
                str += EmitterHelpers::generateIndexOperation(indexExprReduced);
            }else{
                str += EmitterHelpers::generateIndexOperationWODereference(indexExprReduced);
            }
            return str;
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("ARRAY_REPEAT expects, repeatStride < vecLen && vecLen % repeatStride == 0"));
        }
    }else if(exprType==ExprType::SCALAR_VAR_REPEAT || exprType==ExprType::SCALAR_EXPR_REPEAT){
        if(vecLen != repeatStride){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Expected SCALAR_VAR_REPEAT or SCALAR_EXPR_REPEAT to have vecLen==repeatStride"));
        }

        //Do not perform indexing, just return the scalar
        return expr;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown ExprType"));
    }
}

bool CExpr::isArrayOrBuffer() const {
    return exprType == ExprType::ARRAY ||
    exprType == ExprType::ARRAY_HANKEL_COMPRESSED ||
    exprType == ExprType::CIRCULAR_BUFFER_ARRAY ||
    exprType == ExprType::CIRCULAR_BUFFER_HANKEL_COMPRESSED ||
    exprType == ExprType::ARRAY_REPEAT ||
    exprType == ExprType::SCALAR_EXPR_REPEAT ||
    exprType == ExprType::SCALAR_VAR_REPEAT;
}

int CExpr::getRepeatStride() const {
    return repeatStride;
}

void CExpr::setRepeatStride(int repeatStride) {
    CExpr::repeatStride = repeatStride;
}

bool CExpr::isRepeatType() {
    return exprType == ExprType::ARRAY_REPEAT || exprType == ExprType::SCALAR_VAR_REPEAT || exprType == ExprType::SCALAR_EXPR_REPEAT;
}

bool CExpr::isReferenceExpr() const {
    return referenceExpr;
}

void CExpr::setIsReferenceExpr(bool isReferenceExpr) {
    CExpr::referenceExpr = isReferenceExpr;
}

bool CExpr::isCompressedType() {
    return exprType == ExprType::ARRAY_HANKEL_COMPRESSED ||
           exprType == ExprType::CIRCULAR_BUFFER_HANKEL_COMPRESSED ||
           exprType == ExprType::ARRAY_REPEAT ||
           exprType == ExprType::SCALAR_EXPR_REPEAT ||
           exprType == ExprType::SCALAR_VAR_REPEAT;

}
