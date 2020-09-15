//
// Created by Christopher Yarp on 7/31/20.
//

#include "Concatenate.h"
#include "GraphMLTools/GraphMLDialect.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"

Concatenate::Concatenate() {

}

Concatenate::Concatenate(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent) {

}

Concatenate::Concatenate(std::shared_ptr<SubSystem> parent, Concatenate *orig) : PrimitiveNode(parent, orig) {

}

std::shared_ptr<Concatenate>
Concatenate::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                               std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<Concatenate> newNode = NodeFactory::createNode<Concatenate>(parent);
    newNode->setId(id);
    newNode->setName(name);

    if(dialect == GraphMLDialect::VITIS){
        //TODO: Import parameters for multidimensional concat
    } else if(dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Name -- Inputs
        std::string mode = dataKeyValueMap.at("Mode");
        if(mode == "Vector"){
            //This is what we currently support
        }else if(mode == "Multidimensional array"){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Multidimensional array Concatenate is not currently supported", newNode));
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown mode for Concatenate", newNode));
        }
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Concatenate", newNode));
    }

    return newNode;
}


std::set<GraphMLParameter> Concatenate::graphMLParameters() {
    //TODO: update when parameters saved for multidimensional concat
    return Node::graphMLParameters();
}

xercesc::DOMElement *
Concatenate::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Concatenate");

    return thisNode;
}

std::string Concatenate::typeNameStr() {
    return "Concatenate";
}

std::string Concatenate::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr();

    return label;
}

std::shared_ptr<Node> Concatenate::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Concatenate>(parent, this);
}

void Concatenate::validate() {
    //TODO: implement multidimensional concat

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
    if(!outputDT.isVector()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Multidimensional array Concatenate is not currently supported, the output port should be a vector", getSharedPointer()));
    }

    int elements = 0;

    for(int i = 0; i<inputPorts.size(); i++){
        DataType inputDT = getInputPort(i)->getDataType();
        DataType inputDTScalar = inputDT;
        inputDTScalar.setDimensions({1});

        if(!inputDT.isScalar() && !inputDT.isVector()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Multidimensional array Concatenate is not currently supported, port " + GeneralHelper::to_string(i) + " is not a scalar or vector", getSharedPointer()));
        }

        if(inputDTScalar != outputDTScalar){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Input port (" + GeneralHelper::to_string(i) + ") type should match the output port", getSharedPointer()));
        }
        elements += inputDT.numberOfElements();
    }

    if(elements != outputDT.numberOfElements()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Number of elements in output should match the sum of the number of elements at the inputs", getSharedPointer()));
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
    //TODO: Change the offset to be multidimension when multidimensional arrays supported
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

        //TODO: Change to multidimensional for loop when multidimensional arrays supported
        //For now, relies on output being a vector
        std::string dstIndex = GeneralHelper::to_string(outputOffset) + (inputDT.isScalar() ? "" : " + " + forLoopIndexVars[0]);
        std::vector<std::string> dstIndexVec = {dstIndex};
        std::string assignDest = outputVar.getCVarName(imag) + EmitterHelpers::generateIndexOperation(dstIndexVec);

        //Relies on input being a scalar or vector
        std::vector<std::string> emptyArr;
        std::string assignVal = inputExprs[i].getExprIndexed(inputDT.isScalar() ? emptyArr : forLoopIndexVars, true);

        cStatementQueue.push_back(assignDest + " = " + assignVal + ";");

        if(!inputDT.isScalar()) {
            //Close for loop
            cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
        }

        outputOffset += inputDT.numberOfElements();
    }

    return CExpr(outputVar.getCVarName(imag), CExpr::ExprType::ARRAY);
}
