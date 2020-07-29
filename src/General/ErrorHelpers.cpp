//
// Created by Christopher Yarp on 2019-03-15.
//

#include "ErrorHelpers.h"

std::string ErrorHelpers::genErrorStr(std::string errorText, std::string errorPreamble, std::string errorContext) {
    return "[" + errorPreamble + "] " + errorText + ((!errorContext.empty()) ? " {" + errorContext + "}" : "");
}

std::string ErrorHelpers::genErrorStr(std::string errorText, std::string errorContext) {
    return ErrorHelpers::genErrorStr(errorText, VITIS_STD_ERROR_PREAMBLE, errorContext);
}

std::string ErrorHelpers::genErrorStr(std::string errorText) {
    return ErrorHelpers::genErrorStr(errorText, "");
}

std::string ErrorHelpers::genErrorStr(std::string errorText, std::shared_ptr<Node> node, std::string errorPreamble) {
    return ErrorHelpers::genErrorStr(errorText, errorPreamble, node->getErrorReportContextStr());
}

std::string ErrorHelpers::genErrorStr(std::string errorText, std::shared_ptr<Node> node) {
    return ErrorHelpers::genErrorStr(errorText, node->getErrorReportContextStr());
}

std::string ErrorHelpers::genWarningStr(std::string errorText, std::string errorContext) {
    return ErrorHelpers::genErrorStr(errorText, VITIS_STD_WARNING_PREAMBLE, errorContext);
}

std::string ErrorHelpers::genWarningStr(std::string errorText) {
    return ErrorHelpers::genWarningStr(errorText, "");
}

std::string ErrorHelpers::genWarningStr(std::string errorText, std::shared_ptr<Node> node) {
    return ErrorHelpers::genWarningStr(errorText, node->getErrorReportContextStr());
}