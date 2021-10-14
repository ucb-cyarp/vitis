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

    //Do not copy but simply pass through
    emitExpr.setIsReferenceExpr(true);

    cStatementQueue.push_back("//Complex->" + (real ? std::string("Real") : std::string("Imag")) + " Passthrough: " + emitExpr.getExpr());

    return emitExpr;
}

ComplexToRealImag::ComplexToRealImag(std::shared_ptr<SubSystem> parent, ComplexToRealImag* orig) : PrimitiveNode(
        parent, orig) {
    //Nothing new to copy, call superclass constructor
}

std::shared_ptr<Node> ComplexToRealImag::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<ComplexToRealImag>(parent, this);
}

bool ComplexToRealImag::passesThroughInputs() {
    return true;
}

bool ComplexToRealImag::specializesForBlocking() {
    return true;
}

void ComplexToRealImag::specializeForBlocking(int localBlockingLength,
                                              int localSubBlockingLength,
                                              std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                              std::vector<std::shared_ptr<Node>> &nodesToRemove,
                                              std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                              std::vector<std::shared_ptr<Arc>> &arcsToRemove,
                                              std::vector<std::shared_ptr<Node>> &nodesToRemoveFromTopLevel,
                                              std::map<std::shared_ptr<Arc>, int> &arcsWithDeferredBlockingExpansion) {
    //Just expand the arcs since this node simply passes through the input to the output

    //The input arcs should be expanded to the block length
    std::set<std::shared_ptr<Arc>> directInArcs = getDirectInputArcs();
    std::set<std::shared_ptr<Arc>> directOutArcs = getDirectOutputArcs();
    std::set<std::shared_ptr<Arc>> arcsToExpand = directInArcs;
    arcsToExpand.insert(directOutArcs.begin(), directOutArcs.end());

    for(const std::shared_ptr<Arc> &arc : arcsToExpand) {
        arcsWithDeferredBlockingExpansion.emplace(arc, localBlockingLength);
    }
}
