//
// Created by Christopher Yarp on 7/29/20.
//

#include "InnerProduct.h"
#include "GraphCore/NodeFactory.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"
#include "Product.h"

InnerProduct::InnerProduct() : emittedBefore(false), complexConjBehavior(ComplexConjBehavior::FIRST) {

}

InnerProduct::InnerProduct(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), emittedBefore(false), complexConjBehavior(ComplexConjBehavior::FIRST) {

}

InnerProduct::InnerProduct(std::shared_ptr<SubSystem> parent, InnerProduct *orig) : PrimitiveNode(parent, orig), emittedBefore(orig->emittedBefore), complexConjBehavior(orig->complexConjBehavior) {

}

std::shared_ptr<InnerProduct>
InnerProduct::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<InnerProduct> newNode = NodeFactory::createNode<InnerProduct>(parent);
    newNode->setId(id);
    newNode->setName(name);

    if (dialect == GraphMLDialect::VITIS) {
        std::string complexConjBehaviorStr = dataKeyValueMap.at("ComplexConjBehavior");
        newNode->complexConjBehavior = parseComplexConjBehavior(complexConjBehaviorStr);
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //The simulink semantic is to take the complex conjugate of the first input
        newNode->complexConjBehavior = ComplexConjBehavior::FIRST;
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - InnerProduct", newNode));
    }

    return newNode;
}

std::set<GraphMLParameter> InnerProduct::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("ComplexConjBehavior", "string", true));

    return parameters;
}

xercesc::DOMElement *
InnerProduct::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "InnerProduct");

    GraphMLHelper::addDataNode(doc, thisNode, "ComplexConjBehavior", complexConjBehaviorToString(complexConjBehavior));

    return thisNode;
}

std::string InnerProduct::typeNameStr() {
    return "InnerProduct";
}

std::string InnerProduct::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() + "\nComplexConjBehavior: " + complexConjBehaviorToString(complexConjBehavior);

    return label;
}

void InnerProduct::validate() {
    Node::validate();

    if(inputPorts.size() != 2){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - InnerProduct - Should Have Exactly 2 Input Ports", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - InnerProduct - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    //Check that the inputs have the same dimensions
    if(getInputPort(0)->getDataType().getDimensions() != getInputPort(1)->getDataType().getDimensions()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - InnerProduct - Input Ports Need to Have te Same Dimensions", getSharedPointer()));
    }

    if(!getInputPort(0)->getDataType().isVector() && !getInputPort(0)->getDataType().isScalar()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - InnerProduct - Input Port 0 Must be a Vector or Scalar", getSharedPointer()));
    }

    if(!getInputPort(1)->getDataType().isVector() && !getInputPort(1)->getDataType().isScalar()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - InnerProduct - Input Port 1 Must be a Vector or Scalar", getSharedPointer()));
    }

    //--- Check for complexity (code borrowed from Product class) ---
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
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - InnerProduct - An Input Port is Complex but Output is Real", getSharedPointer()));
        }
    }

    if(!foundComplex){
        DataType outType = getOutputPort(0)->getDataType();
        if(outType.isComplex()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - InnerProduct - Output Port is Complex but Inputs are all Real", getSharedPointer()));
        }
    }
}

std::shared_ptr<Node> InnerProduct::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<InnerProduct>(parent, this);
}

bool InnerProduct::hasInternalFanout(int inputPort, bool imag) {
    //==== Use the same code as Product class for now =====

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

CExpr
InnerProduct::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                        bool imag) {
    //=== Find intermediate type ===
    DataType input0DT = getInputPort(0)->getDataType();
    DataType input1DT = getInputPort(1)->getDataType();

    //Check if either type is a floating point type, if so, the result will be floating point
    DataType productType;
    if(input0DT.isFloatingPt() || input1DT.isFloatingPt()){
        //The product type will be the larger of these 2 types
        productType = input0DT.getTotalBits() > input1DT.getFractionalBits() ? input0DT : input1DT;
        productType.setDimensions({1});
    }else{
        //Both types are either integer or fixed point
        if(input0DT.getFractionalBits() != 0 || input1DT.getFractionalBits() != 0){
            //One of the the inputs is a fixed point type
            //TODO: support fixed point
            throw std::runtime_error(ErrorHelpers::genErrorStr("InnerProduct does not currently support fixed point types", getSharedPointer()));
        }else{
            //Both are integer types
            //The productType is the sum of the number of bits in each
            int totalBits = input0DT.getTotalBits() + input1DT.getTotalBits();

            if(input0DT.isComplex() || input1DT.isComplex()){
                //Add an additional bit to accomodate the add which is a part of complex multiplication
                totalBits++;
            }

            productType = input0DT;
            productType.setTotalBits(totalBits);
            productType.setDimensions({1});
        }
    }
    //Set intermediate type to be complex if either input is complex
    if(input0DT.isComplex() || input1DT.isComplex()){
        productType.setComplex(true);
    }else{
        productType.setComplex(false);
    }

    //Get the intermediate point based on the product type.  It is the type of the accumulator and needs to expand for the additions
    DataType intermediateType = productType;
    if(!productType.isFloatingPt()){
        //We grow by the number of elements in the vector (the number of sums)
        int totalBits = productType.getTotalBits() + ((int)ceil(log2(input0DT.numberOfElements())));
        intermediateType.setTotalBits(totalBits);
    }//else, the type is floating point and additional expansion is not performed.

    //We need a CPU storage type for the intermediate type
    DataType intermediateTypeCPUStore = intermediateType.getCPUStorageType();

    //=== Check if emitted before ===
    DataType outputType = getOutputPort(0)->getDataType();
    Variable outputVar;
    Variable accumulatorVar(name + "_n" + GeneralHelper::to_string(id) + "_Accum", intermediateTypeCPUStore);
    accumulatorVar.setInitValue({NumericValue((long)0)}); //Set initial condition to be used when declaring accumulator var
    if(intermediateTypeCPUStore == getOutputPort(0)->getDataType()){
        //If they are equivalent, the output variable is the accumulator
        outputVar = accumulatorVar;
    }else{
        outputVar = Variable(name + "_n" + GeneralHelper::to_string(id) + "_Out", outputType);
    }

    if(!emittedBefore) {
        //=== Get Input Expressions (borrowed from Product class as we are using some of its helper functions) ===
        std::vector<std::string> inputExprs_re;
        std::vector<std::string> inputExprs_im;

        for (unsigned long i = 0; i < getInputPorts().size(); i++) {
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

        //=== Create accumulator variables - init to 0 ===
        //Init to 0 via the includeInit option in the decl
        cStatementQueue.push_back(accumulatorVar.getCVarDecl(false, false, true, false, false) + ";");
        if(intermediateTypeCPUStore.isComplex()){
            cStatementQueue.push_back(accumulatorVar.getCVarDecl(true, false, true, false, false) + ";");
        }

        //=== Create a for loop over the dimensions of the vectors ===
        //Note that the inputs are allowed to be scalar as this is considered a degenerate case
        //The inputs should be validated to be vectors
        std::vector<std::string> forLoopIndexVars;
        std::vector<std::string> forLoopClose;
        if(!input0DT.isScalar()){
            //Create nested loops for a given array
            std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                    EmitterHelpers::generateVectorMatrixForLoops(input0DT.getDimensions());

            std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
            forLoopIndexVars = std::get<1>(forLoopStrs);
            forLoopClose = std::get<2>(forLoopStrs);

            cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());
        }

        //=== Dereference terms ===
        std::vector<std::string> inputExprsDeref_re;
        std::vector<std::string> inputExprsDeref_im;
        //There should only be 2 ports (validated above) but easy to write the loop
        //note that the expression index is the same as the port number
        for(int i = 0; i<inputExprs_re.size(); i++){
            std::string inputExpr_re_deref = inputExprs_re[i] + (getInputPort(i)->getDataType().isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars));
            std::string inputExpr_re_deref_cast = DataType::cConvertType(inputExpr_re_deref, getInputPort(i)->getDataType(), intermediateTypeCPUStore);

            inputExprsDeref_re.push_back(inputExpr_re_deref_cast);
        }
        for(int i = 0; i<inputExprs_im.size(); i++){
            if(getInputPort(i)->getDataType().isComplex()){
                std::string inputExpr_im_deref = inputExprs_im[i] + (getInputPort(i)->getDataType().isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars));
                std::string inputExpr_im_deref_cast = DataType::cConvertType(inputExpr_im_deref, getInputPort(i)->getDataType(), intermediateTypeCPUStore);

                inputExprsDeref_im.push_back(inputExpr_im_deref_cast);
            }else {
                inputExprsDeref_im.push_back("");
            }
        }

        //=== Complex conjugate ===
        if(input0DT.isComplex() && complexConjBehavior == ComplexConjBehavior::FIRST){
            inputExprsDeref_im[0] = "-(" + inputExprsDeref_im[0] + ")";
        }

        if(input1DT.isComplex() && complexConjBehavior == ComplexConjBehavior::SECOND){
            inputExprsDeref_im[1] = "-(" + inputExprsDeref_im[1] + ")";
        }

        //=== Multiply terms ===
        std::string multResult_re;
        std::string multResult_im;
        std::string normExpr; //Not used since we are multiplying and not dividing
        Product::generateMultExprs(inputExprsDeref_re[0], inputExprsDeref_im[0], inputExprsDeref_re[1], inputExprsDeref_im[1],
                                   true, "", intermediateTypeCPUStore, normExpr, multResult_re, multResult_im);

        if(!normExpr.empty()){//Sanity check TODO: Remove
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - InnerProduct - Unexpected result from gerating multiply expression", getSharedPointer()));
        }

        //=== Add to accumulator ===
        cStatementQueue.push_back(accumulatorVar.getCVarName(false) + " += " + multResult_re + ";");
        if(intermediateTypeCPUStore.isComplex()){
            cStatementQueue.push_back(accumulatorVar.getCVarName(true) + " += " + multResult_im + ";");
        }

        //=== Close For Loop ===
        if(!input0DT.isScalar()){
            cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
        }

        //=== Cast accumulator vars to output type and output vars (single values) (if casting needed) ===
        if(outputVar != accumulatorVar){
            cStatementQueue.push_back(outputVar.getCVarDecl(false, false, false, false, false) + " = " +
                                              DataType::cConvertType(accumulatorVar.getCVarName(false), intermediateTypeCPUStore, outputType)+ ";");
            if(intermediateTypeCPUStore.isComplex()){
                cStatementQueue.push_back(outputVar.getCVarDecl(true, false, false, false, false) + " = " +
                                          DataType::cConvertType(accumulatorVar.getCVarName(true), intermediateTypeCPUStore, outputType)+ ";");
            }
        }

        emittedBefore = true;
    }

    //=== Return ouput variable (accumulator or output var if casting needed) ===
    return CExpr(outputVar.getCVarName(imag), true);
}

std::string InnerProduct::complexConjBehaviorToString(InnerProduct::ComplexConjBehavior complexConjBehavior) {
    if(complexConjBehavior == ComplexConjBehavior::FIRST){
        return "First";
    }else if(complexConjBehavior == ComplexConjBehavior::SECOND){
        return "Second";
    }else if(complexConjBehavior == ComplexConjBehavior::NONE){
        return "None";
    }

    throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown ComplexConjBehavior"));
}

InnerProduct::ComplexConjBehavior InnerProduct::parseComplexConjBehavior(std::string string) {
    if(string == "First"){
        return ComplexConjBehavior::FIRST;
    }else if(string == "Second"){
        return ComplexConjBehavior::SECOND;
    }else if(string == "None"){
        return ComplexConjBehavior::NONE;
    }

    throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown ComplexConjBehavior: " + string));
}

InnerProduct::ComplexConjBehavior InnerProduct::getComplexConjBehavior() const {
    return complexConjBehavior;
}

void InnerProduct::setComplexConjBehavior(InnerProduct::ComplexConjBehavior complexConjBehavior) {
    InnerProduct::complexConjBehavior = complexConjBehavior;
}