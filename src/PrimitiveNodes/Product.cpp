//
// Created by Christopher Yarp on 7/6/18.
//

#include "Product.h"
#include "GraphCore/NodeFactory.h"
#include "GraphMLTools/GraphMLHelper.h"
#include "General/GeneralHelper.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"
#include <iostream>

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

    //Check if inputs have the same dimension as the output (or are scalar)
    std::vector<int> outputDim = outputPorts[0]->getDataType().getDimensions();
    for(unsigned long i = 0; i<inputPorts.size(); i++){
        DataType inputDT = inputPorts[i]->getDataType();

        //If the input is a scalar, do not check the dimensions
        //Otherwise, check that the dimensions match
        if(!inputDT.isScalar()){
            std::vector<int> inputDim = inputPorts[i]->getDataType().getDimensions();
            if(inputDim != outputDim){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Product - The dimensions of input port " + GeneralHelper::to_string(i) + " did not match the output", getSharedPointer()));
            }
        }
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
    //====Calculate Types====
    //Check if any of the inputs are floating point & if so, find the largest
    //Also check for any fixed point types.  Find the integer final width

    unsigned long numInputPorts = inputPorts.size();

    DataType outputType = getOutputPort(0)->getDataType();

    //Determine the intermediate type
    std::pair<DataType, bool> intermediateTypeFoundFixedPt = findIntermediateType();
    DataType intermediateType = intermediateTypeFoundFixedPt.first;
    bool foundFixedPt = intermediateTypeFoundFixedPt.second;

    DataType intermediateTypeCplx = intermediateType;
    intermediateTypeCplx.setComplex(true);

    std::string outputVarName = name + "_n" + GeneralHelper::to_string(id) + "_out";

    //Note: this emit functions differently from the sum in that the computation of the real and imagionary portion of the solution is not independent
    //To avoid redundant computation, both the real and imagionary components are computed the first time the emit function is called and are stored in
    //temporary variables.  These variables are what are ultimately returned

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

        //Declare the variable here in case it is a vector
        Variable outputVar = Variable(outputVarName, outputType);

        //Emit variable declaration
        std::string outputVarDecl_re = outputVar.getCVarDecl(false, true, false, true) + ";";
        cStatementQueue.push_back(outputVarDecl_re);
        if(outputType.isComplex()){
            std::string outputVarDecl_im = outputVar.getCVarDecl(true, true, false, true) + ";";
            cStatementQueue.push_back(outputVarDecl_im);
        }

        //Insert for loops here
        std::vector<std::string> forLoopIndexVars;
        std::vector<std::string> forLoopClose;
        if(!outputType.isScalar()){
            //Create nested loops for a given array
            std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                    EmitterHelpers::generateVectorMatrixForLoops(outputType.getDimensions());

            std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
            forLoopIndexVars = std::get<1>(forLoopStrs);
            forLoopClose = std::get<2>(forLoopStrs);

            cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());
        }

        //Output the Computation
        //For more than 2 inputs, we will emit a chain of expressions with each subexpression being emitted seperately
        if (!foundFixedPt) {
            std::string norm_var_name_prefix = name + "_n" + GeneralHelper::to_string(id) + "_norm";

            //The first expr is a special case since the first term can be a divide
            std::string first_expr_re = "";
            std::string first_expr_im = "";

            //Dereference here with index of for loop if a vector/matrix
            std::string inputExprDeref_re = inputExprs_re[0] + (getInputPort(0)->getDataType().isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars));
            std::string input0_re = DataType::cConvertType(inputExprDeref_re, getInputPort(0)->getDataType(),
                                                           intermediateType);
            std::string input0_im = "";
            if (getInputPort(0)->getDataType().isComplex()) {
                //Dereference here with index of for loop if a vector/matrix
                std::string inputExprDeref_im = inputExprs_im[0] + (getInputPort(0)->getDataType().isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars));
                input0_im = DataType::cConvertType(inputExprDeref_im, getInputPort(0)->getDataType(), intermediateType);
            }

            //First input is handled separately, it is passed through if the operation is multiply or the reciprocal is calculated if the operation is divide
            if (!inputOp[0]) {
                //The 1st input is a reciprocal
                if (getInputPort(0)->getDataType().isComplex()) {
                    //1st input is imagionary, need to compute a complex reciprocal
                    //However, we need real and imagionary components for the result
                    //We accomplish this by multipying the numberator and denominator by the complex conjugate of the denominator.
                    //This leads to the complex conjugate of the input being divided by a real number which is called the "normalization" here
                    //Compute the normalization

                    //This normalization value is used in both the computation of the real and imagionary component which is why it is stored in a variable
                    //However, it is a temporary variable that is only used for computing first_expr_re and first_expr_im
                    //For vectors/matricies, it is OK if this temporary is declared inside of the for loop since it is not
                    //used outside of the local computation
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
                    first_expr_re = "( (" + intermediateType.toString(DataType::StringStyle::C, false) + " ) 1)/(" +
                                    input0_re + ")";
                }

            } else {
                //The 1st input is unchanged
                first_expr_re = input0_re;
                first_expr_im = input0_im;
            }

            //It is OK for intermediates to be declared inside of the for loop as the intermediate is not used after the final
            //result is generated.
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

                //Dereference here with index of for loop if a vector/matrix
                std::string inputExprDeref_re = inputExprs_re[i] + (getInputPort(i)->getDataType().isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars));
                std::string operand_b_expr_re = DataType::cConvertType(inputExprDeref_re, getInputPort(0)->getDataType(),
                                                                       intermediateType);

                std::string operand_b_expr_im = inputExprs_im[i];
                if(getInputPort(i)->getDataType().isComplex()) {
                    //Dereference here with index of for loop if a vector/matrix.  Only do this if the input port is complex.  Otherwise, leave as ""
                    std::string inputExprDeref_im = inputExprs_im[i] + (getInputPort(i)->getDataType().isScalar() ? "" :
                            EmitterHelpers::generateIndexOperation(forLoopIndexVars));

                    //Only do the data type conversion if the input expression is not empty (ie. if there is an imagionary component of the input)
                    //Otherwise, leave it as ""
                    operand_b_expr_im = DataType::cConvertType(inputExprDeref_im, getInputPort(i)->getDataType(), intermediateType);
                }

                std::string norm_expr = "";

                //Generate the actual multiply or divide operation
                generateMultExprs(operand_a_expr_re, operand_a_expr_im, operand_b_expr_re, operand_b_expr_im,
                                  inputOp[i], +"_" + GeneralHelper::to_string(i), intermediateTypeCplx, norm_expr,
                                  prev_expr_re, prev_expr_im);

                if (!norm_expr.empty()) {
                    //Since the norm expression is a temporary intermediate, it is OK for it be be declared inside the for loops
                    cStatementQueue.push_back(norm_expr);
                }
            }

            //emit the final result
            //Note that the variables are now declared above
            //Dereference here with index of for loop if a vector/matrix
            std::string finalResultDeref_re = outputVar.getCVarName(false)  + (outputType.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars));
            std::string final_result_re = finalResultDeref_re + " = " + DataType::cConvertType(prev_expr_re, intermediateType, getOutputPort(0)->getDataType()) + ";";
            cStatementQueue.push_back(final_result_re);

            if(outputType.isComplex()){
                //Dereference here with index of for loop if a vector/matrix
                std::string finalResultDeref_im = outputVar.getCVarName(true) + (outputType.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars));
                std::string final_result_im = finalResultDeref_im + " = " + DataType::cConvertType(prev_expr_im, intermediateTypeCplx, getOutputPort(0)->getDataType()) + ";";
                cStatementQueue.push_back(final_result_im);
            }

            //close for loop if vector/matrix
            if(!outputType.isScalar()){
                cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
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

std::tuple<EstimatorCommon::ComputeWorkload, bool, bool>
Product::estimateMultExpr(bool mult, bool opAComplex, bool opBComplex, DataType intermediateType, bool expandComplex, bool includeIntermediateLoadStore, bool realDivComplexWaiting) {
    /*
     * | Complexity  | Input             | Result                                           |
     * |-------------|-------------------|--------------------------------------------------|
     * | Real * Real | a*b               | ab                                               |
     * | Real / Real | a/b               | a/b                                              |
     * | Real * Cplx | (a)(b + ci)       | ab + (ac)i                                       |
     * | Cplx * Real | (a + bi)(c)       | ac + (bc)i                                       |
     * | Real / Cplx | a/(b + ci)        | ab/(b^2 + c^2) - (ac/(b^2 + c^2))i               |
     * | Cplx / Real | (a + bi)/c        | a/c + {b/c}i                                     |
     * | Cplx * Cplx | (a + bi)(c + di)  | ac - bd + (ad + bc)i                             |
     * | Cplx / Cplx | (a + bi)/(c + di) | (ac + bd)/(c^2 + d^2) + ((bc - ad)/(c^2 + d^2))i |
     */

    EstimatorCommon::ComputeWorkload workload;
    bool isResultComplex = true;
    bool realDivComplexWaitingAfter = realDivComplexWaiting;

    if(mult){
        //Mult
        if(!opAComplex && !opBComplex){
            //Real * Real | a*b | ab
            if(realDivComplexWaiting){
                throw std::runtime_error(ErrorHelpers::genErrorStr("realDivComplexWaiting should only be true if the first operand is complex (since real/complex should be complex and all subsequent operations should be complex)"));
            }

            isResultComplex = false;

            //Result is the same if expandComplex
            workload.addOperation(EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::MULT,
                                                                    (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
                                                                    EstimatorCommon::OperandComplexity::REAL,
                                                                    intermediateType.getTotalBits(),
                                                                    intermediateType.numberOfElements()));
        }else if((!opAComplex && opBComplex) || (opAComplex && !opBComplex)){
            //Real*Complex || Complex*Real
            //Real * Cplx | (a)(b + ci) | ab + (ac)i
            //Cplx * Real | (a + bi)(c) | ac + (bc)i
            if(realDivComplexWaiting){
                throw std::runtime_error(ErrorHelpers::genErrorStr("realDivComplexWaiting should only be true if the first operand is complex (since real/complex should be complex and all subsequent operations should be complex)"));
            }

            isResultComplex = true;

            if(expandComplex){
                //Note the sum between the real and imagionary components is not computed unless it is subtract where the negation occurs
                EstimatorCommon::ComputeOperation op = EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::MULT,
                                                                                         (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
                                                                                         EstimatorCommon::OperandComplexity::REAL,
                                                                                         intermediateType.getTotalBits(),
                                                                                         intermediateType.numberOfElements());
                //There are 2 real operations
                workload.addOperation(op);
                workload.addOperation(op);
            }else{
                workload.addOperation(EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::MULT,
                                                                        (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
                                                                        EstimatorCommon::OperandComplexity::MIXED,
                                                                        intermediateType.getTotalBits(),
                                                                        intermediateType.numberOfElements()));
            }
        }else{
            //Cplx * Cplx | (a + bi)(c + di) | ac - bd + (ad + bc)i
            isResultComplex = true;
            if(expandComplex){
                realDivComplexWaitingAfter = false; //This will absorb the extra subtract from a real/complex

                EstimatorCommon::ComputeOperation multOp = EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::MULT,
                                                                                            (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
                                                                                            EstimatorCommon::OperandComplexity::REAL,
                                                                                            intermediateType.getTotalBits(),
                                                                                            intermediateType.numberOfElements());

                EstimatorCommon::ComputeOperation sumOp = EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::ADD_SUB,
                                                                                             (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
                                                                                             EstimatorCommon::OperandComplexity::REAL,
                                                                                             intermediateType.getTotalBits(),
                                                                                             intermediateType.numberOfElements());

                //4 Mults and 2 Sums
                workload.addOperation(multOp);
                workload.addOperation(multOp);
                workload.addOperation(multOp);
                workload.addOperation(multOp);
                workload.addOperation(sumOp);
                workload.addOperation(sumOp);
            }else{
                workload.addOperation(EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::MULT,
                                                                        (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
                                                                        EstimatorCommon::OperandComplexity::COMPLEX,
                                                                        intermediateType.getTotalBits(),
                                                                        intermediateType.numberOfElements()));
            }
        }
    }else{
        //Div
        if(!opAComplex && !opBComplex){
            //Real / Real | a/b | a/b
            if(realDivComplexWaiting){
                throw std::runtime_error(ErrorHelpers::genErrorStr("realDivComplexWaiting should only be true if the first operand is complex (since real/complex should be complex and all subsequent operations should be complex)"));
            }

            isResultComplex = false;

            //Does not change realDivComplexWaitingAfter

            //Result is the same if expandComplex
            workload.addOperation(EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::DIV,
                                                                    (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
                                                                    EstimatorCommon::OperandComplexity::REAL,
                                                                    intermediateType.getTotalBits(),
                                                                    intermediateType.numberOfElements()));
        }else if(!opAComplex && opBComplex){
            //Real / Cplx | a/(b + ci) | ab/(b^2 + c^2) - (ac/(b^2 + c^2))i
            if(realDivComplexWaiting){
                throw std::runtime_error(ErrorHelpers::genErrorStr("realDivComplexWaiting should only be true if the first operand is complex (since real/complex should be complex and all subsequent operations should be complex)"));
            }

            isResultComplex = true;

            if(expandComplex){
                realDivComplexWaitingAfter = true;
                //Set the realDivComplexWaitingAfter line to true.  If this is still true after all of the products/divs, need an extra subtract operation

                EstimatorCommon::ComputeOperation multOp = EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::MULT,
                                                                                             (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
                                                                                             EstimatorCommon::OperandComplexity::REAL,
                                                                                             intermediateType.getTotalBits(),
                                                                                             intermediateType.numberOfElements());
                EstimatorCommon::ComputeOperation divOp =  EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::DIV,
                                                                                             (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
                                                                                             EstimatorCommon::OperandComplexity::REAL,
                                                                                             intermediateType.getTotalBits(),
                                                                                             intermediateType.numberOfElements());
                EstimatorCommon::ComputeOperation sumOp = EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::ADD_SUB,
                                                                                            (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
                                                                                            EstimatorCommon::OperandComplexity::REAL,
                                                                                            intermediateType.getTotalBits(),
                                                                                            intermediateType.numberOfElements());

                //Normalization Term: 2 Products, 1 Sum, 1 Intermediate Store
                workload.addOperation(multOp);
                workload.addOperation(multOp);
                workload.addOperation(sumOp);
                //Components: (1 Product, 1 Divide, 1 Intermediate Load)*2
                workload.addOperation(multOp);
                workload.addOperation(multOp);
                workload.addOperation(divOp);
                workload.addOperation(divOp);

                //TODO: Omitting load/stores since these will almost assuredly be in registers since they are used so close together
//                if(includeIntermediateLoadStore){
//                    EstimatorCommon::ComputeOperation storeOp = EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::STORE,
//                                                                                                 (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
//                                                                                                 EstimatorCommon::OperandComplexity::REAL,
//                                                                                                 intermediateType.getTotalBits(),
//                                                                                                 intermediateType.numberOfElements());
//                    EstimatorCommon::ComputeOperation loadOp =  EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::LOAD,
//                                                                                                  (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
//                                                                                                  EstimatorCommon::OperandComplexity::REAL,
//                                                                                                  intermediateType.getTotalBits(),
//                                                                                                  intermediateType.numberOfElements());
//                    workload.addOperation(storeOp);
//                    workload.addOperation(loadOp);
//                    workload.addOperation(loadOp);
//                }
            }else{
                workload.addOperation(EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::DIV,
                                                                        (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
                                                                        EstimatorCommon::OperandComplexity::MIXED,
                                                                        intermediateType.getTotalBits(),
                                                                        intermediateType.numberOfElements()));
            }
        }else if(opAComplex && !opBComplex){
            //Cplx / Real | (a + bi)/c | a/c + {b/c}i
            isResultComplex = true;

            //Does not impact realDivComplexWaitingAfter

            if(expandComplex) {
                //2 Div
                EstimatorCommon::ComputeOperation divOp =  EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::DIV,
                                                                                             (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
                                                                                             EstimatorCommon::OperandComplexity::REAL,
                                                                                             intermediateType.getTotalBits(),
                                                                                             intermediateType.numberOfElements());
                workload.addOperation(divOp);
                workload.addOperation(divOp);
            }else{
                workload.addOperation(EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::DIV,
                                                                        (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
                                                                        EstimatorCommon::OperandComplexity::MIXED,
                                                                        intermediateType.getTotalBits(),
                                                                        intermediateType.numberOfElements()));
            }
        }else{
            //Cplx / Cplx | (a + bi)/(c + di) | (ac + bd)/(c^2 + d^2) + ((bc - ad)/(c^2 + d^2))i
            isResultComplex = true;

            if(expandComplex) {
                EstimatorCommon::ComputeOperation multOp = EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::MULT,
                                                                                             (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
                                                                                             EstimatorCommon::OperandComplexity::REAL,
                                                                                             intermediateType.getTotalBits(),
                                                                                             intermediateType.numberOfElements());
                EstimatorCommon::ComputeOperation divOp =  EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::DIV,
                                                                                             (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
                                                                                             EstimatorCommon::OperandComplexity::REAL,
                                                                                             intermediateType.getTotalBits(),
                                                                                             intermediateType.numberOfElements());
                EstimatorCommon::ComputeOperation sumOp = EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::ADD_SUB,
                                                                                            (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
                                                                                            EstimatorCommon::OperandComplexity::REAL,
                                                                                            intermediateType.getTotalBits(),
                                                                                            intermediateType.numberOfElements());

                //Normalization Term: 2 Products, 1 Sum, 1 Intermediate Store
                workload.addOperation(multOp);
                workload.addOperation(multOp);
                workload.addOperation(sumOp);

                //Components: (2 Products, 1 Add/Sub, 1 Divide, 1 Intermediate Load)*2
                workload.addOperation(multOp);
                workload.addOperation(multOp);
                workload.addOperation(multOp);
                workload.addOperation(multOp);
                workload.addOperation(sumOp);
                workload.addOperation(sumOp);
                workload.addOperation(divOp);
                workload.addOperation(divOp);

                if(realDivComplexWaitingAfter){
                    //Need to emit the extra subtract
                    workload.addOperation(sumOp);

                    realDivComplexWaitingAfter = false;
                }

                //TODO: Omitting load/stores since these will almost assuredly be in registers since they are used so close together
//                if(includeIntermediateLoadStore){
//                    EstimatorCommon::ComputeOperation storeOp = EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::STORE,
//                                                                                                  (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
//                                                                                                  EstimatorCommon::OperandComplexity::REAL,
//                                                                                                  intermediateType.getTotalBits(),
//                                                                                                  intermediateType.numberOfElements());
//                    EstimatorCommon::ComputeOperation loadOp =  EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::LOAD,
//                                                                                                  (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
//                                                                                                  EstimatorCommon::OperandComplexity::REAL,
//                                                                                                  intermediateType.getTotalBits(),
//                                                                                                  intermediateType.numberOfElements());
//                    workload.addOperation(storeOp);
//                    workload.addOperation(loadOp);
//                    workload.addOperation(loadOp);
//                }
            }else{
                workload.addOperation(EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::DIV,
                                                                        (intermediateType.isFloatingPt() ? EstimatorCommon::OperandType::FLOAT : EstimatorCommon::OperandType::INT),
                                                                        EstimatorCommon::OperandComplexity::COMPLEX,
                                                                        intermediateType.getTotalBits(),
                                                                        intermediateType.numberOfElements()));
            }
        }
    }

    return std::tuple<EstimatorCommon::ComputeWorkload, bool, bool>(workload, isResultComplex, realDivComplexWaitingAfter);
}

EstimatorCommon::ComputeWorkload
Product::getComputeWorkloadEstimate(bool expandComplexOperators, bool expandHighLevelOperators,
                                    ComputationEstimator::EstimatorOption includeIntermediateLoadStore,
                                    ComputationEstimator::EstimatorOption includeInputOutputLoadStores) {
    int numScalarInputs = 0;
    for(int i = 0; i<inputPorts.size(); i++){
        if(getInputPort(i)->getDataType().isScalar()){
            numScalarInputs++;
        }
    }

    //TODO: Update Product Emit and Estimation so that it is more efficient and easier to estimate
    //      Change to re-arrange inputs, compute numerator, compute denominator, and have a single divide (if even necessaryy)
    //      Re-arrange scalars so they are computed seperatly.  Compute scalar real values and scalar complex values seperately.  Compute the numerator and devisor scalars seperatly.
    //      Apply the real scalar result to the real vector input computation (sepeately in numerator and denominator).  Apply the complex scalar result to the complex vector input computation (speratly in numerator and denominator)
    //      Combine the real and complex components (in the numerator and denominator seperatly)
    //      Do the final division of the numerator and denominator
    if(numScalarInputs>1){
        std::cerr << ErrorHelpers::genWarningStr("Workload estimation for product with >1 scalar inputs may be inaccurate (reporting more operations than actually are needed)", getSharedPointer()) << std::endl;
    }

    //+++ Add I/O Load/Store if appripriate +++
    EstimatorCommon::ComputeWorkload workload = getIOLoadStoreWorkloadEst(getSharedPointer(),
                                                                          includeInputOutputLoadStores);

    //For product, we require all the inputs to have the same dimensions (with the exception of scalars)
    //However, scalars still need to be added to each vector element, so we will treat them as vector operations
    int vecLen = EstimatorCommon::getLargestInputNumElements(getSharedPointer());

    std::pair<DataType, bool> intermediateTypeFoundFixedPt = findIntermediateType();
    DataType intermediateType = intermediateTypeFoundFixedPt.first;

    bool realDivComplexWaiting = false;
    //Determine if the first operand is complex
    bool opCurrentlyComplex = getInputPort(0)->getDataType().isComplex();

    bool includeIntermediateLoadStoreBool = (includeIntermediateLoadStore==ComputationEstimator::EstimatorOption::ENABLED ||
                                            (includeIntermediateLoadStore==ComputationEstimator::EstimatorOption::NONSCALAR_ONLY && vecLen>1)) ?
                                            true : false;

    //+++ Add casts to first operand if applicable +++
    EstimatorCommon::addCastsIfBaseTypesDifferent(workload, getInputPort(0)->getDataType(), intermediateType, expandComplexOperators, vecLen);

    //+++ Include Extra Reciprocal Op if (all ops are real and divides) or (op is complex) +++
    if(!inputOp[0]){
        //The first operand is a reciprocal
        DataType portDatatype = getInputPort(0)->getDataType();
        if(portDatatype.isComplex()){
            //First port is complex, include extra reciprocal (an int divide)

            //TODO: Should reciprocal be treated differently (seperate op type)

            //For these, use the number of elements in the port datatype because
            if(expandComplexOperators){
                EstimatorCommon::ComputeOperation productOp = EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::MULT,
                                                                                                (intermediateType.isFloatingPt()
                                                                                                 ? EstimatorCommon::OperandType::FLOAT
                                                                                                 : EstimatorCommon::OperandType::INT),
                                                                                                EstimatorCommon::OperandComplexity::REAL,
                                                                                                intermediateType.getTotalBits(),
                                                                                                portDatatype.numberOfElements());
                EstimatorCommon::ComputeOperation sumOp = EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::ADD_SUB,
                                                                                            (intermediateType.isFloatingPt()
                                                                                             ? EstimatorCommon::OperandType::FLOAT
                                                                                             : EstimatorCommon::OperandType::INT),
                                                                                            EstimatorCommon::OperandComplexity::REAL,
                                                                                            intermediateType.getTotalBits(),
                                                                                            portDatatype.numberOfElements());
                EstimatorCommon::ComputeOperation divOp = EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::DIV,
                                                                                            (intermediateType.isFloatingPt()
                                                                                             ? EstimatorCommon::OperandType::FLOAT
                                                                                             : EstimatorCommon::OperandType::INT),
                                                                                            EstimatorCommon::OperandComplexity::REAL,
                                                                                            intermediateType.getTotalBits(),
                                                                                            portDatatype.numberOfElements());
                //Normalize: 2 Products, 1 Sum, 1 Intermediate Store
                workload.addOperation(productOp);
                workload.addOperation(productOp);
                workload.addOperation(sumOp);
                //Real Component: 1 Divide, 1 Intermediate Load
                workload.addOperation(divOp);
                //Imag Component: 1 Divide, 1 Subtract (Handled Later Via realDivComplexWaiting), 1 Intermediate Load
                workload.addOperation(divOp);
                realDivComplexWaiting = true;

                //TODO: Omitting load/stores since these will almost assuredly be in registers since they are used so close together
//                if(includeIntermediateLoadStoreBool){
//                    EstimatorCommon::ComputeOperation storeOp = EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::STORE,
//                                                                                                  (intermediateType.isFloatingPt()
//                                                                                                   ? EstimatorCommon::OperandType::FLOAT
//                                                                                                   : EstimatorCommon::OperandType::INT),
//                                                                                                  EstimatorCommon::OperandComplexity::REAL, //The normalization os real
//                                                                                                  intermediateType.getTotalBits(),
//                                                                                                  portDatatype.numberOfElements());
//
//                    EstimatorCommon::ComputeOperation loadOp = EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::LOAD,
//                                                                                                 (intermediateType.isFloatingPt()
//                                                                                                  ? EstimatorCommon::OperandType::FLOAT
//                                                                                                  : EstimatorCommon::OperandType::INT),
//                                                                                                 EstimatorCommon::OperandComplexity::REAL, //The normalization os real
//                                                                                                 intermediateType.getTotalBits(),
//                                                                                                 portDatatype.numberOfElements());
//                    workload.addOperation(storeOp);
//                    workload.addOperation(loadOp);
//                    workload.addOperation(loadOp);
//                }
            }else {
                workload.addOperation(EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::DIV,
                                                                        (intermediateType.isFloatingPt()
                                                                         ? EstimatorCommon::OperandType::FLOAT
                                                                         : EstimatorCommon::OperandType::INT),
                                                                        EstimatorCommon::OperandComplexity::MIXED, //Mixed because Real/Complex
                                                                        intermediateType.getTotalBits(),
                                                                        portDatatype.numberOfElements()));
            }
        }else{
            //First port was real  Check if subsequent ports are

            bool foundCplxPort = false;
            bool foundMult = false;
            for(int i = 1; i<inputPorts.size(); i++){
                DataType portDatatype = getInputPort(i)->getDataType();
                foundCplxPort |= portDatatype.isComplex();
                foundMult |= inputOp[i];
            }

            if(foundCplxPort || !foundMult){
                //Either there are complex ports after this or all the subsequent operations
                //are divides, need to add the reciprocal, which is a real reciprocal since the first port is real
                //TODO: should reciprocals be tracked seperatly?
                workload.addOperation(EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::DIV,
                                                                        (intermediateType.isFloatingPt()
                                                                         ? EstimatorCommon::OperandType::FLOAT
                                                                         : EstimatorCommon::OperandType::INT),
                                                                        EstimatorCommon::OperandComplexity::REAL, //Mixed because Real/Complex
                                                                        intermediateType.getTotalBits(),
                                                                        portDatatype.numberOfElements()));
            }
        }
    }

    //+++ Run through the subsequent operands +++
    for(int i = 1; i<inputPorts.size(); i++){
        DataType portDatatype = getInputPort(0)->getDataType();
        std::tuple<EstimatorCommon::ComputeWorkload, bool, bool> estTuple = estimateMultExpr(inputOp[i], opCurrentlyComplex, portDatatype.isComplex(), intermediateType, expandComplexOperators, includeIntermediateLoadStoreBool, realDivComplexWaiting);

        EstimatorCommon::ComputeWorkload portWorkload = std::get<0>(estTuple);
        opCurrentlyComplex = std::get<1>(estTuple);
        realDivComplexWaiting = std::get<2>(estTuple);

        workload.addOperations(portWorkload);

        //Add cast to intermediate
        EstimatorCommon::addCastsIfBaseTypesDifferent(workload, getInputPort(i)->getDataType(), intermediateType, expandComplexOperators, vecLen);
    }

    //+++ Include Extra Subtract if realDivComplexWaiting is true at end of traversing ports (only if expandComplexOperators) +++
    if(realDivComplexWaiting && expandComplexOperators){
        //If the first term is a complex reciprocal, and assuming the value is broadcast, the extra sum will be the width of the input port
        //Otherwise, the real/complex occured later
        DataType firstDatatype = getInputPort(0)->getDataType();

        workload.addOperation(EstimatorCommon::ComputeOperation(EstimatorCommon::OpType::ADD_SUB,
                                                                (intermediateType.isFloatingPt()
                                                                 ? EstimatorCommon::OperandType::FLOAT
                                                                 : EstimatorCommon::OperandType::INT),
                                                                EstimatorCommon::OperandComplexity::REAL, //Mixed because Real/Complex
                                                                intermediateType.getTotalBits(),
                                                                vecLen));
    }

    //+++ Include Output cast (if needed) +++
    EstimatorCommon::addCastsIfBaseTypesDifferent(workload, intermediateType, getOutputPort(0)->getDataType(), expandComplexOperators, vecLen);

    //Both real and imag are computed at the same time and stored in intermediates.  However, these are also the inputs/output.  Will consider these inputs/outputs (to avoid double counting) and the normalizations as true intermediates

    return workload;
}

std::pair<DataType, bool> Product::findIntermediateType() {
    bool foundFloat = false;
    DataType largestFloat;
    bool foundFixedPt = false;
    unsigned long intWorkingWidth = 0;
    bool foundComplex = false;

    unsigned long numInputPorts = inputPorts.size();
    for (unsigned long i = 0; i < numInputPorts; i++) {
        DataType portDataType = getInputPort(i)->getDataType();
        if (portDataType.isFloatingPt()) {
            if (!foundFloat) {
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


    //Determine the intermediate type
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

    //Set the dimension of the intermediate type to be the same as the output.
    //This is because the largest float datatype may be a scalar while other inputs are vectors/matricies
    DataType outputType = getOutputPort(0)->getDataType();
    intermediateType.setDimensions(outputType.getDimensions());

    return std::pair<DataType, bool>(intermediateType, foundFixedPt);
}


