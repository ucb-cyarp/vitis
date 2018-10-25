//
// Created by Christopher Yarp on 7/16/18.
//

#include "Mux.h"
#include "GraphCore/Arc.h"
#include "GraphCore/NodeFactory.h"
#include "General/GeneralHelper.h"
#include "General/GraphAlgs.h"
#include "GraphCore/ContextRoot.h"
#include <iostream>

Mux::Mux() : booleanSelect(false) {
    selectorPort = std::unique_ptr<SelectPort>(new SelectPort(this, 0)); //Don't need to do this in init as a raw pointer is passed to the port
    //However, any call to get a shared_ptr of the node or port need to be conducted after a shared pointer has returned
}

Mux::Mux(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), booleanSelect(false) {
    selectorPort = std::unique_ptr<SelectPort>(new SelectPort(this, 0)); //Don't need to do this in init as a raw pointer is passed to the port
    //However, any call to get a shared_ptr of the node or port need to be conducted after a shared pointer has returned
}

std::shared_ptr<SelectPort> Mux::getSelectorPort() const {
    //Selector port should never be null as it is creates by the Mux constructor and accessors to change it to null do not exist.
    return selectorPort->getSharedPointerSelectPort();
}

bool Mux::isBooleanSelect() const {
    return booleanSelect;
}

void Mux::setBooleanSelect(bool booleanSelect) {
    Mux::booleanSelect = booleanSelect;
}

std::shared_ptr<Mux>
Mux::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                       std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Check Valid Import ====
    if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Not supported for simulink import
        throw std::runtime_error("Mux import is not supported for the SIMULINK_EXPORT dialect");
    } else if (dialect == GraphMLDialect::VITIS) {
        //==== Create Node and set common properties ====
        std::shared_ptr<Mux> newNode = NodeFactory::createNode<Mux>(parent);
        newNode->setId(id);
        newNode->setName(name);

        return newNode;
    } else {
        throw std::runtime_error("Unsupported Dialect when parsing XML - Mux");
    }
}

std::set<GraphMLParameter> Mux::graphMLParameters() {
    return std::set<GraphMLParameter>(); //No parameters to return
}

xercesc::DOMElement *
Mux::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Mux");

    return thisNode;
}

std::string Mux::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: Mux";

    return label;
}

void Mux::validate() {
    Node::validate();

    selectorPort->validate();

    if(inputPorts.size() < 1){
        throw std::runtime_error("Validation Failed - Mux - Should Have 1 or More Input Ports");
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - Mux - Should Have Exactly 1 Output Port");
    }

    //Check that the select port is real
    if((*selectorPort->getArcs().begin())->getDataType().isComplex()){
        throw std::runtime_error("Validation Failed - Mux - Select Port Cannot be Complex");
    }

    //warn if floating point type
    //TODO: enforce integer for select (need to rectify Simulink use of double)
    if((*selectorPort->getArcs().begin())->getDataType().isFloatingPt()){
        std::cerr << "Warning: MUX Select Port is Driven by Floating Point" << std::endl;
    }

    //Check that all input ports and the output port have the same type
    DataType outType = getOutputPort(0)->getDataType();
    unsigned long numInputPorts = inputPorts.size();

    for(unsigned long i = 0; i<numInputPorts; i++){
        DataType inType = getInputPort(i)->getDataType();

        if(inType != outType){
            throw std::runtime_error("Validation Failed - Mux - DataType of Input Port Does not Match Output Port");
        }
    }

}

void Mux::addSelectArcUpdatePrevUpdateArc(std::shared_ptr<Arc> arc) {

    //Set the dst port of the arc, updating the previous port and this one
    arc->setDstPortUpdateNewUpdatePrev(selectorPort->getSharedPointerInputPort());
}

bool Mux::hasInternalFanout(int inputPort, bool imag) {
    return true;
}

CExpr Mux::emitCExpr(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag) {

    if(getOutputPort(0)->getDataType().getWidth()>1){
        throw std::runtime_error("C Emit Error - Mux Support for Vector Types has Not Yet Been Implemented");
    }

    //Get the expression for the select line
    std::shared_ptr<OutputPort> selectSrcOutputPort = getSelectorPort()->getSrcOutputPort();
    int selectSrcOutputPortNum = selectSrcOutputPort->getPortNum();
    std::shared_ptr<Node> selectSrcNode = selectSrcOutputPort->getParent();

    std::string selectExpr = selectSrcNode->emitC(cStatementQueue, selectSrcOutputPortNum, imag);

    //Get the expressions for each input
    std::vector<std::string> inputExprs;

    unsigned long numInputPorts = inputPorts.size();
    for(unsigned long i = 0; i<numInputPorts; i++){
        std::shared_ptr<OutputPort> srcOutputPort = getInputPort(i)->getSrcOutputPort();
        int srcOutputPortNum = srcOutputPort->getPortNum();
        std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

        inputExprs.push_back(srcNode->emitC(cStatementQueue, srcOutputPortNum, imag));
    }

    //Declare output tmp var
    std::string outputVarName = name+"_n"+GeneralHelper::to_string(id)+"_out";
    DataType outType = getOutputPort(0)->getDataType();
    Variable outVar = Variable(outputVarName, outType);

    //TODO: This function currently implements a seperate conditional for real and imag.  Merge them in a later version
    //to do this, need to pull both real and imagionary components of each input, construct both, and emit.
    //a state variable would be needed to determine that the mux had already been emitted when the other component's emit
    //call is made (ie. if real emit is called first, imag emit should simply return the output var).
    if(outType.isComplex()){
        cStatementQueue.push_back(outVar.getCVarDecl(true, true, false, true) + ";");
    }else{
        cStatementQueue.push_back(outVar.getCVarDecl(false, true, false, true) + ";");
    }

    DataType selectDataType = getSelectorPort()->getDataType();
    if(selectDataType.isBool()){
        //if/else statement
        std::string ifExpr = "if(" + selectExpr + "){";
        cStatementQueue.push_back(ifExpr);

        std::string trueAssign;
        std::string falseAssign;
        if(booleanSelect){
            //In this case, port 0 is the true port and port 1 is the false port
            trueAssign = outVar.getCVarName(imag) + " = " + inputExprs[0] + ";";
            falseAssign = outVar.getCVarName(imag) + " = " + inputExprs[1] + ";";
        }else{
            //This takes a select statement type perspective with the input ports being considered numbers
            //false is 0 and true is 1.
            trueAssign = outVar.getCVarName(imag) + " = " + inputExprs[1] + ";";
            falseAssign = outVar.getCVarName(imag) + " = " + inputExprs[0] + ";";
        }

        cStatementQueue.push_back(trueAssign);
        cStatementQueue.push_back("}else{");
        cStatementQueue.push_back(falseAssign);
        cStatementQueue.push_back("}");
    }else{
        //switch statement
        std::string switchExpr = "switch(" + selectExpr + "){";
        cStatementQueue.push_back(switchExpr);

        for(unsigned long i = 0; i<(numInputPorts-1); i++){
            std::string caseExpr = "case " + GeneralHelper::to_string(i) + ":";
            cStatementQueue.push_back(caseExpr);
            std::string caseAssign = outVar.getCVarName(imag) + " = " + inputExprs[i] + ";";
            cStatementQueue.push_back(caseAssign);
            cStatementQueue.push_back("break;");
        }

        cStatementQueue.push_back("default:");
        std::string caseAssign = outVar.getCVarName(imag) + " = " + inputExprs[numInputPorts-1] + ";";
        cStatementQueue.push_back(caseAssign);
        cStatementQueue.push_back("break;");
        cStatementQueue.push_back("}");
    }

    return CExpr(outVar.getCVarName(imag), true); //This is a variable

}

Mux::Mux(std::shared_ptr<SubSystem> parent, Mux* orig) : PrimitiveNode(parent, orig), booleanSelect(orig->booleanSelect){
    //The select port is not copied but a new one is created
    selectorPort = std::unique_ptr<SelectPort>(new SelectPort(this, 0)); //Don't need to do this in init as a raw pointer is passed to the port
}

std::shared_ptr<Node> Mux::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Mux>(parent, this);
}

void Mux::cloneInputArcs(std::vector<std::shared_ptr<Arc>> &arcCopies,
                         std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode,
                         std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &origToCopyArc,
                         std::map<std::shared_ptr<Arc>, std::shared_ptr<Arc>> &copyToOrigArc) {
    //Copy the input arcs from standard input ports
    Node::cloneInputArcs(arcCopies, origToCopyNode, origToCopyArc, copyToOrigArc);

    //Copy the input arcs from the enable line
    std::set<std::shared_ptr<Arc>> selectPortArcs = selectorPort->getArcs();

    //Adding an arc to a mux node.  The copy of this node should be an Mux so we should be able to cast to it
    std::shared_ptr<Mux> clonedDstNode = std::dynamic_pointer_cast<Mux>(
            origToCopyNode[shared_from_this()]);

    //Itterate through the arcs and duplicate
    for (auto arcIt = selectPortArcs.begin(); arcIt != selectPortArcs.end(); arcIt++) {
        std::shared_ptr<Arc> origArc = (*arcIt);
        //Get Src Output Port Number and Src Node (as of now, all arcs origionate at an output port)
        std::shared_ptr<OutputPort> srcPort = origArc->getSrcPort();
        int srcPortNumber = srcPort->getPortNum();

        std::shared_ptr<Node> origSrcNode = srcPort->getParent();
        std::shared_ptr<Node> clonedSrcNode = origToCopyNode[origSrcNode];

        //Create the Cloned Arc
        std::shared_ptr<Arc> clonedArc = Arc::connectNodes(clonedSrcNode, srcPortNumber, clonedDstNode,
                                                           (*arcIt)->getDataType()); //This creates a new arc and connects them to the referenced node ports.  This method connects to the select port of the clone of this node
        clonedArc->shallowCopyPrameters(origArc.get());

        //Add to arc list and maps
        arcCopies.push_back(clonedArc);
        origToCopyArc[origArc] = clonedArc;
        copyToOrigArc[clonedArc] = origArc;

    }
}

std::set<std::shared_ptr<Arc>> Mux::disconnectInputs() {
    std::set<std::shared_ptr<Arc>> disconnectedArcs = Node::disconnectInputs(); //Remove the arcs connected to the standard ports

    //Remove arcs connected to the selector port
    std::set<std::shared_ptr<Arc>> selectArcs = selectorPort->getArcs();

    //Disconnect each of the arcs from both ends
    //We can do this without disturbing the port the set since the set here is a copy of the port set
    for(auto it = selectArcs.begin(); it != selectArcs.end(); it++){
        //These functions update the previous endpoints of the arc (ie. removes the arc from them)
        (*it)->setDstPortUpdateNewUpdatePrev(nullptr);
        (*it)->setSrcPortUpdateNewUpdatePrev(nullptr);

        //Add this arc to the list of arcs removed
        disconnectedArcs.insert(*it);
    }

    return disconnectedArcs;
}

std::set<std::shared_ptr<Node>> Mux::getConnectedInputNodes() {
    std::set<std::shared_ptr<Node>> connectedNodes = Node::getConnectedInputNodes(); //Get the nodes connected to the standard input ports

    //Get nodes connected to the selector port
    std::set<std::shared_ptr<Arc>> selectArcs = selectorPort->getArcs();

    for(auto it = selectArcs.begin(); it != selectArcs.end(); it++){
        connectedNodes.insert((*it)->getSrcPort()->getParent()); //The selector port is an input port
    }

    return connectedNodes;
}

unsigned long Mux::inDegree() {
    unsigned long count = Node::inDegree(); //Get the number of arcs connected to the standard input ports

    //Get the arcs connected to the select port
    count += selectorPort->getArcs().size();

    return count;
}

std::vector<std::shared_ptr<Arc>>
Mux::connectUnconnectedPortsToNode(std::shared_ptr<Node> connectToSrc, std::shared_ptr<Node> connectToSink,
                                   int srcPortNum, int sinkPortNum) {
    std::vector<std::shared_ptr<Arc>> newArcs = Node::connectUnconnectedPortsToNode(connectToSrc, connectToSink, srcPortNum, sinkPortNum); //Connect standard ports

    unsigned long numSelectArcs = selectorPort->getArcs().size();

    if(numSelectArcs == 0){
        //This port is unconnected, connect it to the given node.

        //Connect from given node to this node's input port
        //TODO: Use default datatype for now.  Perhaps change later?
        std::shared_ptr<Arc> arc = Arc::connectNodes(connectToSrc, srcPortNum, std::dynamic_pointer_cast<Mux>(shared_from_this()), DataType());
        newArcs.push_back(arc);
    }

    return newArcs;
}

std::vector<std::shared_ptr<InputPort>> Mux::getInputPortsIncludingSpecial() {
    //Get standard input ports
    std::vector<std::shared_ptr<InputPort>> portList = Node::getInputPortsIncludingSpecial();

    //Add select port
    portList.push_back(selectorPort->getSharedPointerSelectPort());

    return portList;
}

std::map<std::shared_ptr<InputPort>, std::set<std::shared_ptr<Node>>> Mux::discoverContexts() {
    std::map<std::shared_ptr<InputPort>, std::set<std::shared_ptr<Node>>> nodeContexts;

    //For muxes, contexts only extend from standard inputs
    for(unsigned long i = 0; i<inputPorts.size(); i++){
        std::shared_ptr<InputPort> inputPort = std::dynamic_pointer_cast<InputPort>(inputPorts[i]->getSharedPointer());

        std::map<std::shared_ptr<Arc>, bool> marks;
        nodeContexts[inputPort] = GraphAlgs::scopedTraceBackAndMark(inputPort, marks);
    }

    return nodeContexts;
}

void Mux::discoverAndMarkMuxContextsAtLevel(std::vector<std::shared_ptr<Mux>> muxes) {
    //Discover contexts for muxes
    std::map<std::shared_ptr<Mux>, std::map<std::shared_ptr<InputPort>, std::set<std::shared_ptr<Node>>>> muxContexts;
//    std::map<std::shared_ptr<Mux>, std::set<std::shared_ptr<Node>>> allNodesInMuxConexts;
    std::map<std::shared_ptr<Mux>, unsigned long> muxInContextCount;

    for(unsigned long i = 0; i<muxes.size(); i++){
        muxContexts[muxes[i]] = muxes[i]->discoverContexts();
    }

    //Set count to 0
    //TODO: may be redundant with map initialization
    for(unsigned long i = 0; i<muxes.size(); i++){
        muxInContextCount[muxes[i]] = 0;
    }

    for(auto muxPair = muxContexts.begin(); muxPair != muxContexts.end(); muxPair++){
        std::shared_ptr<Mux> mux = muxPair->first;
        std::map<std::shared_ptr<InputPort>, std::set<std::shared_ptr<Node>>> portContexts = muxPair->second;

        for(auto portPair = portContexts.begin(); portPair != portContexts.end(); portPair++){
            std::set<std::shared_ptr<Node>> nodesInContext = portPair->second;

            for(auto node = nodesInContext.begin(); node != nodesInContext.end(); node++){
//                allNodesInMuxConexts[mux].insert(*node);

                if(GeneralHelper::isType<Node, Mux>(*node) != nullptr){
                    muxInContextCount[std::dynamic_pointer_cast<Mux>(*node)]++;
                }
            }
        }
    }

    //Assert that the number of muxes in the map is unchanged
    //TODO: Remove
    if(muxInContextCount.size() != muxes.size()){
        throw std::runtime_error("Found unexpected muxes durring mux context discovery.");
    }

    //Get an ordered list of muxes by count
    std::vector<std::vector<std::shared_ptr<Mux>>> muxesByCount;

    for(auto muxCountPair = muxInContextCount.begin(); muxCountPair != muxInContextCount.end(); muxCountPair++){
        std::shared_ptr<Mux> mux = muxCountPair->first;
        unsigned long count = muxCountPair->second;

        while(muxesByCount.size() < (count+1)){
            muxesByCount.push_back(std::vector<std::shared_ptr<Mux>>());
        }

        muxesByCount[count].push_back(mux);
    }


    //THEORY:
    //start with muxes with 0 count
    //For muxes with count 0, re-set the context for the nodes & set mux nodes in context arrays
    //Set the context containers within the muxes with 0 count

    //Of the nodes in each mux's context, find the nodes with count 1 and repeat
    //  Take the context of the mux and append the appropriate context for each context created by this node
    //  Reset (update) the context of each node in the context to this new context.  Note, because this node will have
    //  Had it's context updated in the previous itteration, it uses its own context as the base

    //TODO: Remove
    if(muxesByCount.size() > 0){
        if(muxesByCount[0].size()==0){
            throw std::runtime_error("Discoverd muxes with count >0 without finding muxes of count ==0");
        }
    }

    //PRACTICE:
    //Iterate through list & validate difference between length of contexts
    for(unsigned long count = 0; count < muxesByCount.size(); count++){
        for(unsigned i = 0; i<muxesByCount[count].size(); i++){
            std::shared_ptr<Mux> mux = muxesByCount[count][i];

            std::vector<Context> contextStack = mux->getContext();

            //TODO: remove
            if(count > 0){//Do not check for level 0
                //Check context length difference
                //Get context this mux is within from the last entry in it's context
                if(contextStack.size()<count){
                    //Check for impossible case, the context stack needs to be at least as deep as the count
                    //It can also be deeper if this mux is in an enabled subsystem
                    throw std::runtime_error("Mux context had unexpected length");
                }

                Context parentContext = contextStack[contextStack.size()-1];
                if(GeneralHelper::isType<ContextRoot, Mux>(parentContext.getContextRoot()) != nullptr){
                    throw std::runtime_error("Last ContextRoot for this mux expected to be a mux");
                }

                std::shared_ptr<Mux> contextParent = std::dynamic_pointer_cast<Mux>(parentContext.getContextRoot());

                if(contextStack.size() != contextParent->getContext().size() + 1){
                    throw std::runtime_error("Unexpected difference in context stack sizes between this mux and the next mux in the hierarchy");
                }
            }

            std::vector<std::shared_ptr<InputPort>> inputPorts = mux->getInputPorts();
            for(unsigned long i = 0; i<inputPorts.size(); i++){
                int portNum = inputPorts[i]->getPortNum();

                std::vector<Context> newContext = contextStack;
                newContext.push_back(Context(mux, portNum));

                std::set<std::shared_ptr<Node>> nodesInContext = muxContexts[mux][inputPorts[i]];

                for(auto node = nodesInContext.begin(); node != nodesInContext.end(); node++){
                    //Set the nodes in context list for the mux
                    (*node)->setContext(newContext);

                    //set the context of the nodes in the context
                    mux->addSubContextNode(portNum, *node);
                }
            }
        }
    }

}
