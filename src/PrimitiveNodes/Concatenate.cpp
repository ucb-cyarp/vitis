//
// Created by Christopher Yarp on 7/31/20.
//

#include "Concatenate.h"
#include "GraphMLTools/GraphMLDialect.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"

Concatenate::Concatenate() :concatDim(0) {

}

Concatenate::Concatenate(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), concatDim(0) {

}

Concatenate::Concatenate(std::shared_ptr<SubSystem> parent, Concatenate *orig) : PrimitiveNode(parent, orig), concatDim(orig->concatDim) {

}

std::shared_ptr<Concatenate>
Concatenate::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                               std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<Concatenate> newNode = NodeFactory::createNode<Concatenate>(parent);
    newNode->setId(id);
    newNode->setName(name);

    if(dialect == GraphMLDialect::VITIS){
        std::string concatDimStr = dataKeyValueMap.at("ConcatDim");
        int concatDim = std::stoi(concatDimStr);
        newNode->setConcatDim(concatDim);
    } else if(dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Name -- Inputs
        std::string mode = dataKeyValueMap.at("Mode");
        if(mode == "Vector"){
            newNode->setConcatDim(0);
        }else if(mode == "Multidimensional array"){
            std::string concatDimStr = dataKeyValueMap.at("Numeric.ConcatenateDimension");
            int concatDim = std::stoi(concatDimStr);
            newNode->setConcatDim(concatDim);
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown mode for Concatenate", newNode));
        }
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Concatenate", newNode));
    }

    return newNode;
}


std::set<GraphMLParameter> Concatenate::graphMLParameters() {
    std::set<GraphMLParameter> parameters;
    parameters.insert(GraphMLParameter("ConcatDim", "int", true));

    return parameters;
}

xercesc::DOMElement *
Concatenate::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Concatenate");
    GraphMLHelper::addDataNode(doc, thisNode, "ConcatDim", GeneralHelper::to_string(concatDim));

    return thisNode;
}

std::string Concatenate::typeNameStr() {
    return "Concatenate";
}

std::string Concatenate::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() +
             "\nConcatDim: " + GeneralHelper::to_string(concatDim);

    return label;
}

std::shared_ptr<Node> Concatenate::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Concatenate>(parent, this);
}

void Concatenate::validate() {
    Node::validate();

    if(inputPorts.size() < 2){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Concatenate - Should Have 2 or More Input Ports", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Concatenate - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    DataType outputDT = getOutputPort(0)->getDataType();
    DataType outputDTScalar = outputDT;
    outputDTScalar.setDimensions({1});

    //Check that inputs are consistent except for the dimension along which concatenation is occurring
    //Note: Will not expand number of dimensions.  Use a reshape block to expand the number of dimensions if needed.  The one exception is scalars which can be expanded to a vector or row/col vector
    int concatDimLen = 0;
    std::vector<int> dimensionConstraint = getInputPort(0)->getDataType().getDimensions();
    dimensionConstraint[concatDim] = 0;
    for(int i = 0; i<inputPorts.size(); i++){
        DataType inputDT = getInputPort(i)->getDataType();
        DataType inputDTScalar = inputDT;
        inputDTScalar.setDimensions({1});

        std::vector<int> portDim = inputDT.getDimensions();
        if(inputDT.isScalar()){
            //This is the one case where we will auto-expand since adding dimensions is unambiguous when expanding to a vector or row/col vector (does not matter if row major or col major)
            for(int dim = portDim.size(); dim<outputDT.getDimensions().size(); dim++){
                portDim.push_back(1);
            }
        }

        concatDimLen += portDim[concatDim];
        portDim[concatDim] = 0;
        if(portDim != dimensionConstraint){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Concatenate - Input Dimension " + GeneralHelper::to_string(i) + " does not match the first input (excluding the concatinate dimension).  Laminar does not expand the dimension automatically (due to row/column major semantic disagreement).  Use a reshape block to manually expand the dimension of the input if nessisary", getSharedPointer()));
        }

        if(inputDTScalar != outputDTScalar){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Input port (" + GeneralHelper::to_string(i) + ") type should match the output port", getSharedPointer()));
        }
    }

    //Check output port dimension
    dimensionConstraint[concatDim] = concatDimLen;
    if(getOutputPort(0)->getDataType().getDimensions() != dimensionConstraint){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Concatenate - Output Dimension Does Not Match Input Dimensions Concatenated Along Specified Dimension", getSharedPointer()));
    }
}

CExpr
Concatenate::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                       bool imag) {
    std::vector<CExpr> inputExprs;

    for (unsigned long i = 0; i < inputPorts.size(); i++) {
        std::shared_ptr<OutputPort> srcOutputPort = getInputPort(i)->getSrcOutputPort();
        int srcOutputPortNum = srcOutputPort->getPortNum();
        std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

        inputExprs.push_back(srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, imag));
    }

    std::string outputVarName = name + "_n" + GeneralHelper::to_string(id) + "_out";

    Variable outputVar = Variable(outputVarName, getOutputPort(0)->getDataType());

    cStatementQueue.push_back(outputVar.getCVarDecl(imag, true, false, true) + ";");

    int outputOffset = 0;
    for(unsigned long i = 0; i<inputExprs.size(); i++){
        DataType inputDT = getInputPort(i)->getDataType();

        std::vector<std::string> forLoopIndexVars;
        std::vector<std::string> forLoopClose;
        if(!inputDT.isScalar()) {
            //Create nested loops for a given array
            std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                    EmitterHelpers::generateVectorMatrixForLoops(inputDT.getDimensions());

            std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
            forLoopIndexVars = std::get<1>(forLoopStrs);
            forLoopClose = std::get<2>(forLoopStrs);

            cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());
        }

        //For now, relies on output being a vector
        std::vector<std::string> dstIndexVec;
        if(inputDT.isScalar()){
            //The one case we do auto-expand is when the input is a scalar.  It will expand to a vector
            std::string concatDimDstIndex = GeneralHelper::to_string(outputOffset);

            for(int dim = 0; dim < getOutputPort(0)->getDataType().getDimensions().size(); dim++){
                dstIndexVec.push_back("1");
            }

            dstIndexVec[concatDim] = concatDimDstIndex;
        }else{
            std::string concatDimDstIndex = GeneralHelper::to_string(outputOffset) + " + " + forLoopIndexVars[concatDim];
            dstIndexVec = forLoopIndexVars;
            dstIndexVec[concatDim] = concatDimDstIndex;
        }
        std::string assignDest = outputVar.getCVarName(imag) + EmitterHelpers::generateIndexOperation(dstIndexVec);

        //Relies on input being a scalar or array
        std::vector<std::string> emptyArr;
        std::string assignVal = inputExprs[i].getExprIndexed(inputDT.isScalar() ? emptyArr : forLoopIndexVars, true);

        cStatementQueue.push_back(assignDest + " = " + assignVal + ";");

        if(!inputDT.isScalar()) {
            //Close for loop
            cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
        }

        outputOffset += inputDT.getDimensions()[concatDim];
    }

    return CExpr(outputVar.getCVarName(imag), CExpr::ExprType::ARRAY);
}

int Concatenate::getConcatDim() const {
    return concatDim;
}

void Concatenate::setConcatDim(int concatDim) {
    Concatenate::concatDim = concatDim;
}
