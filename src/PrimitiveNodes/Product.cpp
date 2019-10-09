//
// Created by Christopher Yarp on 7/6/18.
//

#include "Product.h"
#include "GraphCore/NodeFactory.h"
#include "GraphMLTools/GraphMLHelper.h"
#include "General/GeneralHelper.h"
#include "General/ErrorHelpers.h"

Product::Product() : emittedBefore(false) {

}

Product::Product(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), emittedBefore(false) {

}

std::vector<bool> Product::getInputOp() const {
    return inputOp;
}

void Product::setInputOp(const std::vector<bool> &inputOp) {
    Product::inputOp = inputOp;
}

std::shared_ptr<Product> Product::createFromGraphML(int id, std::string name,
                                                    std::map<std::string, std::string> dataKeyValueMap,
                                                    std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<Product> newNode = NodeFactory::createNode<Product>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important property -- Inputs ====
    std::string inputOperations;

    if(dialect == GraphMLDialect::VITIS){
        //Vitis Name -- InputOps
        inputOperations = dataKeyValueMap.at("InputOps");
    } else if(dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Name -- Inputs
        inputOperations = dataKeyValueMap.at("Inputs");
    } else
    {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Product", newNode));
    }

    //There are multiple cases for inputs.  One is a string of * or /.  The other is a number.
    std::vector<bool> ops;

    if(inputOperations.empty()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Empty Inputs parameter passed to Product", newNode));
    }else if(inputOperations[0] == '*' || inputOperations[0] == '/' || inputOperations[0] == '|'){
        //An array of *,/
        unsigned long inputLength = inputOperations.size();
        for(unsigned long i = 0; i<inputLength; i++){
            if(inputOperations[i] == '*'){
                ops.push_back(true);
            }else if(inputOperations[i] == '/'){
                ops.push_back(false);
            }else if(inputOperations[i] == '|'){
                //This is is a placeholder character that changes the position of the ports in the GUI but does not effect their numbering
            }else{
                throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown format for Product Input Parameter", newNode));
            }
        }
    }else{
        //Parmater is a number
        int numInputs = std::stoi(inputOperations);

        for(int i = 0; i<numInputs; i++){
            ops.push_back(true);
        }
    }

    newNode->setInputOp(ops);

    return newNode;
}

std::set<GraphMLParameter> Product::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("InputOps", "string", true));

    return parameters;
}

xercesc::DOMElement *
Product::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Product");

    GraphMLHelper::addDataNode(doc, thisNode, "InputOps", GeneralHelper::vectorToString(inputOp, "*", "/", "", false));

    return thisNode;
}

std::string Product::typeNameStr(){
    return "Product";
}

std::string Product::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() + "\nInputOps:" + GeneralHelper::vectorToString(inputOp, "*", "/", "", false);

    return label;
}

void Product::validate() {
    Node::validate();

    if(inputPorts.size() < 2){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Product - Should Have 2 or More Input Ports", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Product - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    //Check that if any input is complex, the result is complex
    unsigned long numInputPorts = inputPorts.size();
    bool foundComplex = false;

    for(unsigned long i = 0; i<numInputPorts; i++){
        DataType inType = getInputPort(i)->getDataType();
        if(inType.isComplex()){
            foundComplex = true;
            break;
        }

    }

    if(foundComplex) {
        DataType outType = getOutputPort(0)->getDataType();
        if(!outType.isComplex()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Product - An Input Port is Complex but Output is Real", getSharedPointer()));
        }
    }

    if(!foundComplex){
        DataType outType = getOutputPort(0)->getDataType();
        if(outType.isComplex()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Product - Output Port is Complex but Inputs are all Real", getSharedPointer()));
        }
    }

    if(inputOp.size() != inputPorts.size()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Product - The number of operators (" + GeneralHelper::to_string(inputOp.size()) + ") does not match the number of inputs (" + GeneralHelper::to_string(inputPorts.size()) + ")", getSharedPointer()));
    }
}

CExpr Product::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    //TODO: Implement Vector Support
    if(getOutputPort(0)->getDataType().getWidth()>1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("C Emit Error - Product Support for Vector Types has Not Yet Been Implemented", getSharedPointer()));
    }

    //====Calculate Types====
    //Check if any of the inputs are floating point & if so, find the largest
    //Also check for any fixed point types.  Find the integer final width
    bool foundFloat = false;
    DataType largestFloat;
    bool foundFixedPt = false;
    unsigned long intWorkingWidth = 0;
    bool foundComplex = false;

    unsigned long numInputPorts = inputPorts.size();
    for (unsigned long i = 0; i < numInputPorts; i++) {
        DataType portDataType = getInputPort(i)->getDataType();
        if (portDataType.isFloatingPt()) {
            if (foundFloat == false) {
                foundFloat = true;
                largestFloat = portDataType;
            } else {
                if (largestFloat.getTotalBits() < portDataType.getTotalBits()) {
                    largestFloat = portDataType;
                }
            }
        } else if (portDataType.getFractionalBits() != 0) {
            foundFixedPt = true;
        } else {
            //Integer
            if (inputOp[i]) {
                intWorkingWidth += portDataType.getTotalBits(); //For multiply, bit growth is the sum of

                if (portDataType.isComplex()) {
                    if (foundComplex) {
                        intWorkingWidth++; //This is a complex*complex, increase the width by 1 for the plus
                    } else {
                        foundComplex = true; //This is a real*complex, the result will be complex but the width is not increased by 1 in this case.
                    }
                }
            }//We don't grow the width of division (since this is integer and you can't divide by a fraction).  However, we do not decrease the width in this case.  TODO: come up with more complex scheme to deal with divides
        }
    }

    DataType intermediateType;
    if (foundFloat) {
        intermediateType = largestFloat; //Floating point types do not grow
    } else {
        //Integer
        intermediateType = getInputPort(
                0)->getDataType(); //Get the base datatype of the 1st input to modify (we did not find a float or fixed pt so this is an int)
        intermediateType.setTotalBits(
                intWorkingWidth); //Since this is a promotion, masking will not occur in the datatype convert
        intermediateType.setComplex(false); //The intermediate is real
    };

    DataType intermediateTypeCplx = intermediateType;
    intermediateTypeCplx.setComplex(true);

    std::string outputVarName = name + "_n" + GeneralHelper::to_string(id) + "_out";

    //====Emit====
    //Check if this is the first time this emit function has been called, if so, get the input expressions and compute the results
    if(!emittedBefore) {
        //Get the expressions for each input
        std::vector<std::string> inputExprs_re;
        std::vector<std::string> inputExprs_im;

        for (unsigned long i = 0; i < numInputPorts; i++) {
            std::shared_ptr<OutputPort> srcOutputPort = getInputPort(i)->getSrcOutputPort();
            int srcOutputPortNum = srcOutputPort->getPortNum();
            std::shared_ptr<Node> srcNode = srcOutputPort->getParent();


            inputExprs_re.push_back(srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, false));
            if (getInputPort(i)->getDataType().isComplex()) {
                inputExprs_im.push_back(srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, true));
            } else {
                inputExprs_im.push_back("");
            }
        }


        //Output the Computation
        //For more than 2 inputs, we will emit a chain of expressions with each subexpression being emitted seperately
        if (!foundFixedPt) {
            std::string norm_var_name_prefix = name + "_n" + GeneralHelper::to_string(id) + "_norm";

            //The first expr is a special case since the first term can be a divide
            std::string first_expr_re = "";
            std::string first_expr_im = "";

            std::string input0_re = DataType::cConvertType(inputExprs_re[0], getInputPort(0)->getDataType(),
                                                           intermediateType);
            std::string input0_im = "";
            if (getInputPort(0)->getDataType().isComplex()) {
                input0_im = DataType::cConvertType(inputExprs_im[0], getInputPort(0)->getDataType(), intermediateType);
            }

            if (!inputOp[0]) {
                //The 1st input is a reciprocal
                if (getInputPort(0)->getDataType().isComplex()) {
                    //1st input is imagionary
                    //Compute the normalization
                    std::string result_norm_expr_body =
                            "((" + input0_re + ")*(" + input0_re + ")+(" + input0_im + ")*(" + input0_im + "))";
                    Variable result_norm_expr_var = Variable(norm_var_name_prefix + GeneralHelper::to_string(0),
                                                             intermediateType);
                    std::string result_norm_expr =
                            result_norm_expr_var.getCVarDecl(false, false, false, false) + " = " +
                            result_norm_expr_body + ";";

                    //Emit normalization
                    cStatementQueue.push_back(result_norm_expr);

                    //Compute the real component
                    first_expr_re = "(" + input0_re + ")/(" + result_norm_expr_var.getCVarName(false) + ")";

                    //Compute the image component
                    first_expr_im = "-(" + input0_im + ")/(" + result_norm_expr_var.getCVarName(false) + ")";

                } else {
                    //1st input is real, do a simple reciprocal
                    first_expr_re = "( (" + intermediateType.toString(DataType::StringStyle::C, false) + " ) 1.0)/(" +
                                    input0_re + ")";
                }

            } else {
                //The 1st input is unchanged
                first_expr_re = input0_re;
                first_expr_im = input0_im;
            }

            std::string intermediate_var_name_prefix = name + "_n" + GeneralHelper::to_string(id) + "_intermediate";

            //Emit the products / divisions
            std::string prev_expr_re = "";
            std::string prev_expr_im = "";

            for (unsigned long i = 1; i < numInputPorts; i++) {
                //emit prev statement if i > 1 (final emit handled seperatly)

                std::string operand_a_expr_re = "";
                std::string operand_a_expr_im = "";

                if (i > 1) {
                    //This is the previous intermediate result
                    Variable prev_result_var = Variable(
                            intermediate_var_name_prefix + "_" + GeneralHelper::to_string(i - 1), intermediateTypeCplx);
                    std::string prev_result_expr =
                            prev_result_var.getCVarDecl(false, false, false, false) + " = " + prev_expr_re + ";";
                    cStatementQueue.push_back(prev_result_expr);

                    if (!prev_expr_im.empty()) {
                        //Previous result was complex
                        std::string result_norm_expr_im =
                                prev_result_var.getCVarDecl(true, false, false, false) + " = " + prev_expr_im + ";";
                        cStatementQueue.push_back(result_norm_expr_im);
                    }
                } else {
                    //This is the 1st operand
                    operand_a_expr_re = first_expr_re;
                    operand_a_expr_im = first_expr_im;
                }

                std::string operand_b_expr_re = DataType::cConvertType(inputExprs_re[i], getInputPort(0)->getDataType(),
                                                                       intermediateType);
                std::string operand_b_expr_im = DataType::cConvertType(inputExprs_im[i], getInputPort(0)->getDataType(),
                                                                       intermediateType);

                std::string norm_expr = "";

                //Generate the actual multiply or divide operation
                generateMultExprs(operand_a_expr_re, operand_a_expr_im, operand_b_expr_re, operand_b_expr_im,
                                  inputOp[i], +"_" + GeneralHelper::to_string(i), intermediateTypeCplx, norm_expr,
                                  prev_expr_re, prev_expr_im);

                if (!norm_expr.empty()) {
                    cStatementQueue.push_back(norm_expr);
                }
            }

            Variable outputVar;
            if(getOutputPort(0)->getDataType().isComplex()) {
                outputVar = Variable(outputVarName, intermediateTypeCplx);
            }else{
                outputVar = Variable(outputVarName, intermediateType);
            }

            //emit the final result
            std::string final_result_re = outputVar.getCVarDecl(false, false, false, false) + " = " + prev_expr_re + ";";
            cStatementQueue.push_back(final_result_re);

            if(getOutputPort(0)->getDataType().isComplex()){
                std::string final_result_im = outputVar.getCVarDecl(true, false, false, false) + " = " + prev_expr_im + ";";
                cStatementQueue.push_back(final_result_im);
            }

            emittedBefore = true;

            return CExpr(outputVar.getCVarName(imag), true);

        } else {
            //TODO: Finish
            throw std::runtime_error(ErrorHelpers::genErrorStr("C Emit Error - Fixed Point Not Yet Implemented for Product", getSharedPointer()));
        }
    }else{
        //Return the variable name
        Variable outputVar;
        if(getOutputPort(0)->getDataType().isComplex()) {
            outputVar = Variable(outputVarName, intermediateTypeCplx);
        }else{
            outputVar = Variable(outputVarName, intermediateType);
        }

        return CExpr(outputVar.getCVarName(imag), true);

    }

    return CExpr("", false);
}

Product::Product(std::shared_ptr<SubSystem> parent, Product* orig) : PrimitiveNode(parent, orig), inputOp(orig->inputOp), emittedBefore(orig->emittedBefore){

}

std::shared_ptr<Node> Product::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Product>(parent, this);
}

bool Product::hasInternalFanout(int inputPort, bool imag) {
    //For now, if any of the input ports are complex, we say there is internal fanout.  This is not actually nessisary
    //TODO: replace with version that checks each input to see if it is reused.  This will depend on the structure used for the reduction of the * and /s

    bool foundComplex = false;
    unsigned long numInputPorts = inputPorts.size();

    for(unsigned long i = 0; i<numInputPorts; i++){
        DataType portDataType = getInputPort(i)->getDataType();
        if(portDataType.isComplex()){
            foundComplex = true;
            break;
        }
    }

    return foundComplex;
}

void Product::generateMultExprs(const std::string &a_re, const std::string &a_im, const std::string &b_re,
                                const std::string &b_im, bool mult, const std::string &result_norm_name,
                                DataType result_norm_type, std::string &result_norm_expr, std::string &result_re,
                                std::string &result_im) {

    bool a_cplx = !a_im.empty();
    bool b_cplx = !b_im.empty();

    result_norm_expr = ""; //default

    if(!a_cplx && !b_cplx){
        if(mult){
            //Real Mult
            result_re = "(" + a_re + ")*(" + b_re + ")";
            result_im = "";
        }else{
            //Real Div
            result_re = "(" + a_re + ")/(" + b_re + ")";
            result_im = "";
        }
    }else if(!a_cplx && b_cplx){
        if(mult){
            //Real * Cplx
            result_re = "(" + a_re + ")*(" + b_re + ")";
            result_im = "(" + a_re + ")*(" + b_im + ")";
        }else{
            //Real / Cplx
            std::string result_norm_expr_body = "((" + b_re + ")*(" + b_re + ")+(" + b_im + ")*(" + b_im + "))";
            Variable result_norm_expr_var = Variable(result_norm_name, result_norm_type);
            result_norm_expr = result_norm_expr_var.getCVarDecl(false, false, false, false) + " = " + result_norm_expr_body + ";";

            result_re = "((" + a_re + ")*(" + b_re + ")/(" + result_norm_expr_var.getCVarName(false) + "))";
            result_im = "(-(" + a_re + ")*(" + b_im + ")/(" + result_norm_expr_var.getCVarName(false) + "))";
        }
    }else if(a_cplx && !b_cplx){
        if(mult){
            //Cmpl * Real
            result_re = "(" + a_re + ")*(" + b_re + ")";
            result_im = "(" + a_im + ")*(" + b_re + ")";
        }else{
            //Cplx / Real
            result_re = "(" + a_re + ")/(" + b_re + ")";
            result_im = "(" + a_im + ")/(" + b_re + ")";
        }
    }else{
        //a_cplx && b_cplx
        if(mult){
            //Cplx * Cplx
            result_re = "(" + a_re + ")*(" + b_re + ")-(" + a_im + ")*(" + b_im + ")";
            result_im = "(" + a_re + ")*(" + b_im + ")+(" + a_im + ")*(" + b_re + ")";
        }else{
            //Cplx / Cplx
            std::string result_norm_expr_body = "((" + b_re + ")*(" + b_re + ")+(" + b_im + ")*(" + b_im + "))";
            Variable result_norm_expr_var = Variable(result_norm_name, result_norm_type);
            result_norm_expr = result_norm_expr_var.getCVarDecl(false, false, false, false) + " = " + result_norm_expr_body + ";";

            result_re = "(((" + a_re + ")*(" + b_re + ")+(" + a_im +  ")*(" + b_im + "))/(" + result_norm_expr_var.getCVarName(false) + "))";
            result_im = "(((" + a_im + ")*(" + b_re + ")-(" + a_re +  ")*(" + b_im + "))/(" + result_norm_expr_var.getCVarName(false) + "))";
        }
    }
}


