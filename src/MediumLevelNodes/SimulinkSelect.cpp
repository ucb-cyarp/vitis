//
// Created by Christopher Yarp on 8/2/20.
//

#include "SimulinkSelect.h"
#include "General/ErrorHelpers.h"
#include "PrimitiveNodes/Constant.h"
#include "PrimitiveNodes/Sum.h"

SimulinkSelect::SimulinkSelect() : oneBased(false){

}

SimulinkSelect::SimulinkSelect(std::shared_ptr<SubSystem> parent) : MediumLevelNode(parent), oneBased(false) {

}

SimulinkSelect::SimulinkSelect(std::shared_ptr<SubSystem> parent, SimulinkSelect *orig) : MediumLevelNode(parent, orig), oneBased(orig->oneBased), modes(orig->modes), dialogIndexes(orig->dialogIndexes), fromDialog(orig->fromDialog), outputLens(orig->outputLens) {

}

bool SimulinkSelect::isOneBased() const {
    return oneBased;
}

void SimulinkSelect::setOneBased(bool oneBased) {
    SimulinkSelect::oneBased = oneBased;
}

std::shared_ptr<SimulinkSelect>
SimulinkSelect::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                  std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node & Set Common Properties ====
    std::shared_ptr<SimulinkSelect> newNode = NodeFactory::createNode<SimulinkSelect>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- OneBased, Mode, DialogIndexes, FromDialog, OutputLen
        std::string oneBasedStr = dataKeyValueMap.at("OneBased");
        newNode->oneBased = GeneralHelper::parseBool(oneBasedStr);

        std::string numOfDimsStr = dataKeyValueMap.at("NumDimensions");
        int numDims = std::stoi(numOfDimsStr);

        if(numDims < 1){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Number of dimensions should be >=1", newNode));
        }

        //Get the dimension specs
        std::vector<Select::SelectMode> selectModes;
        std::vector<bool> fromDialog;
        std::vector<int> outputLens;
        std::vector<std::vector<NumericValue>> dialogIndexes;
        for(int i = 0; i<numDims; i++) {
            std::string modeStr = dataKeyValueMap.at("Mode"+GeneralHelper::to_string(i));
            Select::SelectMode selectMode = Select::parseSelectModeStr(modeStr);
            selectModes.push_back(selectMode);

            if(selectMode == Select::SelectMode::OFFSET_LENGTH){
               std::string outputLenStr = dataKeyValueMap.at("OutputLen"+GeneralHelper::to_string(i));
               int outputLen = std::stoi(outputLenStr);
               outputLens.push_back(outputLen);
            }else if(selectMode == Select::SelectMode::INDEX_VECTOR){
                outputLens.push_back(0);
            }else{
                throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown Select Mode", newNode));
            }

            std::string fromDialogStr = dataKeyValueMap.at("FromDialog"+std::to_string(i));
            bool fromDialogBool = GeneralHelper::parseBool(fromDialogStr);
            fromDialog.push_back(fromDialogBool);
            if(fromDialogBool){
                std::string dialogIndexesStr = dataKeyValueMap.at("DialogIndexes"+std::to_string(i));
                std::vector<NumericValue> dialogIndexesVec = NumericValue::parseXMLString(dialogIndexesStr);
                dialogIndexes.push_back(dialogIndexesVec);
            }else{
                dialogIndexes.emplace_back();
            }
        }

        newNode->setModes(selectModes);
        newNode->setFromDialog(fromDialog);
        newNode->setOutputLens(outputLens);
        newNode->setDialogIndexes(dialogIndexes);
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

        std::string numOfDimsStr = dataKeyValueMap.at("Numeric.NumberOfDimensions");
        int numDims = std::stoi(numOfDimsStr);

        if(numDims < 1){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Number of dimensions should be >=1", newNode));
        }

        //Get the dimension specs
        std::vector<Select::SelectMode> selectModes;
        std::vector<bool> fromDialog;
        std::vector<int> outputLens;
        std::vector<std::vector<NumericValue>> dialogIndexes;
        for(int i = 0; i<numDims; i++) {
            std::string suffix = i == 0 ? "1st" : GeneralHelper::to_string(i); //For legacy reasons, the first index has a different format from subsequent indexes

            std::string modeStr = dataKeyValueMap.at("IndexOption"+suffix);
            if (modeStr == "Index vector (dialog)") {
                selectModes.push_back(Select::SelectMode::INDEX_VECTOR);
                fromDialog.push_back(true);
            } else if (modeStr == "Index vector (port)") {
                selectModes.push_back(Select::SelectMode::INDEX_VECTOR);
                fromDialog.push_back(false);
            } else if (modeStr == "Starting index (dialog)") {
                selectModes.push_back(Select::SelectMode::OFFSET_LENGTH);
                fromDialog.push_back(true);
            } else if (modeStr == "Starting index (port)") {
                selectModes.push_back(Select::SelectMode::OFFSET_LENGTH);
                fromDialog.push_back(false);
            } else {
                throw std::runtime_error(ErrorHelpers::genErrorStr(
                        "Indexing Mode is Currently Unsupported in SimulinkSelect: " + modeStr, newNode));
            }

            std::string outputLenStr = dataKeyValueMap.at("Numeric.OutputSize"+suffix);

            std::string dialogIndexesStr = dataKeyValueMap.at("Numeric.IndexParam"+suffix);

            outputLens.push_back(std::stoi(outputLenStr));

            dialogIndexes.push_back(NumericValue::parseXMLString(dialogIndexesStr));
            newNode->dialogIndexes = dialogIndexes;
        }

        newNode->setModes(selectModes);
        newNode->setFromDialog(fromDialog);
        newNode->setOutputLens(outputLens);
        newNode->setDialogIndexes(dialogIndexes);
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - SimulinkSelect", newNode));
    }

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

    parameters.insert(GraphMLParameter("NumDimensions", "int", true));

    for(int i = 0; i<modes.size(); i++){
        parameters.insert(GraphMLParameter("Mode"+GeneralHelper::to_string(i), "string", true));

        if(modes[i] == Select::SelectMode::OFFSET_LENGTH){
            parameters.insert(GraphMLParameter("OutputLen"+GeneralHelper::to_string(i), "int", true));
        }

        parameters.insert(GraphMLParameter("FromDialog"+std::to_string(i), "boolean", true));
        if(fromDialog[i]){
            parameters.insert(GraphMLParameter("DialogIndexes"+std::to_string(i), "string", true));
        }
    }

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

    GraphMLHelper::addDataNode(doc, thisNode, "NumDimensions", GeneralHelper::to_string(modes.size()));

    for(int i = 0; i<modes.size(); i++){
        GraphMLHelper::addDataNode(doc, thisNode, "Mode"+GeneralHelper::to_string(i), Select::selectModeToStr(modes[i]));

        if(modes[i] == Select::SelectMode::OFFSET_LENGTH){
            GraphMLHelper::addDataNode(doc, thisNode, "OutputLen"+GeneralHelper::to_string(i), GeneralHelper::to_string(outputLens[i]));
        }

        bool fromDialogLoc = fromDialog[i];
        GraphMLHelper::addDataNode(doc, thisNode, "FromDialog"+std::to_string(i), GeneralHelper::to_string(fromDialogLoc));
        if(fromDialog[i]){
            GraphMLHelper::addDataNode(doc, thisNode, "DialogIndexes"+std::to_string(i), NumericValue::toString(dialogIndexes[i]));
        }
    }

    return thisNode;
}

std::string SimulinkSelect::typeNameStr() {
    return "SimulinkSelect";
}

std::string SimulinkSelect::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() +
             "\nOneBased: " + GeneralHelper::to_string(oneBased);

    for(int i = 0; i<modes.size(); i++){
        label += "\nMode" + GeneralHelper::to_string(i) + ": " + Select::selectModeToStr(modes[i]);

        if(modes[i] == Select::SelectMode::OFFSET_LENGTH){
            label += "\nOutputLen" + GeneralHelper::to_string(i) + ": " + GeneralHelper::to_string(outputLens[i]);
        }

        bool fromDialogLoc = fromDialog[i];
        label += "\nFromDialog" + GeneralHelper::to_string(i) + ": " + GeneralHelper::to_string(fromDialogLoc);
        if(fromDialog[i]){
            label += "\nDialogIndexes" + GeneralHelper::to_string(i) + ": " + NumericValue::toString(dialogIndexes[i]);
        }
    }

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

    if(inputDT.isScalar()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Input Cannot be Scalar", getSharedPointer()));
    }

    //Check dimension of output matches the number of dimensions in the selection
    if(outputDT.getDimensions().size() != modes.size()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Number of Select Ports Must Match Number of Dimensions in the Output.  Follow up with a reshape node to contract number of dimensions", getSharedPointer()));
    }

    //Check number of input ports
    int fromDialogDims = 0;
    for(int i = 0; i<modes.size(); i++) {
        if (fromDialog[i]) {
            fromDialogDims++;
        }
    }
    if(inputPorts.size() != modes.size()-fromDialogDims+1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Number of Select Ports Must Match Number of Dimensions in the Output Which Do Not Take Their Selection from Dialog.  Follow up with a reshape node to contract number of dimensions", getSharedPointer()));
    }


    for(int i = 0; i<modes.size(); i++) {
        if (fromDialog[i]) {
            //Check that they are real integers
            for (int j = 0; j < dialogIndexes[i].size(); j++) {
                if (dialogIndexes[i][j].isFractional()) {
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "Validation Failed - SimulinkSelect - Dialog Index Must be Integer", getSharedPointer()));
                }
                if (dialogIndexes[i][j].isComplex()) {
                    throw std::runtime_error(
                            ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Dialog Index Must be Real",
                                                      getSharedPointer()));
                }
            }

            if (modes[i] == Select::SelectMode::INDEX_VECTOR) {
                if (outputDT.getDimensions()[i] != dialogIndexes.size()) {
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "Validation Failed - SimulinkSelect - Output Dimension Should Have Same Number of Elements as Index Vector",
                            getSharedPointer()));
                }
            } else if (modes[i] == Select::SelectMode::OFFSET_LENGTH) {
                if (dialogIndexes.size() != 1) {
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "Validation Failed - SimulinkSelect - Index Should be a Scalar When Indexing Mode is Offset",
                            getSharedPointer()));
                }

                if (outputLens[i] != outputDT.getDimensions()[i]) {
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "Validation Failed - SimulinkSelect - For Offset/Length Mode, Output Dimension Length Should Match OutputLen Parameter ",
                            getSharedPointer()));
                }
            } else {
                throw std::runtime_error(
                        ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Unknown Mode Select",
                                                  getSharedPointer()));
            }
        } else {
            //Check they are real integers
            DataType selectDT = getInputPort(1+i)->getDataType();

            if (selectDT.isFloatingPt()) {
                throw std::runtime_error(
                        ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Index Must be Integer",
                                                  getSharedPointer()));
            }
            if (selectDT.isComplex()) {
                throw std::runtime_error(
                        ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Index Must be Real",
                                                  getSharedPointer()));
            }

            if (modes[i] == Select::SelectMode::INDEX_VECTOR) {
                if (outputDT.getDimensions()[i] != selectDT.numberOfElements()) {
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "Validation Failed - SimulinkSelect - Output Dimension Should Have Same Number of Elements as Index Vector",
                            getSharedPointer()));
                }
            } else if (modes[i] == Select::SelectMode::OFFSET_LENGTH) {
                if (!selectDT.isScalar()) {
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "Validation Failed - SimulinkSelect - Index Should be a Scalar When Indexing Mode is Offset",
                            getSharedPointer()));
                }

                if (outputLens[i] != outputDT.getDimensions()[i]) {
                    throw std::runtime_error(ErrorHelpers::genErrorStr(
                            "Validation Failed - SimulinkSelect - For Offset/Length Mode, Output Dimension Length Should Match OutputLen Parameter ",
                            getSharedPointer()));
                }
            } else {
                throw std::runtime_error(
                        ErrorHelpers::genErrorStr("Validation Failed - SimulinkSelect - Unknown Mode Select",
                                                  getSharedPointer()));
            }
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
    selectNode->setModes(modes);
    selectNode->setOutputLens(outputLens);
    new_nodes.push_back(selectNode);

    //--Rewire Outputs--
    //Was validated to have only 1 output port
    std::set<std::shared_ptr<Arc>> outputArcs = getOutputPort(0)->getArcs();
    for(const auto & outputArc : outputArcs){
        outputArc->setSrcPortUpdateNewUpdatePrev(selectNode->getOutputPortCreateIfNot(0));
    }

    //--Rewire OrderConstraintOutput Arcs --
    std::set<std::shared_ptr<Arc>> orderConstraintOutputArcs = getOrderConstraintOutputArcs();
    for (const auto &orderConstraintOutputArc : orderConstraintOutputArcs) {
        orderConstraintOutputArc->setSrcPortUpdateNewUpdatePrev(
                selectNode->getOrderConstraintOutputPortCreateIfNot());
    }

    //--Rewire Input--
    std::set<std::shared_ptr<Arc>> inputArc = getInputPort(0)->getArcs();
    //Validated to only have 1 input
    (*inputArc.begin())->setDstPortUpdateNewUpdatePrev(selectNode->getInputPortCreateIfNot(0));

    //--Handle Indexes (either from Dialogs or Ports)--
    for(int dim = 0; dim < modes.size(); dim++) {
        std::shared_ptr<OutputPort> indexSrcPort;
        DataType indexDT;
        if (fromDialog[dim]) {
            //Create Constant
            std::shared_ptr<Constant> indConst = NodeFactory::createNode<Constant>(expandedNode);
            indConst->setName("IndConst_Dim"+GeneralHelper::to_string(dim));
            indConst->setValue(dialogIndexes[dim]);
            new_nodes.push_back(indConst);

            indexSrcPort = indConst->getOutputPortCreateIfNot(0);

            //Was validated that index was real and integer
            int numBits = 0;
            bool isSigned = false;
            for (int i = 0; i < dialogIndexes[dim].size(); i++) {
                isSigned |= dialogIndexes[dim][i].isSigned();
            }

            for (int i = 0; i < dialogIndexes[dim].size(); i++) {
                int maxBitsRequired = dialogIndexes[dim][i].numIntegerBits();
                if (!dialogIndexes[dim][i].isSigned() && isSigned) {
                    maxBitsRequired++;
                }
                if (maxBitsRequired > numBits) {
                    numBits = maxBitsRequired;
                }
            }

            int constElements = dialogIndexes.size();
            indexDT = DataType(false, isSigned, false, numBits, 0, {constElements});
            indexDT = indexDT.getCPUStorageType(); //Do not want extra masking operation
        } else {
            //Get src and datatype
            std::set<std::shared_ptr<Arc>> indArc = getInputPort(1+dim)->getArcs();
            //Was validated to have 1 input arc during validation
            indexSrcPort = (*indArc.begin())->getSrcPort();
            indexDT = (*indArc.begin())->getDataType();

            //Delete arc
            (*indArc.begin())->disconnect();
            deleted_arcs.push_back(*indArc.begin());
        }

        //--Wire Index Port of Select & Move Order Constrain Input--
        if (oneBased) {
            //Create Constant and Sum Nodes
            std::shared_ptr<Constant> decrConst = NodeFactory::createNode<Constant>(expandedNode);
            decrConst->setName("DecrConst_Dim" + GeneralHelper::to_string(dim));
            decrConst->setValue({NumericValue((long) 1)});
            new_nodes.push_back(decrConst);
            DataType decrConstDT = indexDT;
            decrConstDT.setDimensions({1});

            std::shared_ptr<Sum> decrSum = NodeFactory::createNode<Sum>(expandedNode);
            decrSum->setName("DecrSum_Dim" + GeneralHelper::to_string(dim));
            decrSum->setInputSign({true, false});  //Second port is minus
            new_nodes.push_back(decrSum);

            //Wire Index Arc
            std::shared_ptr<Arc> indArcIn = Arc::connectNodes(indexSrcPort, decrSum->getInputPortCreateIfNot(0),
                                                              indexDT);
            new_arcs.push_back(indArcIn);

            //Wire decr and decr to select
            std::shared_ptr<Arc> constDecArc = Arc::connectNodes(decrConst->getOutputPortCreateIfNot(0),
                                                                 decrSum->getInputPortCreateIfNot(1), decrConstDT);
            new_arcs.push_back(constDecArc);
            std::shared_ptr<Arc> indArc = Arc::connectNodes(decrSum->getOutputPortCreateIfNot(0),
                                                            selectNode->getInputPortCreateIfNot(1+dim), indexDT);
            new_arcs.push_back(indArc);

            //Rewire Order Constraint Input Arcs to Sum
            std::set<std::shared_ptr<Arc>> orderConstraintInputArcs = getOrderConstraintInputArcs();
            for (const auto &orderConstraintInputArc : orderConstraintInputArcs) {
                orderConstraintInputArc->setDstPortUpdateNewUpdatePrev(
                        decrSum->getOrderConstraintInputPortCreateIfNot());
            }
        } else {
            //Rewire Index Arc
            std::shared_ptr<Arc> indArc = Arc::connectNodes(indexSrcPort, selectNode->getInputPortCreateIfNot(1+dim),
                                                            indexDT);
            new_arcs.push_back(indArc);

            //Rewire Order Constraint Input Arcs
            std::set<std::shared_ptr<Arc>> orderConstraintInputArcs = getOrderConstraintInputArcs();
            for (const auto &orderConstraintInputArc : orderConstraintInputArcs) {
                orderConstraintInputArc->setDstPortUpdateNewUpdatePrev(
                        selectNode->getOrderConstraintInputPortCreateIfNot());
            }
        }
    }

    return expandedNode;
}

std::vector<Select::SelectMode> SimulinkSelect::getModes() const {
    return modes;
}

void SimulinkSelect::setModes(const std::vector<Select::SelectMode> &modes) {
    SimulinkSelect::modes = modes;
}

std::vector<std::vector<NumericValue>> SimulinkSelect::getDialogIndexes() const {
    return dialogIndexes;
}

void SimulinkSelect::setDialogIndexes(const std::vector<std::vector<NumericValue>> &dialogIndexes) {
    SimulinkSelect::dialogIndexes = dialogIndexes;
}

std::vector<int> SimulinkSelect::getOutputLens() const {
    return outputLens;
}

void SimulinkSelect::setOutputLens(const std::vector<int> &outputLens) {
    SimulinkSelect::outputLens = outputLens;
}

std::vector<bool> SimulinkSelect::getFromDialog() const {
    return fromDialog;
}

void SimulinkSelect::setFromDialog(const std::vector<bool> &fromDialog) {
    SimulinkSelect::fromDialog = fromDialog;
}
