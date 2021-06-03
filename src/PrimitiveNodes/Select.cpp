//
// Created by Christopher Yarp on 8/2/20.
//

#include "Select.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"

Select::Select() {

}

Select::Select(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent) {

}

Select::Select(std::shared_ptr<SubSystem> parent, Select *orig) : PrimitiveNode(parent, orig), modes(orig->modes), outputLens(orig->outputLens){

}

std::shared_ptr<Select>
Select::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                          std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<Select> newNode = NodeFactory::createNode<Select>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important property ====
    std::vector<SelectMode> selectModes;
    std::vector<int> outputLens;

    //This is a vitis only block.  When importing from simulink it uses SimulinkSelect
    if(dialect == GraphMLDialect::VITIS){
        std::string numDimensionsStr = dataKeyValueMap.at("NumDimensions");
        int numDimensions = std::stoi(numDimensionsStr);

        for(int i = 0; i<numDimensions; i++){
            std::string selectModeStr = dataKeyValueMap.at("Mode" + GeneralHelper::to_string(i));
            SelectMode selectMode = parseSelectModeStr(selectModeStr);
            selectModes.push_back(selectMode);
            if(selectMode == SelectMode::OFFSET_LENGTH){
                std::string offsetLen = dataKeyValueMap.at("OutputLen" + GeneralHelper::to_string(i));
                outputLens.push_back(std::stoi(offsetLen));
            }else{
                outputLens.push_back(0);
            }
        }
    } else if(dialect == GraphMLDialect::SIMULINK_EXPORT) {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Select", newNode));
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Select", newNode));
    }

    newNode->modes = selectModes;
    newNode->outputLens = outputLens;

    return newNode;
}

Select::SelectMode Select::parseSelectModeStr(std::string str) {
    if(str == "OffsetLen"){
        return SelectMode::OFFSET_LENGTH;
    }else if(str == "IndexVector"){
        return SelectMode::INDEX_VECTOR;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown SelectMode"));
    }
}

std::string Select::selectModeToStr(Select::SelectMode mode) {
    if(mode == SelectMode::OFFSET_LENGTH){
        return "OffsetLen";
    }else if(mode == SelectMode::INDEX_VECTOR){
        return "IndexVector";
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown SelectMode"));
    }
}

std::set<GraphMLParameter> Select::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    for(int i = 0; i<modes.size(); i++) {
        parameters.insert(GraphMLParameter("Mode" + GeneralHelper::to_string(i), "string", true));
        if(modes[i] == SelectMode::OFFSET_LENGTH) {
            parameters.insert(GraphMLParameter("OutputLen" + GeneralHelper::to_string(i), "int", true));
        }
    }

    return parameters;
}

xercesc::DOMElement *
Select::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Select");

    for(int i = 0; i<modes.size(); i++) {
        GraphMLHelper::addDataNode(doc, thisNode, "Mode" + GeneralHelper::to_string(i), selectModeToStr(modes[i]));
        if(modes[i] == SelectMode::OFFSET_LENGTH) {
            GraphMLHelper::addDataNode(doc, thisNode, "OutputLen" + GeneralHelper::to_string(i), GeneralHelper::to_string(outputLens[i]));
        }
    }

    return thisNode;
}

std::string Select::typeNameStr() {
    return "Select";
}

std::string Select::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr();

    for(int i = 0; i<modes.size(); i++) {
        label +=  "\nMode" + GeneralHelper::to_string(i) + ":" + selectModeToStr(modes[i]);
        if(modes[i] == SelectMode::OFFSET_LENGTH) {
            label += "\nOutputLen" + GeneralHelper::to_string(i) + ":" + GeneralHelper::to_string(outputLens[i]);
        }
    }

    return label;
}

std::shared_ptr<Node> Select::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Select>(parent, this);
}

void Select::validate() {
    Node::validate();

    if(inputPorts.size() != 1 + modes.size()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Should Have 1 + Number of Select Dimensions", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    DataType outputDT = getOutputPort(0)->getDataType();
    DataType outputDTScalar = outputDT;
    outputDTScalar.setDimensions({1});

    DataType inputDT = getInputPort(0)->getDataType();
    DataType inputDTScalar = outputDT;
    inputDTScalar.setDimensions({1});

    if(inputDT.isScalar()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Input Cannot be Scalar", getSharedPointer()));
    }

    if(outputDTScalar != inputDTScalar){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Input and Output Should Have the Same Type", getSharedPointer()));
    }

    //Check dimension of input and output matches the number of dimensions in the selection
    if(inputDT.getDimensions().size() != modes.size()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Number of Select Ports Must Match Number of Dimensions in the Input", getSharedPointer()));
    }

    if(outputDT.getDimensions().size() != modes.size()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Number of Select Ports Must Match Number of Dimensions in the Output.  Follow up with a reshape node to contract number of dimensions", getSharedPointer()));
    }

    for(int i = 0; i<modes.size(); i++){
        DataType indexDT = getInputPort(1+i)->getDataType();

        if (indexDT.isFloatingPt()) {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Index Port Should be an Integer (or Vector of Integers)", getSharedPointer()));
        }

        if (indexDT.isComplex()) {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Index Port Should be Real",getSharedPointer()));
        }

        if (modes[i] == SelectMode::OFFSET_LENGTH) {
            if (!indexDT.isScalar()) {
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - The Index Input Should be a Scalar When Operating in OffsetLength Mode", getSharedPointer()));
            }

            if (outputDT.getDimensions()[i] != outputLens[i]) {
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Output vector length should match outputLen parameter when operating in OffsetLength mode", getSharedPointer()));
            }
        } else if (modes[i] == SelectMode::INDEX_VECTOR) {
            if (!indexDT.isScalar() && !indexDT.isVector()) {
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - The Index Input Should be a Scalar or Vector When Operating in IndexVector Mode", getSharedPointer()));
            }

            if (outputDT.getDimensions()[i] != indexDT.numberOfElements()) {
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Dimensions of Output Vector and Index Vector Should Match When Operating in IndexVector Mode", getSharedPointer()));
            }
        }
    }

    if(outputDT.numberOfElements() > inputDT.numberOfElements()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Select - Dimensions of Output Vector/Matrix Must be Smaller or Equal to Dimensions of Input Vector/Matrix", getSharedPointer()));
    }
}

CExpr Select::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                        bool imag) {
    //==== Get the expressions for each input ====
    //Input
    std::shared_ptr<OutputPort> inputSrcOutputPort = getInputPort(0)->getSrcOutputPort();
    int inputSrcOutputPortNum = inputSrcOutputPort->getPortNum();
    std::shared_ptr<Node> inputSrcNode = inputSrcOutputPort->getParent();
    CExpr inputExpr = inputSrcNode->emitC(cStatementQueue, schedType, inputSrcOutputPortNum, imag);

    //Index(s)
    std::vector<CExpr> indExprs;
    for(int i = 0; i<modes.size(); i++) {
        std::shared_ptr<OutputPort> indSrcOutputPort = getInputPort(1)->getSrcOutputPort();
        int indSrcOutputPortNum = indSrcOutputPort->getPortNum();
        std::shared_ptr<Node> indSrcNode = indSrcOutputPort->getParent();
        indExprs.push_back(indSrcNode->emitC(cStatementQueue, schedType, indSrcOutputPortNum, false));
    }

    //==== Emit Output Var ====
    std::string outputVarName = name + "_n" + GeneralHelper::to_string(id) + "_out";
    DataType outputDT = getOutputPort(0)->getDataType();

    Variable outputVar = Variable(outputVarName, outputDT);
    //Emit variable declaration
    cStatementQueue.push_back(outputVar.getCVarDecl(imag, true, false, true, false) + ";");

    //==== Emit Select ====
    //For loop indexes and dimensions will be based on the size of the output which has been validated

    //Create nested loops for a given array
    std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
            EmitterHelpers::generateVectorMatrixForLoops(outputDT.getDimensions());

    std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
    std::vector<std::string> forLoopIndexVars = std::get<1>(forLoopStrs);
    std::vector<std::string> forLoopClose = std::get<2>(forLoopStrs);

    std::vector<std::string> expandedIndexExprs;

    for(int i = 0; i<modes.size(); i++) {
        if (modes[i] == SelectMode::INDEX_VECTOR) {
            DataType indexDT = getInputPort(1+i)->getDataType();

            //The index into the input comes from the input vector.  If it is indeed a vector and not a scalar, we need to index into
            //the index vector
            std::vector<std::string> dimensionOuterIndex = {outputDT.getDimensions()[i] > 1 ? forLoopIndexVars[i] : "0"};

            std::string indTermDeref;
            if(indExprs[i].isArrayOrBuffer()) {
                indTermDeref = indExprs[i].getExprIndexed(dimensionOuterIndex, true);
            }else{
                if(outputDT.getDimensions()[i] > 1){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Index Vector for Given Dimension is Expected to have Multiple Entries but is a Scalar", getSharedPointer()));
                }

                indTermDeref = indExprs[i].getExpr();
            }

            //Note: this currently relies on the input being a vector and not a scalar (was validated above)
            expandedIndexExprs.push_back(indTermDeref);
        } else if (modes[i] == SelectMode::OFFSET_LENGTH) {
            //The index is the indexExpr + the offset from the for loop
            if (indExprs[i].isArrayOrBuffer()) {
                throw std::runtime_error(ErrorHelpers::genErrorStr(
                        "Selector in OFFSET_LENGTH mode expects a scalar expression for the index",
                        getSharedPointer()));
            }

            std::string indTerm = indExprs[i].getExpr() + (outputDT.getDimensions()[i] > 1 ? " + " + forLoopIndexVars[i] : "");
            expandedIndexExprs.push_back(indTerm);
        } else {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown Select Mode", getSharedPointer()));
        }

        //Emit for loop
        //Do not emit loops for a given dimension when it only has 1 element
        for(int i = 0; i<modes.size(); i++) {
            if(outputDT.getDimensions()[i] > 1){
                cStatementQueue.push_back(forLoopOpen[i]);
            }
        }

        //Copy Logic
        std::string srcTerm = inputExpr.getExprIndexed(expandedIndexExprs, true); //Deref with the index terms
        std::string dstTerm = outputVar.getCVarName(imag) + (outputDT.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars)); //Deref (if not a scalar) with just the indexes from the for loops
        cStatementQueue.push_back(dstTerm + " = " + srcTerm + ";");

        //Close
        for(int i = 0; i<modes.size(); i++) {
            if (outputDT.getDimensions()[i] > 1) {
                cStatementQueue.push_back(forLoopClose[i]);
            }
        }
    }

    return CExpr(outputVar.getCVarName(imag), outputDT.isScalar() ? CExpr::ExprType::SCALAR_VAR : CExpr::ExprType::ARRAY);
}

std::vector<Select::SelectMode> Select::getModes() const{
    return modes;
}

void Select::setModes(const std::vector<Select::SelectMode> &modes) {
    Select::modes = modes;
}

std::vector<int> Select::getOutputLens() const{
    return outputLens;
}

void Select::setOutputLens(const std::vector<int> &outputLens) {
    Select::outputLens = outputLens;
}

