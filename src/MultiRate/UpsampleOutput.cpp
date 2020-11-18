//
// Created by Christopher Yarp on 4/29/20.
//

#include "UpsampleOutput.h"
#include "General/ErrorHelpers.h"
#include "MultiRateHelpers.h"
#include "GraphCore/StateUpdate.h"
#include "GraphCore/ContextFamilyContainer.h"
#include "GraphCore/ContextContainer.h"

UpsampleOutput::UpsampleOutput() {

}

UpsampleOutput::UpsampleOutput(std::shared_ptr<SubSystem> parent) : Upsample(parent) {

}

UpsampleOutput::UpsampleOutput(std::shared_ptr<SubSystem> parent, UpsampleOutput *orig) : Upsample(parent, orig), stateVar(orig->stateVar) {

}

Variable UpsampleOutput::getStateVar() const {
    return stateVar;
}

void UpsampleOutput::setStateVar(const Variable &stateVar) {
    UpsampleOutput::stateVar = stateVar;
}

xercesc::DOMElement *
UpsampleOutput::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "RateChange");
    }

    emitGraphMLProperties(doc, thisNode);

    return thisNode;
}

std::set<GraphMLParameter> UpsampleOutput::graphMLParameters() {
    std::set<GraphMLParameter> params = Upsample::graphMLParameters();

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this

    return params;
}

std::shared_ptr<UpsampleOutput>
UpsampleOutput::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                  std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<UpsampleOutput> newNode = NodeFactory::createNode<UpsampleOutput>(parent);

    newNode->populateUpsampleParametersFromGraphML(id, name, dataKeyValueMap, dialect);

    std::string initialConditionStr;

    return newNode;
}

std::string UpsampleOutput::typeNameStr() {
    return "UpsampleOutput";
}

std::string UpsampleOutput::labelStr() {
    std::string label = Upsample::labelStr();

    return label;
}

void UpsampleOutput::validate() {
    Upsample::validate();

    std::shared_ptr<UpsampleOutput> thisAsUpsampleOutput = std::static_pointer_cast<UpsampleOutput>(getSharedPointer());
    MultiRateHelpers::validateRateChangeOutput_SetMasterRates(thisAsUpsampleOutput, false);
}

std::shared_ptr<Node> UpsampleOutput::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<UpsampleOutput>(parent, this);
}

bool UpsampleOutput::hasState() {
    return true;
}

bool UpsampleOutput::hasCombinationalPath() {
    return true;
}

bool UpsampleOutput::createStateUpdateNode(std::vector<std::shared_ptr<Node>> &new_nodes,
                                           std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                           std::vector<std::shared_ptr<Arc>> &new_arcs,
                                           std::vector<std::shared_ptr<Arc>> &deleted_arcs, bool includeContext) {

    if(!includeContext) {
        throw std::runtime_error(ErrorHelpers::genErrorStr("UpsampleOutput StateUpdate creation requires contexts to be present and includeContexts to be enabled", getSharedPointer()));
    }

    //==== Add Latch State Update Node.  This is the same as RepeatOutput ====
    std::shared_ptr<StateUpdate> stateUpdateLatch = NodeFactory::createNode<StateUpdate>(getParent());
    stateUpdateLatch->setName("StateUpdate-For-" + getName() + "-Latch");
    stateUpdateLatch->setPartitionNum(partitionNum);
    stateUpdateLatch->setPrimaryNode(getSharedPointer());
    addStateUpdateNode(stateUpdateLatch); //Set the state update node pointer in this node

    //Set context to be the same as the primary node.  This should be subcontext 0 of the ContextRoot
    std::vector<Context> primarayContext = getContext();
    stateUpdateLatch->setContext(primarayContext);

    //Add node to the lowest level context
    Context specificContext = primarayContext[primarayContext.size() - 1];
    specificContext.getContextRoot()->addSubContextNode(specificContext.getSubContext(), stateUpdateLatch);

    new_nodes.push_back(stateUpdateLatch);

    //rewire input arc
    std::set<std::shared_ptr<Arc>> inputArcs = getInputPort(0)->getArcs();
    if(inputArcs.size() == 0){
        throw std::runtime_error("Encountered UpsampleOutput node with no inputs when creating state update");
    }

    std::shared_ptr<InputPort> origDestPort = (*inputArcs.begin())->getDstPort();
    DataType origDataType = (*inputArcs.begin())->getDataType();
    double origSampleTime = (*inputArcs.begin())->getSampleTime();

    (*inputArcs.begin())->setDstPortUpdateNewUpdatePrev(stateUpdateLatch->getInputPortCreateIfNot(0));

    //Create new
    std::shared_ptr<Arc> newArc = Arc::connectNodes(stateUpdateLatch->getOutputPortCreateIfNot(0), origDestPort, origDataType, origSampleTime);

    new_arcs.push_back(newArc);

    //==== Add State Update Node to ContextFamilyContainer subcontext 1 ====
    //This node has no order constraint arcs
    std::vector<Context> zeroFillContext = primarayContext;
    //Change the subcontext of the lowest level
    zeroFillContext[zeroFillContext.size()-1].setSubContext(1);

    //Find parent
    std::shared_ptr<SubSystem> zeroParentNode;
    std::shared_ptr<ContextRoot> clockDomain = zeroFillContext[zeroFillContext.size()-1].getContextRoot();
    std::map<int, std::shared_ptr<ContextFamilyContainer>> contextFamilyContainers = clockDomain->getContextFamilyContainers();
    if(clockDomain->getContextFamilyContainers().size() > 0){
        //Encapsulation has already happened, place this node in the contextContainer for subcontext 1
        auto contextContainer = contextFamilyContainers.find(partitionNum);
        if(contextContainer == contextFamilyContainers.end()){
            throw std::runtime_error("Could not find expected ContextFamilyContainer when creating StateUpdateNodes for ");
        }

        zeroParentNode = contextContainer->second->getSubContextContainer(1);
    }else{
        //Encapsulation has not happened yet, use the same parent as this node
        zeroParentNode = parent;
    }

    std::shared_ptr<StateUpdate> stateUpdateZero = NodeFactory::createNode<StateUpdate>(zeroParentNode);
    stateUpdateZero->setName("StateUpdate-For-" + getName() + "-Zero");
    stateUpdateZero->setPartitionNum(partitionNum);
    stateUpdateZero->setContext(zeroFillContext);
    stateUpdateZero->setPrimaryNode(getSharedPointer());
    addStateUpdateNode(stateUpdateZero);

    new_nodes.push_back(stateUpdateZero);

    return true;
}

CExpr UpsampleOutput::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                int outputPortNum, bool imag) {
    //TODO: Implement Vector Support
    if(!getInputPort(0)->getDataType().isScalar()){
        throw std::runtime_error("C Emit Error - UpsampleOutput Support for Vector Types has Not Yet Been Implemented");
    }

    //Return the state var name as the expression
    //Return the simple name (no index needed as it is not an array
    return CExpr(stateVar.getCVarName(imag), stateVar.getDataType().isScalar() ? CExpr::ExprType::SCALAR_VAR : CExpr::ExprType::ARRAY); //This is a variable name therefore inform the cEmit function
}

std::vector<Variable> UpsampleOutput::getCStateVars() {
    std::vector<Variable> vars;

    DataType stateType = getInputPort(0)->getDataType();

    //TODO: Extend to support vectors (must declare 2D array for state)

    std::string varName = name+"_n"+GeneralHelper::to_string(id)+"_state";
    Variable var = Variable(varName, stateType, initCond);
    stateVar = var;

    vars.push_back(var);

    return vars;
}

bool UpsampleOutput::isSpecialized() {
    return true;
}

void UpsampleOutput::emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType,
                                      std::shared_ptr<StateUpdate> stateUpdateSrc) {

    //TODO: Remove this sanity check
    if(stateUpdateSrc->getPrimaryNode() != getSharedPointer()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("When emitting the Upsample StateUpdate, the primary node of the StateUpdate node is not the UpsampleOutput node", getSharedPointer()));
    }

    std::vector<Context> updateContext = stateUpdateSrc->getContext();
    if(updateContext.size()<=0){
        throw std::runtime_error(ErrorHelpers::genErrorStr("When emitting the Upsample StateUpdate, the state update node did not have a context", getSharedPointer()));
    }

    int subContextNum = updateContext[updateContext.size()-1].getSubContext();

    DataType inputDataType = getInputPort(0)->getDataType();

    if(subContextNum == 0) {
        //This is the latching logic
        std::shared_ptr<OutputPort> srcPort = getInputPort(0)->getSrcOutputPort();
        int srcOutPortNum = srcPort->getPortNum();
        std::shared_ptr<Node> srcNode = srcPort->getParent();

        //Emit the upstream
        CExpr inputExprRe = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, false);
        CExpr inputExprIm;

        if (inputDataType.isComplex()) {
            inputExprIm = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, true);
        }

        //TODO: Implement Vector Support

        //The state variable is not an array
        //TODO: Modify when vector support added
        if(inputExprRe.isArrayOrBuffer()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Vector/matrices are not currently supported in UpsampleOutput", getSharedPointer()));
        }
        cStatementQueue.push_back(stateVar.getCVarName(false) + " = " + inputExprRe.getExpr() + ";");

        if (stateVar.getDataType().isComplex()) {
            //TODO: Modify when vector support added
            if(inputExprRe.isArrayOrBuffer()){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Vector/matrices are not currently supported in UpsampleOutput", getSharedPointer()));
            }
            cStatementQueue.push_back(stateVar.getCVarName(true) + " = " + inputExprIm.getExpr() + ";");
        }
    }else if(subContextNum == 1){
        //This is the zero fill logic
        NumericValue zeroConst((long) 0);

        //TODO: Implement Vector Support

        //Set the statr
        cStatementQueue.push_back(stateVar.getCVarName(false) + " = " + zeroConst.toStringComponent(false, inputDataType) + ";");

        if (stateVar.getDataType().isComplex()) {
            cStatementQueue.push_back(stateVar.getCVarName(true) + " = " + zeroConst.toStringComponent(true, inputDataType) + ";");
        }
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("When emitting the Upsample StateUpdate, the state update node had an unexpected subcontext number", getSharedPointer()));
    }
}