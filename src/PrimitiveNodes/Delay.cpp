//
// Created by Christopher Yarp on 7/6/18.
//

#include <vector>
#include <iostream>

#include "Delay.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/NumericValue.h"
#include "General/GraphAlgs.h"

#include "General/GeneralHelper.h"
#include "General/ErrorHelpers.h"
#include "General/EmitterHelpers.h"

Delay::Delay() : delayValue(0), earliestFirst(false), allocateExtraSpace(false){

}

Delay::Delay(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), delayValue(0), earliestFirst(false), allocateExtraSpace(false) {

}

int Delay::getDelayValue() const {
    return delayValue;
}

void Delay::setDelayValue(int delayValue) {
    Delay::delayValue = delayValue;
}

std::vector<NumericValue> Delay::getInitCondition() const {
    return initCondition;
}

void Delay::setInitCondition(const std::vector<NumericValue> &initCondition) {
    Delay::initCondition = initCondition;
}

void Delay::populatePropertiesFromGraphML(std::shared_ptr<Delay> node, std::string simulinkDelayLenName, std::string simulinkInitCondName, int id, std::string name,
                                          std::map<std::string, std::string> dataKeyValueMap,
                                          std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    node->setId(id);
    node->setName(name);

    //==== Import important properties ====
    std::string delayStr;
    std::string initialConditionStr;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- DelayLength, InitialCondit
        delayStr = dataKeyValueMap.at("DelayLength");
        initialConditionStr = dataKeyValueMap.at("InitialCondition");
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Names -- Numeric.DelayLength, Numeric.InitialCondition
        delayStr = dataKeyValueMap.at(simulinkDelayLenName);
        initialConditionStr = dataKeyValueMap.at(simulinkInitCondName);

        //NOTE: when importing Simulink, the last dimension of the initial condition needs to be the length of the delay.
        // For us, it is the first dimension.  This is because of the row/column major disagreement between matlab and C/C++
        //A transpose is required to correct for this.  For now, this is unsupported

        //When the input is a vector, either a matrix with a number of columns equal to the delay length (and rows equal to the incoming vector)
        //need to be provided, or a single scalar value can be supplied.  There does not appear to be an intermediate where a vector the length of
        //the delay length is supplied and is broadcast to all the elements in that vector for that delay stage.
        //TODO: Implement multidimensional init conditions.  Will need to provide the transpose or reshape operation when importing from matlab
        //TODO: Reshaping with the multidimensional array flattened to a vector is possible (it's stored in contiguous memory after call) but we need to know the dimensions
        //TODO: Override propagateProperties so that the dimensions of the input can be determined.  Possibly create a flag to note that a transpose or re-shaping operation is required because the values are from simulink

        //Note, this is only for Simulink Import, multidimensional init values are supported internally so that delay merging can be performed.
        //In order to support delay
        if(initialConditionStr.find(';') != std::string::npos && initialConditionStr.find(',') != std::string::npos){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Currently, 2D initial conditions are not supported for Delay.  Only scalars or single vectors.  A single vector is allowed if the delay length is 1 and the input is a vector.  A single vector is also allowed if the input is scalar and the delay length is >1", node));
        }
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Delay", node));
    }

    int delayVal = std::stoi(delayStr);
    node->setDelayValue(delayVal);

    std::vector<NumericValue> initialConds = NumericValue::parseXMLString(initialConditionStr);

    //Note, handling broadcast of scalar init values is now handled in propagateProperties so that the dimensions of the
    //intput/output are known.  By storing initial conditions in a vector and placing any dimension expansion to the left
    //of the indexing list, the initial conditions of merged delay blocks can be simply concatenated.  This is because
    //C/C++ indexing through contiguous memory increments the left most index the least frequently.

    //Note: simuluink allows specifying delays of 0 length with an initial condition supplied.
    //This messes up logic in vitis like FIFO delay absorption
    //To handle this, the initial condition will be set to an empty array if the delay value is 0
    //and a warning will be emitted

    if(delayVal == 0 && initialConds.size() != 0){
        std::cerr << ErrorHelpers::genWarningStr("Removing initial condition " + GeneralHelper::vectorToString(initialConds) + " from delay of 0 length", node) << std::endl;
        initialConds = std::vector<NumericValue>();
    }

    node->setInitCondition(initialConds);

    //The following are parameters used for TappedDelay, which is an extension of Delay.  They are set to default values for standard delays and are not saved/imported for standard delays.
    //These defaults are overwritten in TappedDelay import and are also exported by TappedDelay.
    node->earliestFirst = false;
    node->allocateExtraSpace = false;
}

std::shared_ptr<Delay> Delay::createFromGraphML(int id, std::string name,
                                                std::map<std::string, std::string> dataKeyValueMap,
                                                std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<Delay> newNode = NodeFactory::createNode<Delay>(parent);

    populatePropertiesFromGraphML(newNode, "Numeric.DelayLength", "Numeric.InitialCondition", id, name, dataKeyValueMap, parent, dialect);

    if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //==== Check Supported Config (Only if Simulink Import)====
        std::string delayLengthSource = dataKeyValueMap.at("DelayLengthSource");

        if (delayLengthSource != "Dialog") {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Delay block must specify Delay Source as \"Dialog\"", newNode));
        }

        std::string initialConditionSource = dataKeyValueMap.at("InitialConditionSource");
        if (initialConditionSource != "Dialog") {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Delay block must specify Initial Condition Source source as \"Dialog\"", newNode));
        }
    }

    return newNode;
}

void Delay::propagateProperties(){
    //Check if broadcasting scalar initial condition is required
    if(initCondition.size() == 1){
        //May need to be expanded/broadcast
        int elements = getInputPort(0)->getDataType().numberOfElements() * delayValue;

        NumericValue uniformVal = initCondition[0];

        //Already have one entry, add the rest
        for(unsigned long i = 1; i<elements; i++){
            initCondition.push_back(uniformVal);
        }
    }

    if(allocateExtraSpace && delayValue > 0){ //allocateExtraSpace only has an effect if the delay value > 0
        //Need to add initial conditions for the extra elements
        //It does not really matter as this extra element is only used by TappedDelay to pass the current value
        int elements = getInputPort(0)->getDataType().numberOfElements();

        std::vector<NumericValue> extraPosInit;
        for(int i = 0; i<elements; i++){
            extraPosInit.push_back(initCondition[i]);
        }

        if(earliestFirst){
            //Insert initial conditions at the front (this is where the current value would go)
            initCondition.insert(initCondition.begin(), extraPosInit.begin(), extraPosInit.end());
        }else{
            //Insert initial conditions at the end (this is where the current value would go)
            initCondition.insert(initCondition.end(), extraPosInit.begin(), extraPosInit.end());
        }
    }
}

std::set<GraphMLParameter> Delay::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("DelayLength", "int", true));
    parameters.insert(GraphMLParameter("InitialCondition", "string", true));

    return parameters;
}

void Delay::emitGraphMLDelayParams(xercesc::DOMDocument *doc, xercesc::DOMElement *xmlNode) {
    GraphMLHelper::addDataNode(doc, xmlNode, "DelayLength", GeneralHelper::to_string(delayValue));

    std::vector<NumericValue> initConds;
    if(allocateExtraSpace && delayValue > 0){ //allocateExtraSpace only has an effect if the delay value > 0
        //Remove the extra values when emitting

        if(earliestFirst){
            //Remove from the beginning
            int elements = getInputPort(0)->getDataType().numberOfElements();
            initConds.insert(initConds.begin(), initCondition.begin()+elements, initCondition.end());
        }else{
            //remove from the end
            int initCondLen = initCondition.size();
            initConds.insert(initConds.begin(), initCondition.begin(), initCondition.begin()+initCondLen);
        }
    }else{
        initConds = initCondition;
    }

    GraphMLHelper::addDataNode(doc, xmlNode, "InitialCondition", NumericValue::toString(initConds));
}

xercesc::DOMElement *
Delay::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Delay");

    emitGraphMLDelayParams(doc, thisNode);

    return thisNode;
}

std::string Delay::typeNameStr(){
    return "Delay";
}

std::string Delay::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() +
             "\nDelayLength: " + GeneralHelper::to_string(delayValue);

    std::vector<NumericValue> initConds;
    if(allocateExtraSpace && delayValue > 0){ //allocateExtraSpace only has an effect if the delay value > 0
        //Remove the extra values when emitting

        if(earliestFirst){
            //Remove from the beginning
            int elements = getInputPort(0)->getDataType().numberOfElements();
            initConds.insert(initConds.begin(), initCondition.begin()+elements, initCondition.end());
        }else{
            //remove from the end
            int initCondLen = initCondition.size();
            initConds.insert(initConds.begin(), initCondition.begin(), initCondition.begin()+initCondLen);
        }
    }else{
        initConds = initCondition;
    }

    label += "\nInitialCondition: " + NumericValue::toString(initConds);

    return label;
}

void Delay::validate() {
    Node::validate();

    //Should have 1 input ports and 1 output port
    if(inputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Delay - Should Have Exactly 1 Input Port", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Delay - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    //Check that input port and the output port have the same type
    DataType outType = getOutputPort(0)->getDataType();
    DataType inType = getInputPort(0)->getDataType();

    if(inType != outType){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Delay - DataType of Input Port Does not Match Output Port", getSharedPointer()));
    }

    int arraySize = delayValue;
    if(allocateExtraSpace){
        arraySize++;
    }
    if (delayValue != 0 && arraySize*inType.numberOfElements() != initCondition.size()){
        throw std::runtime_error(ErrorHelpers::genErrorStr(
                "Validation Failed - Delay - Delay Length (" + GeneralHelper::to_string(delayValue) +
                ") * Element Dimensions (" + GeneralHelper::to_string(inType.numberOfElements()) +
                ") does not Match the Length of Init Condition Vector (" +
                GeneralHelper::to_string(initCondition.size()) + ")", getSharedPointer()));
    }
}

bool Delay::hasState() {
    if(delayValue > 0) {
        return true;
    }else{
        return false;
    }
}

std::vector<Variable> Delay::getCStateVars() {
    std::vector<Variable> vars;

    //If delayValue == 0 return empty vector
    if(delayValue == 0){
        return vars;
    }else{
        int arrayLen = delayValue;
        if(allocateExtraSpace){
            arrayLen++;
        }

        //There is a single state variable for the delay.  However for a delay > 1, it is a vector if the input datatype is scalar and a matrix if the input datatype is a vector or matrix
        //Note that if the delay is 1, no expansion is required
        DataType stateType;
        if(arrayLen == 1) { //Note, allocateExtra space will neve have an array length of 1 because it has no effect if the delay length is 0 (checked above)
            stateType = getInputPort(0)->getDataType();
        }else{
            stateType = getInputPort(0)->getDataType().expandForBlock(arrayLen);
        }

        std::string varName = name+"_n"+GeneralHelper::to_string(id)+"_state";
        Variable var = Variable(varName, stateType, initCondition);
        cStateVar = var;
        //Complex variable will be made if needed by the design code based on the data type
        vars.push_back(var);

        return vars;
    }
}

CExpr Delay::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    if(delayValue == 0){
        //NOTE: allocateExtraSpace has no effect with a delayValue == 0
        //If delay = 0, simply pass through
        std::shared_ptr<OutputPort> srcPort = getInputPort(0)->getSrcOutputPort();
        int srcOutPortNum = srcPort->getPortNum();
        std::shared_ptr<Node> srcNode = srcPort->getParent();
        DataType datatype = srcPort->getDataType();

        //Emit the upstream
        CExpr inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, imag);

        if(!datatype.isScalar()) {
            //TODO: Change check after vector support implemented
            //If a vector/matrix, create a temporary and store in that.  Cannot rely on emitter to create a variable for the intermediate result
            //Emit temporary before for loop
            std::string vecOutName = name+"_n"+GeneralHelper::to_string(id)+ "_outVec"; //Changed to
            Variable vecOutVar = Variable(vecOutName, datatype);
            cStatementQueue.push_back(vecOutVar.getCVarDecl(imag, true, false, true, false) + ";");

            //Create For Loops
            std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                    EmitterHelpers::generateVectorMatrixForLoops(datatype.getDimensions());
            std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
            std::vector<std::string> forLoopIndexVars = std::get<1>(forLoopStrs);
            std::vector<std::string> forLoopClose = std::get<2>(forLoopStrs);
            cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());

            //Deref
            std::string inputExprDeref = inputExpr.getExprIndexed(forLoopIndexVars, true);

            //Emit expr
            cStatementQueue.push_back(vecOutVar.getCVarName(imag) + EmitterHelpers::generateIndexOperation(forLoopIndexVars) + " = " + inputExprDeref + ";");

            //Close for loops
            cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());

            //Return temp var
            return CExpr(vecOutVar.getCVarName(imag), CExpr::ExprType::ARRAY);
        }else{
            //Create a temporary variable to avoid issue if this node is directly attached to state
            //at the input.  The state update is placed after this node but the variable from the delay is simply
            //passed through.  This could cause the state to be update before the result is used.
            //TODO: Remove Temporary when StateUpdate insertion logic improved to track passthroughs
            //Accomplished by returning a SCALAR_EXPR instead of a SCALAR_VAR

            return CExpr(inputExpr.getExpr(), CExpr::ExprType::SCALAR_EXPR);
        }
    }else {
        //Return the state var name as the expression
        int arrayLen = delayValue;
        if(allocateExtraSpace){
            arrayLen++;
        }
        if (arrayLen == 1) {
            //Return the simple name (no index needed as it is not an array)
            //Since allocateExtraSpace has no effect with delayVal == 0, it cannot be true and have an arrayLen of 1.  Therefore, no special action is taken
            return CExpr(cStateVar.getCVarName(imag), getOutputPort(outputPortNum)->getDataType().isScalar() ? CExpr::ExprType::SCALAR_VAR : CExpr::ExprType::ARRAY); //This is a variable name therefore inform the cEmit function
        }else{
            //Since datatype expansion adds another dimension to the front of the list (elements are stored contiguously in memory(, dereferencing the first index works even if the type is a vector or array
            if(earliestFirst){
                //If the samples come in at the beginning of the array, return the end of the array
                //The extra space is at the front of the array, therefore, need to offset the returned index
                return CExpr(cStateVar.getCVarName(imag) + "[" + GeneralHelper::to_string(arrayLen-1) + "]", getOutputPort(outputPortNum)->getDataType().isScalar() ? CExpr::ExprType::SCALAR_VAR : CExpr::ExprType::ARRAY);
            }else {
                //If later samples are at the beginning of the array, return the front of the array
                //The extra space from allocateExtraSpace is at the end of the array and is not a factor in the return
                return CExpr(cStateVar.getCVarName(imag) + "[0]", getOutputPort(outputPortNum)->getDataType().isScalar() ? CExpr::ExprType::SCALAR_VAR : CExpr::ExprType::ARRAY);
            }
        }
    }
}

void Delay::emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, std::shared_ptr<StateUpdate> stateUpdateSrc) {
    //TODO: Implement Vector Support (Need 2D state)

    DataType inputDT = getInputPort(0)->getDataType(); //This is the datatype of the input and does not include any extra dimension for the buffer

    if(delayValue == 0){
        return; //No state to update
    }else if(delayValue == 1 && !allocateExtraSpace){
        //The state variable is not an array

        //Emit for loop if the input is a vector/matrix

        std::vector<std::string> forLoopIndexVars;
        std::vector<std::string> forLoopClose;
        //Will check if complex below
        std::string cStateInputVarDeref_Re = cStateInputVar.getCVarName(false); //Will be deref-ed if the input type is a vector/matrix
        std::string cStateVarDeref_Re = cStateVar.getCVarName(false); //Will be deref-ed if the input type is a vector/matrix

        std::string cStateInputVarDeref_Im;
        std::string cStateVarDeref_Im;
        if(cStateVar.getDataType().isComplex()){
            cStateInputVarDeref_Im = cStateInputVar.getCVarName(true); //Will be deref-ed if the input type is a vector/matrix
            cStateVarDeref_Im = cStateVar.getCVarName(true); //Will be deref-ed if the input type is a vector/matrix
        }

        if(!inputDT.isScalar()){
            //Create nested loops for a given array
            std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                    EmitterHelpers::generateVectorMatrixForLoops(inputDT.getDimensions());

            std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
            forLoopIndexVars = std::get<1>(forLoopStrs);
            forLoopClose = std::get<2>(forLoopStrs);

            cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());

            //Will check if complex below
            cStateVarDeref_Re += EmitterHelpers::generateIndexOperation(forLoopIndexVars);
            cStateInputVarDeref_Re += EmitterHelpers::generateIndexOperation(forLoopIndexVars);
            if(cStateVar.getDataType().isComplex()) {
                cStateVarDeref_Im += EmitterHelpers::generateIndexOperation(forLoopIndexVars);
                cStateInputVarDeref_Im += EmitterHelpers::generateIndexOperation(forLoopIndexVars);
            }
        }

        //Emit assignment
        cStatementQueue.push_back(cStateVarDeref_Re + " = " + cStateInputVarDeref_Re + ";");
        if(cStateVar.getDataType().isComplex()){
            cStatementQueue.push_back(cStateVarDeref_Im + " = " + cStateInputVarDeref_Im + ";");
        }

        //close for loop
        if(!inputDT.isScalar()) {
            cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
        }

    }else{
        //This is a (pair) of arrays
        //Emit a for loop to perform the shift for each

        //----Shift State ----
        std::string loopVarNameUnsanitized = name+"_n"+GeneralHelper::to_string(id)+"_loopCounter";
        Variable loopVar(loopVarNameUnsanitized, DataType());
        std::string loopVarNameSanitized = loopVar.getCVarName(false);

        //Shift state loop
        std::string assignToInShiftLoop_Re = cStateVar.getCVarName(false) + "[" + loopVarNameSanitized + "]";
        std::string assignToInShiftLoop_Im;
        if(cStateVar.getDataType().isComplex()) {
            assignToInShiftLoop_Im = cStateVar.getCVarName(true) + "[" + loopVarNameSanitized + "]";
        }

        int arrayLength = delayValue;
        if(allocateExtraSpace){
            arrayLength++;
        }

        std::string assignFromInShiftLoop_Re;
        std::string assignFromInShiftLoop_Im;
        if(earliestFirst){
            //If allocateExtraSpace, the extra space is at the front of the array.  The shift indexes now need to be increased by 1
            int lowInd = allocateExtraSpace ? 1 : 0;
            cStatementQueue.push_back("for(unsigned long " + loopVarNameSanitized + " = " + GeneralHelper::to_string(arrayLength - 1) + "; " + loopVarNameSanitized + " > " + GeneralHelper::to_string(lowInd) + "; " + loopVarNameSanitized + "--){");
            assignFromInShiftLoop_Re = cStateVar.getCVarName(false) + "[" + loopVarNameSanitized + "-1]";
            if(cStateVar.getDataType().isComplex()) {
                assignFromInShiftLoop_Im = cStateVar.getCVarName(true) + "[" + loopVarNameSanitized + "-1]";
            }
        }else {
            //If allocateExtraSpace, the extra space is at the end of the array, shifting does not need to extend to that ind so no additional
            cStatementQueue.push_back("for(unsigned long " + loopVarNameSanitized + " = 0; " + loopVarNameSanitized + " < " + GeneralHelper::to_string(delayValue - 1) + "; " + loopVarNameSanitized + "++){");
            assignFromInShiftLoop_Re = cStateVar.getCVarName(false) + "[" + loopVarNameSanitized + "+1]";
            if(cStateVar.getDataType().isComplex()) {
                assignFromInShiftLoop_Im = cStateVar.getCVarName(true) + "[" + loopVarNameSanitized + "+1]";
            }
        }

        //If the input is not a scalar, insert additional nested for loops
        std::vector<std::string> forLoopShiftClose;
        if(!inputDT.isScalar()){
            //Nest for loop with the dimension of the input (without any extra dimension added for the state variable)
            //Note that the input may be a scalar and, in that case, an extra dimension is not added and the state variable will be a vector
            //Note, additional dereferencing should occur after the delay index since the extra dimension for the buffer is prepended

            std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                    EmitterHelpers::generateVectorMatrixForLoops(inputDT.getDimensions());
            std::vector<std::string> forLoopShiftOpen = std::get<0>(forLoopStrs);
            std::vector<std::string> forLoopShiftIndexVars = std::get<1>(forLoopStrs);
            forLoopShiftClose = std::get<2>(forLoopStrs);
            cStatementQueue.insert(cStatementQueue.end(), forLoopShiftOpen.begin(), forLoopShiftOpen.end());

            assignToInShiftLoop_Re += EmitterHelpers::generateIndexOperation(forLoopShiftIndexVars);
            assignFromInShiftLoop_Re += EmitterHelpers::generateIndexOperation(forLoopShiftIndexVars);
            if(cStateVar.getDataType().isComplex()) {
                assignToInShiftLoop_Im += EmitterHelpers::generateIndexOperation(forLoopShiftIndexVars);
                assignFromInShiftLoop_Im += EmitterHelpers::generateIndexOperation(forLoopShiftIndexVars);
            }
        }

        //Assign shift
        cStatementQueue.push_back(assignToInShiftLoop_Re + " = " + assignFromInShiftLoop_Re + ";");
        if(cStateVar.getDataType().isComplex()){
            cStatementQueue.push_back(assignToInShiftLoop_Im + " = " + assignFromInShiftLoop_Im + ";");
        }

        //Close inner for loops
        if(!inputDT.isScalar()){
            cStatementQueue.insert(cStatementQueue.end(), forLoopShiftClose.begin(), forLoopShiftClose.end());
        }

        //Close shift loop
        cStatementQueue.push_back("}");

        //----Input store----
        std::string assignToInInputLoop_Re;
        std::string assignToInInputLoop_Im;
        if(earliestFirst){
            //Extra element from allocateExtraSpace is at the front of the array, the start of the buffer for the delay is now 1 index up
            int ind = allocateExtraSpace ? 1 : 0;
            assignToInInputLoop_Re = cStateVar.getCVarName(false) + "[" + GeneralHelper::to_string(ind) + "]";
            if(cStateVar.getDataType().isComplex()) {
                assignToInInputLoop_Im = cStateVar.getCVarName(true) + "[" + GeneralHelper::to_string(ind) + "]";
            }
        }else {
            //Extra element from allocateExtraSpace is at the end of the array, no modification nessasary
            assignToInInputLoop_Re = cStateVar.getCVarName(false) + "[" + GeneralHelper::to_string(delayValue - 1) + "]";
            if(cStateVar.getDataType().isComplex()) {
                assignToInInputLoop_Im = cStateVar.getCVarName(true) + "[" + GeneralHelper::to_string(delayValue - 1) + "]";
            }
        }

        std::string assignFromInInputLoop_Re = cStateInputVar.getCVarName(false);
        std::string assignFromInInputLoop_Im;
        if(cStateVar.getDataType().isComplex()) {
            assignFromInInputLoop_Im = cStateInputVar.getCVarName(true);
        }

        //For loop if input is vector/matrix
        //Deref is nessissary
        std::vector<std::string> forLoopInputClose;
        if(!inputDT.isScalar()){
            std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                    EmitterHelpers::generateVectorMatrixForLoops(inputDT.getDimensions());
            std::vector<std::string> forLoopInputOpen = std::get<0>(forLoopStrs);
            std::vector<std::string> forLoopInputIndexVars = std::get<1>(forLoopStrs);
            forLoopInputClose = std::get<2>(forLoopStrs);
            cStatementQueue.insert(cStatementQueue.end(), forLoopInputOpen.begin(), forLoopInputOpen.end());

            assignToInInputLoop_Re += EmitterHelpers::generateIndexOperation(forLoopInputIndexVars);
            assignFromInInputLoop_Re += EmitterHelpers::generateIndexOperation(forLoopInputIndexVars);
            if(cStateVar.getDataType().isComplex()) {
                assignToInInputLoop_Im += EmitterHelpers::generateIndexOperation(forLoopInputIndexVars);
                assignFromInInputLoop_Im += EmitterHelpers::generateIndexOperation(forLoopInputIndexVars);
            }
        }

        cStatementQueue.push_back(assignToInInputLoop_Re + " = " + assignFromInInputLoop_Re + ";");
        if(cStateVar.getDataType().isComplex()){
            cStatementQueue.push_back(assignToInInputLoop_Im + " = " + assignFromInInputLoop_Im + ";");
        }

        //Close inner for loops
        if(!inputDT.isScalar()){
            cStatementQueue.insert(cStatementQueue.end(), forLoopInputClose.begin(), forLoopInputClose.end());
        }
    }

    Node::emitCStateUpdate(cStatementQueue, schedType, stateUpdateSrc);
}

void Delay::emitCExprNextState(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType) {
    DataType inputDataType = getInputPort(0)->getDataType();
    std::shared_ptr<OutputPort> srcPort = getInputPort(0)->getSrcOutputPort();
    int srcOutPortNum = srcPort->getPortNum();
    std::shared_ptr<Node> srcNode = srcPort->getParent();

    //Emit the upstream
    CExpr inputExprRe = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, false);
    CExpr inputExprIm;

    if(inputDataType.isComplex()){
        inputExprIm = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, true);
    }

    //Assign the expr to a special variable defined here (before the state update)
    std::string stateInputName = name + "_n" + GeneralHelper::to_string(id) + "_state_input";
    DataType inputDT = getInputPort(0)->getDataType();
    Variable stateInputVar = Variable(stateInputName, inputDT);
    cStateInputVar = stateInputVar;

    //Declare the input variables here since input may be a vector/matrix
    cStatementQueue.push_back(stateInputVar.getCVarDecl(false, true, false, true) + ";");
    if(inputDataType.isComplex()){
        cStatementQueue.push_back(stateInputVar.getCVarDecl(true, true, false, true) + ";");
    }

    //Create for loop here if input (not delay state) is a vector/matrix
    std::vector<std::string> forLoopIndexVars;
    std::vector<std::string> forLoopClose;
    //If the output is a vector, construct a for loop which puts the results in a temporary array
    if(!inputDT.isScalar()){
        //Create nested loops for a given array
        std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                EmitterHelpers::generateVectorMatrixForLoops(inputDT.getDimensions());
        std::vector<std::string> forLoopOpen = std::get<0>(forLoopStrs);
        forLoopIndexVars = std::get<1>(forLoopStrs);
        forLoopClose = std::get<2>(forLoopStrs);
        cStatementQueue.insert(cStatementQueue.end(), forLoopOpen.begin(), forLoopOpen.end());
    }

    //Deref if input vector/matrix
    std::vector<std::string> emptyArr;
    std::string stateInputDeclAssignRe = stateInputVar.getCVarName(false) + (inputDT.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars)) +
            " = " + inputExprRe.getExprIndexed(inputDT.isScalar() ? emptyArr : forLoopIndexVars, true) + ";";
    cStatementQueue.push_back(stateInputDeclAssignRe);
    if(inputDataType.isComplex()){
        std::string stateInputDeclAssignIm = stateInputVar.getCVarName(true) +(inputDT.isScalar() ? "" : EmitterHelpers::generateIndexOperation(forLoopIndexVars)) +
                " = " + inputExprIm.getExprIndexed(inputDT.isScalar() ? emptyArr : forLoopIndexVars, true) + ";";
        cStatementQueue.push_back(stateInputDeclAssignIm);
    }

    //Close for loop
    if(!inputDT.isScalar()){
        cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
    }
}

Delay::Delay(std::shared_ptr<SubSystem> parent, Delay* orig) : PrimitiveNode(parent, orig), delayValue(orig->delayValue),
    initCondition(orig->initCondition), cStateVar(orig->cStateVar), cStateInputVar(orig->cStateInputVar),
    earliestFirst(orig->earliestFirst), allocateExtraSpace(orig->allocateExtraSpace){

}

std::shared_ptr<Node> Delay::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Delay>(parent, this);
}

bool Delay::createStateUpdateNode(std::vector<std::shared_ptr<Node>> &new_nodes,
                                  std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                  std::vector<std::shared_ptr<Arc>> &new_arcs,
                                  std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                  bool includeContext) {

    return GraphAlgs::createStateUpdateNodeDelayStyle(getSharedPointer(), new_nodes, deleted_nodes, new_arcs, deleted_arcs, includeContext);
}

bool Delay::hasCombinationalPath() {
    return false; //This is a pure state element
}

bool Delay::isEarliestFirst() const {
    return earliestFirst;
}

void Delay::setEarliestFirst(bool earliestFirst) {
    Delay::earliestFirst = earliestFirst;
}

bool Delay::isAllocateExtraSpace() const {
    return allocateExtraSpace;
}

void Delay::setAllocateExtraSpace(bool allocateExtraSpace) {
    Delay::allocateExtraSpace = allocateExtraSpace;
}
