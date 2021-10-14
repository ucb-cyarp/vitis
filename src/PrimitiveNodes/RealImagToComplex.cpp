//
// Created by Christopher Yarp on 9/10/18.
//

#include "RealImagToComplex.h"
#include "GraphCore/NodeFactory.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"

RealImagToComplex::RealImagToComplex() {

}

RealImagToComplex::RealImagToComplex(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent) {

}

std::shared_ptr<RealImagToComplex>
RealImagToComplex::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                     std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<RealImagToComplex> newNode = NodeFactory::createNode<RealImagToComplex>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //There are no properties to import

    return newNode;
}

std::set<GraphMLParameter> RealImagToComplex::graphMLParameters() {
    //No Parameters
    return Node::graphMLParameters();
}

xercesc::DOMElement *RealImagToComplex::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode,
                                                    bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "RealImagToComplex");

    return thisNode;
}

std::string RealImagToComplex::typeNameStr(){
    return "RealImagToComplex";
}

std::string RealImagToComplex::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr();

    return label;
}

void RealImagToComplex::validate() {
    Node::validate();

    if(inputPorts.size() != 2){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - RealImagToComplex - Should Have Exactly 2 Input Ports", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - RealImagToComplex - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    //Check that input is complex, and the outputs are real
    if(!(getOutputPort(0)->getDataType().isComplex())){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - RealImagToComplex - Output Port Must Be Complex", getSharedPointer()));
    }

    if(getInputPort(0)->getDataType().isComplex()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - RealImagToComplex - Input Port 0 Must Be Real", getSharedPointer()));
    }

    if(getInputPort(1)->getDataType().isComplex()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - RealImagToComplex - Input Port 1 Must Be Real", getSharedPointer()));
    }

    if(getInputPort(0)->getDataType().getDimensions() != getOutputPort(0)->getDataType().getDimensions() || getInputPort(1)->getDataType().getDimensions() != getOutputPort(0)->getDataType().getDimensions()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - RealImagToComplex - Inputs and Outputs Should have Same Dimensions", getSharedPointer()));

    }
}

CExpr RealImagToComplex::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    std::shared_ptr<OutputPort> srcOutputPort;
    int srcOutputPortNum;
    std::shared_ptr<Node> srcNode;

    if(imag){
        //Input Port 1 Is Imag
        srcOutputPort = getInputPort(1)->getSrcOutputPort();
        srcOutputPortNum = srcOutputPort->getPortNum();
        srcNode = srcOutputPort->getParent();
    }else{
        //Input Port 0 is Real
        srcOutputPort = getInputPort(0)->getSrcOutputPort();
        srcOutputPortNum = srcOutputPort->getPortNum();
        srcNode = srcOutputPort->getParent();
    }

    CExpr emitExpr = srcNode->emitC(cStatementQueue, schedType, srcOutputPortNum, false); //The input was validated to be real

    //Do not copy but simply pass through
    emitExpr.setIsReferenceExpr(true);

    cStatementQueue.push_back("//" + (imag ? std::string("Imag") : std::string("Real")) + "->Complex Passthrough: " + emitExpr.getExpr());

    return emitExpr;
}

RealImagToComplex::RealImagToComplex(std::shared_ptr<SubSystem> parent, RealImagToComplex* orig) : PrimitiveNode(parent, orig) {
    //Nothing else to copy, call superclass constructor
}

std::shared_ptr<Node> RealImagToComplex::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<RealImagToComplex>(parent, this);
}

bool RealImagToComplex::passesThroughInputs() {
    return true;
}

bool RealImagToComplex::specializesForBlocking() {
    return true;
}

void RealImagToComplex::specializeForBlocking(int localBlockingLength,
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