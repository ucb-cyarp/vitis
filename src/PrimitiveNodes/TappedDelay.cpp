//
// Created by Christopher Yarp on 2019-03-15.
//

#include <General/GeneralHelper.h>
#include "TappedDelay.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"

TappedDelay::TappedDelay() {

}

TappedDelay::TappedDelay(std::shared_ptr<SubSystem> parent) : Delay(parent) {

}

TappedDelay::TappedDelay(std::shared_ptr<SubSystem> parent, TappedDelay *orig) : Delay(parent, orig) {

}

std::shared_ptr<TappedDelay>
TappedDelay::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                               std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<TappedDelay> newNode = NodeFactory::createNode<TappedDelay>(parent);

    //Import Delay base properties
    populatePropertiesFromGraphML(newNode, "Numeric.NumDelays", "Numeric.vinit", id, name, dataKeyValueMap, parent, dialect);

    //Override the TappedDelay specific properties (earliestFirst, allocateExtraSpace)
    if (dialect == GraphMLDialect::VITIS) {
        std::string earliestFirstStr = dataKeyValueMap.at("EarliestFirst");
        newNode->earliestFirst = GeneralHelper::parseBool(earliestFirstStr);
        std::string allocateExtraSpaceStr = dataKeyValueMap.at("IncludeCurrent");
        newNode->allocateExtraSpace = GeneralHelper::parseBool(allocateExtraSpaceStr);
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        std::string orderStr = dataKeyValueMap.at("DelayOrder");
        if(orderStr == "Newest"){
            newNode->earliestFirst = true;
        }else if(orderStr == "Oldest"){
            newNode->earliestFirst = false;
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown DelayOrder: " + orderStr + " - TappedDelay", newNode));
        }

        std::string includeCurrentStr = dataKeyValueMap.at("includeCurrent");
        if(includeCurrentStr == "on"){
            newNode->allocateExtraSpace = true;
        }else if(includeCurrentStr == "off"){
            newNode->allocateExtraSpace = false;
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown includeCurrent: " + orderStr + " - TappedDelay", newNode));
        }
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - TappedDelay", newNode));
    }

    return newNode;
}

bool TappedDelay::hasCombinationalPath() {
    return allocateExtraSpace;
}

std::set<GraphMLParameter> TappedDelay::graphMLParameters() {
    std::set<GraphMLParameter> parameters = Delay::graphMLParameters();

    parameters.insert(GraphMLParameter("EarliestFirst", "boolean", true));
    parameters.insert(GraphMLParameter("IncludeCurrent", "boolean", true));

    return parameters;
}

std::string TappedDelay::typeNameStr() {
    return "TappedDelay";
}

std::string TappedDelay::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() +
             "\nDelayLength: " + GeneralHelper::to_string(delayValue) +
             "\nInitialCondition: " + NumericValue::toString(initCondition) +
             "\nEarliestFirst: " + GeneralHelper::to_string(earliestFirst) +
             "\nIncludeCurrent: " + GeneralHelper::to_string(allocateExtraSpace);

    return label;
}

std::shared_ptr<Node> TappedDelay::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<TappedDelay>(parent, this);
}

xercesc::DOMElement *
TappedDelay::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "TappedDelay");

    //This emits the core Delay parameters
    emitGraphMLDelayParams(doc, thisNode);

    //Emit the TappedDelay Specific Parameters
    GraphMLHelper::addDataNode(doc, thisNode, "EarliestFirst", GeneralHelper::to_string(earliestFirst));

    GraphMLHelper::addDataNode(doc, thisNode, "IncludeCurrent", GeneralHelper::to_string(allocateExtraSpace));

    return thisNode;
}

void TappedDelay::validate() {
    Node::validate();

    //Should have 1 input ports and 1 output port
    if(inputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - TappedDelay - Should Have Exactly 1 Input Port", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - TappedDelay - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    if(delayValue <1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - TappedDelay - Requires a Delay of at least 1", getSharedPointer()));
    }

    //Check that input port and the output port have the same type
    DataType outType = getOutputPort(0)->getDataType();
    DataType inType = getInputPort(0)->getDataType();

    DataType outTypeDim1 = outType;
    outTypeDim1.setDimensions({1});
    DataType inTypeDim1 = inType;
    inTypeDim1.setDimensions({1});

    if(outTypeDim1 != inTypeDim1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - TappedDelay - DataType of Input Port Does not Match Output Port", getSharedPointer()));
    }

    int delayLen = delayValue;
    if(allocateExtraSpace){
        delayLen++;
    }
    if (delayLen != outType.numberOfElements()/inType.numberOfElements()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - TappedDelay - Output must be the length of the delay (+1 if current value passed)", getSharedPointer()));
    }

    int expectedInitCondSize = outType.numberOfElements();
    int initConditionSize = initCondition.size();
    if(initConditionSize != expectedInitCondSize){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - TappedDelay - Number of initial conditions must match the number of elements in the output", getSharedPointer()));
    }
}

CExpr
TappedDelay::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                       bool imag) {

    //We need to pass the current value through if requested
    if(allocateExtraSpace){
        std::shared_ptr<OutputPort> srcPort = getInputPort(0)->getSrcOutputPort();
        int srcOutPortNum = srcPort->getPortNum();
        std::shared_ptr<Node> srcNode = srcPort->getParent();

        std::string inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, imag);

        //It is possible for the input to be a vector, emit a for loop if nessasary
        DataType inputDT = getInputPort(0)->getDataType();

        //=== Open for Loop ===
        std::vector<std::string> forLoopIndexVars;
        std::vector<std::string> forLoopClose;
        if(!inputDT.isScalar()){
            std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                    EmitterHelpers::generateVectorMatrixForLoops(inputDT.getDimensions());

            std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
            forLoopIndexVars = std::get<1>(forLoopStrs);
            forLoopClose = std::get<2>(forLoopStrs);

            cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());
        }

        //Because there is a minimum delay of 0, the state variable has at least 2 elements (and can be dereferenced)
        std::string assignTo = cStateVar.getCVarName(imag) + (earliestFirst ? "[0]" : "[" + GeneralHelper::to_string(delayValue) + "]" ); //Note not delayValue-1 because the array was extended by 1

        std::string assignToDeref = assignTo + (inputDT.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars));
        std::string assignFromDeref = inputExpr + (inputDT.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars));

        cStatementQueue.push_back(assignToDeref + " = " + assignFromDeref + ";");

        //=== Close for Loop ===
        if(!inputDT.isScalar()){
            cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
        }
    }

    //Return the variable without any dereferencing
    return CExpr(cStateVar.getCVarName(imag), true);
}

//TODO: Update delay state update to avoid making an extra copy of the input when an extra space is allocated and
//      the current value is inserted in the extra space.  Allows the shift logic to be used for the entire update
//      with no special casing of the input into the delay
