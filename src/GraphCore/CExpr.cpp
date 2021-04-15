//
// Created by Christopher Yarp on 8/9/18.
//

#include "CExpr.h"
#include "General/ErrorHelpers.h"
#include "General/GeneralHelper.h"
#include "General/EmitterHelpers.h"

CExpr::CExpr() : expr(""), exprType(ExprType::SCALAR_EXPR), vecLen(0){

}

CExpr::CExpr(std::string expr, ExprType exprType) : expr(expr), exprType(exprType), vecLen(0){

}

CExpr::CExpr(std::string expr, int vecLen, std::string offsetVar) : expr(expr), exprType(ExprType::CIRCULAR_BUFFER_ARRAY), vecLen(vecLen), offsetVar(offsetVar) {

}


std::string CExpr::getExpr() const {
    return expr;
}

void CExpr::setExpr(const std::string &expr) {
    CExpr::expr = expr;
}

bool CExpr::isOutputVariable() const {
    return exprType==ExprType::SCALAR_VAR || exprType==ExprType::ARRAY || exprType==ExprType::CIRCULAR_BUFFER_ARRAY;
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
    }else if(exprType==ExprType::CIRCULAR_BUFFER_ARRAY){
        if(indexExprs.size() != 1){
            throw std::runtime_error(ErrorHelpers::genErrorStr("When indexing in to a circular buffer, a single index expression is expected"));
        }

        std::string indStr = "(" + offsetVar + "+" + indexExprs[0] + ")%" + GeneralHelper::to_string(vecLen);

        std::string str = expr;
        if(deref){
            str += "[" + indStr + "]";
        }else{
            str += "+(" + indStr + ")";
        }
        return str;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown ExprType"));
    }
}

bool CExpr::isArrayOrBuffer() const {
    return exprType == ExprType::ARRAY || exprType == ExprType::CIRCULAR_BUFFER_ARRAY;
}
