//
// Created by Christopher Yarp on 9/10/18.
//

#include "ComplexToRealImag.h"
#include "GraphCore/NodeFactory.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"

ComplexToRealImag::ComplexToRealImag() {

}

ComplexToRealImag::ComplexToRealImag(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent) {

}

std::shared_ptr<ComplexToRealImag>
ComplexToRealImag::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                     std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<ComplexToRealImag> newNode = NodeFactory::createNode<ComplexToRealImag>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //There are no properties to import

    return newNode;
}

std::set<GraphMLParameter> ComplexToRealImag::graphMLParameters() {
    //No Parameters
    return Node::graphMLParameters();
}

xercesc::DOMElement *ComplexToRealImag::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode,
                                                    bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "ComplexToRealImag");

    return thisNode;
}

std::string ComplexToRealImag::typeNameStr(){
    return "ComplexToRealImag";
}

std::string ComplexToRealImag::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr();

    return label;
}

void ComplexToRealImag::validate() {
    Node::validate();

    if(inputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ComplexToRealImag - Should Have Exactly 1 Input Port", getSharedPointer()));
    }

    if(outputPorts.size() != 2){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ComplexToRealImag - Should Have Exactly 2 Output Ports", getSharedPointer()));
    }

    //Check that input is complex, and the outputs are real
    if(!(getInputPort(0)->getDataType().isComplex())){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ComplexToRealImag - Input Port Must Be Complex", getSharedPointer()));
    }

    if(getOutputPort(0)->getDataType().isComplex()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ComplexToRealImag - Output Port 0 Must Be Real", getSharedPointer()));
    }

    if(getOutputPort(1)->getDataType().isComplex()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ComplexToRealImag - Output Port 1 Must Be Real", getSharedPointer()));
    }

    if(getInputPort(0)->getDataType().getDimensions() != getOutputPort(0)->getDataType().getDimensions() || getInputPort(0)->getDataType().getDimensions() != getOutputPort(1)->getDataType().getDimensions()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - ComplexToRealImag - Inputs and Outputs Should have Same Dimensions", getSharedPointer()));

    }

}

CExpr ComplexToRealImag::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    //We can ignore imag since we validated that the outputs are both real

    std::shared_ptr<OutputPort> srcOutputPort = getInputPort(0)->getSrcOutputPort();
    int srcOutputPortNum = srcOutputPort->getPortNum();
    std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

    CExpr emitExpr;

    bool real = outputPortNum == 0;

    if(real) {
        //Output Port 0 Is Real
        emitExpr = srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, false);

    }else{ //if(outputPortNum == 1){ --Already validated the number of output ports is 2
        //Output Port 1 Is Imag
        emitExpr = srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, true);
    }

    //Already validated datatypes have the same dimensions
    DataType dataType = getOutputPort(0)->getDataType();

    if(!dataType.isScalar()){
        //TODO: Remove Temporary when StateUpdate insertion logic improved to track passthroughs
        //Need to create a temporary and copy values into it in a for loop

        //Declare tmp array and write to file
        std::string vecOutName = name+"_n"+GeneralHelper::to_string(id)+ "_outVec_" + (real ? "realCpnt" : "imagCpnt"); //Changed to
        Variable vecOutVar = Variable(vecOutName, dataType);
        cStatementQueue.push_back(vecOutVar.getCVarDecl(false, true, false, true, false) + ";");

        //Create nested loops for a given array
        std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                EmitterHelpers::generateVectorMatrixForLoops(dataType.getDimensions());

        std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
        std::vector<std::string> forLoopIndexVars = std::get<1>(forLoopStrs);
        std::vector<std::string> forLoopClose = std::get<2>(forLoopStrs);

        cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());

        //Assign to tmp in loop
        std::string assignExpr = vecOutVar.getCVarName(false) + EmitterHelpers::generateIndexOperation(forLoopIndexVars) + " = " + emitExpr.getExprIndexed(forLoopIndexVars, true);
        cStatementQueue.push_back(assignExpr + ";");

        //Close for loop
        cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());

        return CExpr(vecOutVar.getCVarName(false), CExpr::ExprType::ARRAY);
    }else{
        //Create a temporary variable to avoid issue if this node is directly attached to state
        //at the input.  The state update is placed after this node but the variable from the delay is simply
        //passed through.  This could cause the state to be update before the result is used.
        //TODO: Remove Temporary when StateUpdate insertion logic improved to track passthroughs
        //Accomplished by returning a SCALAR_EXPR instead of a SCALAR_VAR

        return CExpr(emitExpr.getExpr(), CExpr::ExprType::SCALAR_EXPR);
    }
}

ComplexToRealImag::ComplexToRealImag(std::shared_ptr<SubSystem> parent, ComplexToRealImag* orig) : PrimitiveNode(
        parent, orig) {
    //Nothing new to copy, call superclass constructor
}

std::shared_ptr<Node> ComplexToRealImag::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<ComplexToRealImag>(parent, this);
}
