//
// Created by Christopher Yarp on 8/2/20.
//

#include "SimulinkSelect.h"
#include "General/ErrorHelpers.h"
#include "PrimitiveNodes/Constant.h"
#include "PrimitiveNodes/Sum.h"

SimulinkSelect::SimulinkSelect() : oneBased(false), mode(Select::SelectMode::INDEX_VECTOR), fromDialog(false), outputLen(1) {

}

SimulinkSelect::SimulinkSelect(std::shared_ptr<SubSystem> parent) : MediumLevelNode(parent), oneBased(false), mode(Select::SelectMode::INDEX_VECTOR), fromDialog(false), outputLen(1) {

}

SimulinkSelect::SimulinkSelect(std::shared_ptr<SubSystem> parent, SimulinkSelect *orig) : MediumLevelNode(parent, orig), oneBased(orig->oneBased), mode(orig->mode), dialogIndexes(orig->dialogIndexes), fromDialog(orig->fromDialog), outputLen(orig->outputLen) {

}

bool SimulinkSelect::isOneBased() const {
    return oneBased;
}

void SimulinkSelect::setOneBased(bool oneBased) {
    SimulinkSelect::oneBased = oneBased;
}

Select::SelectMode SimulinkSelect::getMode() const {
    return mode;
}

void SimulinkSelect::setMode(Select::SelectMode mode) {
    SimulinkSelect::mode = mode;
}

std::vector<NumericValue> SimulinkSelect::getDialogIndexes() const {
    return dialogIndexes;
}

void SimulinkSelect::setDialogIndexes(const std::vector<NumericValue> &dialogIndexes) {
    SimulinkSelect::dialogIndexes = dialogIndexes;
}

bool SimulinkSelect::isFromDialog() const {
    return fromDialog;
}

void SimulinkSelect::setFromDialog(bool fromDialog) {
    SimulinkSelect::fromDialog = fromDialog;
}

int SimulinkSelect::getOutputLen() const {
    return outputLen;
}

void SimulinkSelect::setOutputLen(int outputLen) {
    SimulinkSelect::outputLen = outputLen;
}


std::shared_ptr<SimulinkSelect>
SimulinkSelect::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                  std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node & Set Common Properties ====
    std::shared_ptr<SimulinkSelect> newNode = NodeFactory::createNode<SimulinkSelect>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    std::string dialogIndexesStr;
    std::string outputLenStr;
    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- OneBased, Mode, DialogIndexes, FromDialog, OutputLen
        std::string oneBasedStr = dataKeyValueMap.at("OneBased");
        newNode->oneBased = GeneralHelper::parseBool(oneBasedStr);
        std::string modeStr = dataKeyValueMap.at("Mode");
        newNode->mode = Select::parseSelectModeStr(modeStr);
        std::string fromDialogStr = dataKeyValueMap.at("FromDialog");
        newNode->fromDialog = GeneralHelper::parseBool(fromDialogStr);
        outputLenStr = dataKeyValueMap.at("OutputLen");
        dialogIndexesStr = dataKeyValueMap.at("DialogIndexes");
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Names - 1st Dim Only -- Numeric.index_mode, IndexOption1st,   Numeric.IndexParam1st,   Numeric.OutputSize1st
        //Simulink Names - All Dims     -- Numeric.index_mode, IndexOptionArray, Numeric.IndexParamArray, Numeric.OutputSizeArray
        std::string baseStr = dataKeyValueMap.at("Numeric.index_mode");
        int baseInt = std::stoi(baseStr);
        if(baseInt == 0){
            newNode->oneBased = false;
        }else if(baseInt == 1){
            newNode->oneBased = true;
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown base type for SimulinkSelect", newNode));
        }

        std::string modeStr = dataKeyValueMap.at("IndexOption1st");
        if(modeStr == "Index vector (dialog)"){
            newNode->mode = Select::SelectMode::INDEX_VECTOR;
            newNode->fromDialog = true;
        }else if(modeStr == "Index vector (port)"){
            newNode->mode = Select::SelectMode::INDEX_VECTOR;
            newNode->fromDialog = false;
        }else if(modeStr == "Starting index (dialog)"){
            newNode->mode = Select::SelectMode::OFFSET_LENGTH;
            newNode->fromDialog = true;
        }else if(modeStr == "Starting index (port)"){
            newNode->mode = Select::SelectMode::OFFSET_LENGTH;
            newNode->fromDialog = false;
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Indexing Mode is Currently Unsupported in SimulinkSelect: " + modeStr, newNode));
        }

        outputLenStr = dataKeyValueMap.at("Numeric.OutputSize1st");

        dialogIndexesStr = dataKeyValueMap.at("Numeric.IndexParam1st");
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - SimulinkSelect", newNode));
    }

    newNode->outputLen = std::stoi(outputLenStr);

    std::vector<NumericValue> dialogIndexes = NumericValue::parseXMLString(dialogIndexesStr);
    newNode->dialogIndexes = dialogIndexes;

    return newNode;
}

std::set<GraphMLParameter> SimulinkSelect::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //OneBased, Mode, DialogIndexes, FromDialog
    parameters.insert(GraphMLParameter("OneBased", "boolean", true));
    parameters.insert(GraphMLParameter("Mode", "string", true));
    parameters.insert(GraphMLParameter("DialogIndexes", "string", true));
    parameters.insert(GraphMLParameter("OutputLen", "int", true));
    parameters.insert(GraphMLParameter("FromDialog", "boolean", true));

    return parameters;
}

xercesc::DOMElement *
SimulinkSelect::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "SimulinkSelect");
    GraphMLHelper::addDataNode(doc, thisNode, "OneBased", GeneralHelper::to_string(oneBased));
    GraphMLHelper::addDataNode(doc, thisNode, "Mode", Select::selectModeToStr(mode));
    GraphMLHelper::addDataNode(doc, thisNode, "DialogIndexes", NumericValue::toString(dialogIndexes));
    GraphMLHelper::addDataNode(doc, thisNode, "OutputLen", GeneralHelper::to_string(outputLen));
    GraphMLHelper::addDataNode(doc, thisNode, "FromDialog", GeneralHelper::to_string(fromDialog));

    return thisNode;
}

std::string SimulinkSelect::typeNameStr() {
    return "SimulinkSelect";
}

std::string SimulinkSelect::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() +
             "\nOneBased: " + GeneralHelper::to_string(oneBased) +
             "\nMode: " + Select::selectModeToStr(mode) +
             "\nDialogIndexes: " + NumericValue::toString(dialogIndexes) +
             "\nOutputLen: " + GeneralHelper::to_string(outputLen) +
             "\nFromDialog: " + GeneralHelper::to_string(fromDialog);

    return label;
}

std::shared_ptr<Node> SimulinkSelect::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<SimulinkSelect>(parent, this);
}

void SimulinkSelect::validate() {
    Node::validate();

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    DataType outputDT = getOutputPort(0)->getDataType();
    DataType outputDTScalar = outputDT;
    outputDTScalar.setDimensions({1});
    DataType inputDT = getInputPort(0)->getDataType();
    DataType inputDTScalar = inputDT;
    inputDTScalar.setDimensions({1});

    if(outputDTScalar != inputDTScalar){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Input and Output Should Have Same Type", getSharedPointer()));
    }

    if(!outputDT.isScalar() && !outputDT.isVector()){
        //TODO: Implement multidimensional
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Output Must be a Scalar or Vector", getSharedPointer()));
    }

    if(fromDialog){
        if(inputPorts.size() != 1){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Should Have Exactly 1 Input Port if Indexes From Dialog", getSharedPointer()));
        }

        //Check that they are real integers
        for(int i = 0; i<dialogIndexes.size(); i++){
            if(dialogIndexes[i].isFractional()){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Dialog Index Must be Integer", getSharedPointer()));
            }
            if(dialogIndexes[i].isComplex()){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Dialog Index Must be Real", getSharedPointer()));
            }
        }

        if(mode == Select::SelectMode::INDEX_VECTOR){
            if(outputDT.numberOfElements() != dialogIndexes.size()){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Output Should Have Same Number of Elements as Index Vector", getSharedPointer()));
            }
        }else if(mode == Select::SelectMode::OFFSET_LENGTH){
            if(dialogIndexes.size() != 1){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Index Should be a Scalar When Indexing Mode is Offset", getSharedPointer()));
            }

            if(outputLen != outputDT.numberOfElements()){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - For Offset/Length Mode, Output Vector Length Should Match OutputLen Parameter ", getSharedPointer()));
            }
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Unknown Mode Select", getSharedPointer()));
        }
    }else{
        if(inputPorts.size() != 2){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Should Have Exactly 2 Input Ports if Indexes From Port", getSharedPointer()));
        }

        //Check they are real integers
        DataType selectDT = getInputPort(1)->getDataType();

        if(selectDT.isFloatingPt()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Index Must be Integer", getSharedPointer()));
        }
        if(selectDT.isComplex()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Index Must be Real", getSharedPointer()));
        }

        if(mode == Select::SelectMode::INDEX_VECTOR){
            if(outputDT.numberOfElements() != selectDT.numberOfElements()){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Output Should Have Same Number of Elements as Index Vector", getSharedPointer()));
            }
        }else if(mode == Select::SelectMode::OFFSET_LENGTH){
            if(!selectDT.isScalar()){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Index Should be a Scalar When Indexing Mode is Offset", getSharedPointer()));
            }

            if(outputLen != outputDT.numberOfElements()){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - For Offset/Length Mode, Output Vector Length Should Match OutputLen Parameter ", getSharedPointer()));
            }
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Unknown Mode Select", getSharedPointer()));
        }
    }
}

std::shared_ptr<ExpandedNode>
SimulinkSelect::expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes,
                       std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                       std::shared_ptr<MasterUnconnected> &unconnected_master) {
    //Validate first to check that SimulinkSelect is properly configured
    validate();

    //---- Expand the node ----
    std::shared_ptr<SubSystem> thisParent = parent;

    //Create Expanded Node and Add to Parent
    std::shared_ptr<ExpandedNode> expandedNode = NodeFactory::createNode<ExpandedNode>(thisParent, shared_from_this());

    //Remove Current Node from Parent and Set Parent to nullptr
    if(thisParent != nullptr) {
        thisParent->removeChild(shared_from_this());
    }
    parent = nullptr;
    //Add This node to the list of nodes to remove from the node vector
    deleted_nodes.push_back(shared_from_this());

    //Add Expanded Node to Node List
    new_nodes.push_back(expandedNode);

    //++++ Create Select Node and Rewire ++++
    std::shared_ptr<Select> selectNode = NodeFactory::createNode<Select>(expandedNode);
    selectNode->setName("Select");
    selectNode->setMode(mode);
    selectNode->setOutputLen(outputLen);
    new_nodes.push_back(selectNode);

    //--Rewire Outputs--
    //Was validated to have only 1 output port
    std::set<std::shared_ptr<Arc>> outputArcs = getOutputPort(0)->getArcs();
    for(const auto & outputArc : outputArcs){
        outputArc->setSrcPortUpdateNewUpdatePrev(selectNode->getOutputPortCreateIfNot(0));
    }

    //--Rewire OrderConstraintOuput Arcs --
    std::set<std::shared_ptr<Arc>> orderConstraintOutputArcs = getOrderConstraintOutputArcs();
    for (const auto &orderConstraintOutputArc : orderConstraintOutputArcs) {
        orderConstraintOutputArc->setSrcPortUpdateNewUpdatePrev(
                selectNode->getOrderConstraintOutputPortCreateIfNot());
    }

    //--Rewire Input--
    std::set<std::shared_ptr<Arc>> inputArc = getInputPort(0)->getArcs();
    //Validated to only have 1 input
    (*inputArc.begin())->setDstPortUpdateNewUpdatePrev(selectNode->getInputPortCreateIfNot(0));

    //--Handle Index (either from Dialog or Port)--
    std::shared_ptr<OutputPort> indexSrcPort;
    DataType indexDT;
    if(fromDialog){
        //Create Constant
        std::shared_ptr<Constant> indConst = NodeFactory::createNode<Constant>(expandedNode);
        indConst->setName("IndConst");
        indConst->setValue(dialogIndexes);
        new_nodes.push_back(indConst);

        indexSrcPort = indConst->getOutputPortCreateIfNot(0);

        //Was validated that index was real and integer
        int numBits = 0;
        bool isSigned = false;
        for(int i = 0; i<dialogIndexes.size(); i++){
            isSigned |= dialogIndexes[i].isSigned();
        }

        for(int i = 0; i<dialogIndexes.size(); i++){
            int maxBitsRequired = dialogIndexes[i].numIntegerBits();
            if (!dialogIndexes[i].isSigned() && isSigned){
                maxBitsRequired++;
            }
            if(maxBitsRequired > numBits){
                numBits = maxBitsRequired;
            }
        }

        //TODO: support multidimensional
        int constElements = dialogIndexes.size();
        indexDT = DataType(false, isSigned, false, numBits, 0, {constElements});
        indexDT = indexDT.getCPUStorageType(); //Do not want extra masking operation
    }else{
        //Get src and datatype
        std::set<std::shared_ptr<Arc>> indArc = getInputPort(1)->getArcs();
        //Was validated to have 1 input arc durring validation
        indexSrcPort = (*indArc.begin())->getSrcPort();
        indexDT = (*indArc.begin())->getDataType();

        //Delete arc
        (*indArc.begin())->disconnect();
        deleted_arcs.push_back(*indArc.begin());
    }

    //--Wire Index Port of Select & Move Order Constrain Input--
    if(oneBased){
        //Create Constant and Sum Nodes
        std::shared_ptr<Constant> decrConst = NodeFactory::createNode<Constant>(expandedNode);
        decrConst->setName("DecrConst");
        decrConst->setValue({NumericValue((long)1)});
        new_nodes.push_back(decrConst);
        DataType decrConstDT = indexDT;
        decrConstDT.setDimensions({1});

        std::shared_ptr<Sum> decrSum = NodeFactory::createNode<Sum>(expandedNode);
        decrSum->setName("DecrSum");
        decrSum->setInputSign({true, false});  //Second port is minus
        new_nodes.push_back(decrSum);

        //Wire Index Arc
        std::shared_ptr<Arc> indArcIn = Arc::connectNodes(indexSrcPort, decrSum->getInputPortCreateIfNot(0), indexDT);
        new_arcs.push_back(indArcIn);

        //Wire decr and decr to select
        std::shared_ptr<Arc> constDecArc = Arc::connectNodes(decrConst->getOutputPortCreateIfNot(0), decrSum->getInputPortCreateIfNot(1), decrConstDT);
        new_arcs.push_back(constDecArc);
        std::shared_ptr<Arc> indArc = Arc::connectNodes(decrSum->getOutputPortCreateIfNot(0), selectNode->getInputPortCreateIfNot(1), indexDT);
        new_arcs.push_back(indArc);

        //Rewire Order Constraint Input Arcs to Sum
        std::set<std::shared_ptr<Arc>> orderConstraintInputArcs = getOrderConstraintInputArcs();
        for (const auto &orderConstraintInputArc : orderConstraintInputArcs) {
            orderConstraintInputArc->setDstPortUpdateNewUpdatePrev(
                    decrSum->getOrderConstraintInputPortCreateIfNot());
        }
    }else {
        //Rewire Index Arc
        std::shared_ptr<Arc> indArc = Arc::connectNodes(indexSrcPort, selectNode->getInputPortCreateIfNot(1), indexDT);
        new_arcs.push_back(indArc);

        //Rewire Order Constraint Input Arcs
        std::set<std::shared_ptr<Arc>> orderConstraintInputArcs = getOrderConstraintInputArcs();
        for (const auto &orderConstraintInputArc : orderConstraintInputArcs) {
            orderConstraintInputArc->setDstPortUpdateNewUpdatePrev(
                    selectNode->getOrderConstraintInputPortCreateIfNot());
        }
    }

    return expandedNode;
}