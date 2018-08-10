//
// Created by Christopher Yarp on 8/9/18.
//

#include "CExpr.h"

CExpr::CExpr() : expr(""), outputVariable(false){

}

CExpr::CExpr(std::string expr, bool outputVariable) : expr(expr), outputVariable(outputVariable){

}

std::string CExpr::getExpr() const {
    return expr;
}

void CExpr::setExpr(const std::string &expr) {
    CExpr::expr = expr;
}

bool CExpr::isOutputVariable() const {
    return outputVariable;
}

void CExpr::setOutputVariable(bool outputVariable) {
    CExpr::outputVariable = outputVariable;
}
