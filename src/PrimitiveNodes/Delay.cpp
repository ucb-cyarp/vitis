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
#include "Passes/MultiThreadPasses.h"

Delay::Delay() : delayValue(0), earliestFirst(false), allocateExtraSpace(false), bufferImplementation(BufferType::AUTO),
                 roundCircularBufferToPowerOf2(true), circularBufferType(CircularBufferType::NO_EXTRA_LEN), transactionBlockSize(1),
                 blockingSpecializationDeferred(false), deferredBlockSize(0), deferredSubBlockSize(0),
                 copyMethod(CopyMethod::CLANG_MEMCPY_INLINE){

}

Delay::Delay(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), delayValue(0), earliestFirst(false),
                                                  allocateExtraSpace(false), bufferImplementation(BufferType::AUTO),
                                                  roundCircularBufferToPowerOf2(true),
                                                  circularBufferType(CircularBufferType::NO_EXTRA_LEN),
                                                  transactionBlockSize(1),
                                                  blockingSpecializationDeferred(false),
                                                  deferredBlockSize(0),
                                                  deferredSubBlockSize(0),
                                                  copyMethod(CopyMethod::CLANG_MEMCPY_INLINE){

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

        int blockSize = std::stoi(dataKeyValueMap.at("BlockSize"));
        node->setTransactionBlockSize(blockSize);
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

        node->setTransactionBlockSize(1);
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Delay", node));
    }

    int delayVal;
    if(dataKeyValueMap.at("block_function") == "VectorTappedDelay" || dataKeyValueMap.at("block_function") == "TappedDelayWithReset") {
        delayVal = std::stoi(delayStr)-1; //Vector Tapped Delay and TappedDelayWithReset (both Laminar Library Blocks) Depth is the delay+1
    }else{
        delayVal = std::stoi(delayStr);
    }
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

    //Note: Simulink initial conditions for TappedDelay are always presented in the same order, regardless of the
    //      setting of earliestFirst.  Internally, if earliestFirst is true, we reverse the order of the provided initial conditions.
    //      To preserve compatibility with simulink, the initial conditions will be reversed during import if earliestFirst is true.
    //      Done in TappedDelay class.

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
    propagatePropertiesHelper();
}

std::set<GraphMLParameter> Delay::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("DelayLength", "int", true));
    parameters.insert(GraphMLParameter("InitialCondition", "string", true));
    parameters.insert(GraphMLParameter("BlockSize", "int", true));

    return parameters;
}

void Delay::emitGraphMLDelayParams(xercesc::DOMDocument *doc, xercesc::DOMElement *xmlNode) {
    GraphMLHelper::addDataNode(doc, xmlNode, "DelayLength", GeneralHelper::to_string(delayValue));
    GraphMLHelper::addDataNode(doc, xmlNode, "InitialCondition", NumericValue::toString(initCondition));
    GraphMLHelper::addDataNode(doc, xmlNode, "BlockSize", GeneralHelper::to_string(transactionBlockSize));
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
             "\nDelayLength: " + GeneralHelper::to_string(delayValue) +
             "\nInitialCondition: " + NumericValue::toString(initCondition);

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

    if(transactionBlockSize < 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Delay - Block Size must be >= 1", getSharedPointer()));
    }

    //For now, we will assume that, for block sizes > 1, circular buffers are being used with double array.  earliestFirst=false
    //allocation.
    //TODO: Implement other variants
    if(transactionBlockSize > 1 && delayValue > 0 && (!usesCircularBuffer() || circularBufferType != CircularBufferType::DOUBLE_LEN)){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Delay - With Block Size > 1, only circular buffers with double buffer length are currently supported", getSharedPointer()));
    }
    if(transactionBlockSize > 1 && delayValue > 0 && earliestFirst){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Delay - With Block Size > 1, earliestFirst is not supported", getSharedPointer()));
    }

    if(!blockingSpecializationDeferred) { //If specialization was deferred, there will be a conflict between the arc types and the expected arc types of the node.  This will be resolved when the specialization occurs.
        //Check that input port and the output port have the same type
        DataType outType = getOutputPort(0)->getDataType();
        DataType inType = getInputPort(0)->getDataType();

        if (inType != outType) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "Validation Failed - Delay - DataType of Input Port Does not Match Output Port",
                    getSharedPointer()));
        }

        std::vector<int> typeDim = inType.getDimensions();
        if (transactionBlockSize > 1) {
            if (typeDim[0] != transactionBlockSize) {
                throw std::runtime_error(ErrorHelpers::genErrorStr(
                        "Validation Failed - Delay - For Block Size > 1, Outer Dimension of I/O Should be the Block Size",
                        getSharedPointer()));
            }
        }

        int arraySize = delayValue;
        if (allocateExtraSpace) {
            arraySize++;
        }

        int expectedSize;
        if (usesCircularBuffer()) {
            expectedSize = getBufferAllocatedLen() * inType.numberOfElements();
            if (transactionBlockSize > 1) {
                //The allocated length is in terms of the sub-elements and the input was expanded by the block size
                expectedSize /= transactionBlockSize;
            }
        } else {
            expectedSize = arraySize * inType.numberOfElements();
        }

        std::vector<NumericValue> reshapedInitCond = getInitConditionsReshapedForConfig();
        if (delayValue != 0 && expectedSize != reshapedInitCond.size()) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "Validation Failed - Delay - Delay Length (" + GeneralHelper::to_string(delayValue) +
                    "), Element Dimensions (" + GeneralHelper::to_string(inType.numberOfElements()) +
                    "), Reshaped Init Condition Vector (" +
                    GeneralHelper::to_string(reshapedInitCond.size()) +
                    ") does not match the expected initial condition length (" +
                    GeneralHelper::to_string(expectedSize) +
                    ")", getSharedPointer()));
        }
    }

    //TODO: Add error check for extra length circular buffers in standard delay node
    //      Remove after updating FIFO delay absorption to handle alternate implementations (which introduce additional initial conditions)
    if(transactionBlockSize == 1 && circularBufferType != CircularBufferType::NO_EXTRA_LEN){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Standard Delay nodes do not currently support alternate circular buffer implementations when block size == 1", getSharedPointer()));
    }

    //TODO: Add error check for extra length circular buffers in standard delay node
    //      Remove after updating FIFO delay absorption to handle the extra space ((which introduces an additional initial condition)
    if(allocateExtraSpace){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Standard Delay nodes should not allocate extra space", getSharedPointer()));
    }

}

bool Delay::hasState() {
    if(delayValue > 0) {
        return true;
    }else{
        return false;
    }
}

int Delay::getCircBufferInitialIdx() {
    //For the standard delay (and block size == 1), the circular buffer is always initialized to 0

    if(transactionBlockSize == 1) {
        return 0;
    }else{
        if(circularBufferType == CircularBufferType::DOUBLE_LEN && !earliestFirst) {
            // The circular buffer initial index is initialized to be at the midpoint of the allocated buffer.
            return getBufferLength();
        }else{
            //For now, we will assume that, for block sizes > 1, circular buffers are being used with double array
            //allocation and !earliestFirst
            //TODO: Implement other variants
            throw std::runtime_error(ErrorHelpers::genErrorStr("With Block Size > 1, only circular buffers with double buffer length, and !earliestFirst are currently supported", shared_from_this()));
        }
    }
}

std::vector<Variable> Delay::getCStateVars() {
    std::vector<Variable> vars;

    //If delayValue == 0 return empty vector
    if(delayValue == 0){
        return vars;
    }else{
        int arrayLen = getBufferAllocatedLen(); //This also account for if a circular buffer is used and roundCircularBufferToPowerOf2 is selected
        //Note that arrayLen is in terms of blocks

        if(usesCircularBuffer()){
            //Declare the circular buffer offset var
            //This is initialized to be 0 at the start
            NumericValue offsetInitVal = NumericValue((long) getCircBufferInitialIdx());
            int maxOffSetVal = arrayLen;
            if(transactionBlockSize == 1 && GeneralHelper::isPowOf2(getBufferLength())){
                maxOffSetVal++; //This is to support the mod method when block size = 1 and buffer length is a power of 2.
            }
            DataType offsetDT = DataType(false, false, false, (int) std::ceil(std::log2(maxOffSetVal)), 0, //+Transaction block size so that the wrapping logic can occur with a mux
                                         {1}).getCPUStorageType();
            std::string offsetVarName = name+"_n"+GeneralHelper::to_string(id)+"_circBufHeadInd";
            Variable offsetVar = Variable(offsetVarName, offsetDT, {offsetInitVal}, false, true);
            circularBufferOffsetVar = offsetVar;
            vars.push_back(offsetVar);
        }

        //There is a single state variable for the delay.
        // The way the expansion occurs is different depending on if the block size is > 1.
        // For a block size of 1 and a delay > 1, it is a vector if the input datatype is scalar and a matrix if the input datatype is a vector or matrix.  ie. the dimensionality of the type is expanded.  The delay is in terms of the input type and is achieved by indexing in the outer domain.
        // For a block size >1, multiple blocks may be accessed at once.  The delay type is in terms of the elements making up the block.  However, the arrayLength is still in terms of blocks.  We simply extend the outer dimension by a multiple of the arrayLength.  The outer dimension should be the block size
        //Note that if the delay is 1, no expansion is required
        DataType stateType;
        //TODO: update
        if(transactionBlockSize == 1) {
            if (arrayLen == 1) { //Note, allocateExtra space will neve have an array length of 1 because it has no effect if the delay length is 0 (checked above)
                stateType = getInputPort(0)->getDataType();
            } else {
                stateType = getInputPort(0)->getDataType().expandForBlock(arrayLen); //The vector elements will be stored contiguously
            }
        }else{
            //Should not be possibly to have arrayLen that is <=1 if using Circular buffer and double buffer length
            //TODO: Update if allowing other configurations
            std::vector<int> inputDimensions = getInputPort(0)->getDataType().getDimensions();
            //Scale outer dimension
            inputDimensions[0] /= transactionBlockSize; //When this function is being called the arcs into and out of the delay have already been expanded by the block size.  Need to undo that
            inputDimensions[0] *= arrayLen;
            stateType = getInputPort(0)->getDataType();
            stateType.setDimensions(inputDimensions);
        }

        std::string varName = name+"_n"+GeneralHelper::to_string(id)+"_state";
        Variable var = Variable(varName, stateType, getInitConditionsReshapedForConfig(), false, true); //The initial condition was reversed from the input if earliestFirst is selected
        cStateVar = var;
        //Complex variable will be made if needed by the design code based on the data type
        vars.push_back(var);

        return vars;
    }
}

CExpr Delay::emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum, bool imag) {
    bool hasRstPort = inputPorts.size() > 1;
    if(hasRstPort){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Async reset line only supported with tapped delays with passthrough"));
    }

    if(delayValue == 0){ //Regardless of the block size, the logic is the same for delayValue == 0 ... it is a simple passthrough
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
        if(transactionBlockSize == 1) { //This is the orig logic for delay
            //Return the state var name as the expression
            int arrayLen = delayValue;
            if (allocateExtraSpace) {
                arrayLen++;
            }
            if (arrayLen == 1) {
                //Return the simple name (no index needed as it is not an array)
                //Since allocateExtraSpace has no effect with delayVal == 0, it cannot be true and have an arrayLen of 1.  Therefore, no special action is taken
                return CExpr(cStateVar.getCVarName(imag),
                             getOutputPort(outputPortNum)->getDataType().isScalar() ? CExpr::ExprType::SCALAR_VAR
                                                                                    : CExpr::ExprType::ARRAY); //This is a variable name therefore inform the cEmit function
            } else {
                //Since datatype expansion adds another dimension to the front of the list (elements are stored contiguously in memory(, dereferencing the first index works even if the type is a vector or array
                if (usesCircularBuffer()) {
                    //TODO: Add error for standard delay c emit and extra length circular buffers

                    if (earliestFirst) {
                        //The oldest (latest) value is at the end of the array
                        //Need to correct for the extra space at the front of the array.
                        //arrayLen above is corrected when an extra array element is
                        std::string indValue = "(" + circularBufferOffsetVar.getCVarName(false) + "+" +
                                               GeneralHelper::to_string(arrayLen - 1) + ")%" +
                                               GeneralHelper::to_string(getBufferLength());
                        return CExpr(cStateVar.getCVarName(imag) + "[" + indValue + "]",
                                     getOutputPort(outputPortNum)->getDataType().isScalar()
                                     ? CExpr::ExprType::SCALAR_VAR
                                     : CExpr::ExprType::ARRAY);

                    } else {
                        //Return the value at the front of the array (at the current offset)
                        return CExpr(
                                cStateVar.getCVarName(imag) + "[" + circularBufferOffsetVar.getCVarName(false) + "]",
                                getOutputPort(outputPortNum)->getDataType().isScalar() ? CExpr::ExprType::SCALAR_VAR
                                                                                       : CExpr::ExprType::ARRAY);
                    }
                } else {
                    if (earliestFirst) {
                        //If the samples come in at the beginning of the array, return the end of the array
                        //The extra space is at the front of the array, therefore, need to offset the returned index
                        return CExpr(cStateVar.getCVarName(imag) + "[" + GeneralHelper::to_string(arrayLen - 1) + "]",
                                     getOutputPort(outputPortNum)->getDataType().isScalar()
                                     ? CExpr::ExprType::SCALAR_VAR
                                     : CExpr::ExprType::ARRAY);
                    } else {
                        //If later samples are at the beginning of the array, return the front of the array
                        //The extra space from allocateExtraSpace is at the end of the array and is not a factor in the return
                        return CExpr(cStateVar.getCVarName(imag) + "[0]",
                                     getOutputPort(outputPortNum)->getDataType().isScalar()
                                     ? CExpr::ExprType::SCALAR_VAR
                                     : CExpr::ExprType::ARRAY);
                    }
                }
            }
        }else{
            //Block size > 1.  We are indexing into the elements within blocks.
            //The buffer outer dimension was extended by the block size.  It is in terms of elements
            //TODO: implement

            //For now, we will assume that, for block sizes > 1, circular buffers are being used with double array, earliestFirst=false
            //allocation.
            //TODO: Implement other variants
            if(earliestFirst || (!usesCircularBuffer() || circularBufferType != CircularBufferType::DOUBLE_LEN)){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Delay - With Block Size > 1, only circular buffers with double buffer length are currently supported", getSharedPointer()));
            }

            if(delayValue < transactionBlockSize){
                //If delay < block size, need to process the input first, write it into the array, then can emit output
                //This is essentially the same action that is performed in TappedDelay when allocateExtra == true
                std::shared_ptr<OutputPort> srcPort = getInputPort(0)->getSrcOutputPort();
                int srcOutPortNum = srcPort->getPortNum();
                std::shared_ptr<Node> srcNode = srcPort->getParent();

                CExpr inputExpr = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, imag);

                //Insert into the circular buffer position
                std::string insertPosition = circularBufferOffsetVar.getCVarName(false);

                //The function should handle the second write with the circular buffer
                assignInputToBuffer(inputExpr, insertPosition, imag, cStatementQueue);
            }

            //If delay >= block size, can process in any order.  No need to process the input now

            //Return the array.  It is the current circular buffer offset minus delay for standard delays
            //Note that this is the circularBufferOffset position where the values from this iteration will be written
            //(ie. before the circularBufferOffsetVar is incremented)
            return CExpr( "(" + cStateVar.getCVarName(imag) +
            "+(" + circularBufferOffsetVar.getCVarName(false) + "-" + GeneralHelper::to_string(delayValue) + "))",
            CExpr::ExprType::ARRAY);
        }
    }
}

void Delay::emitCStateUpdate(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, std::shared_ptr<StateUpdate> stateUpdateSrc) {
    //TODO: Implement Rst input

    DataType inputDT = getInputPort(
            0)->getDataType(); //This is the datatype of the input and does not include any extra dimension for the buffer

    if (delayValue == 0) {
        return; //No state to update
    } else if(transactionBlockSize == 1)  {
        if (delayValue == 1 && !allocateExtraSpace){
            //The state variable is not an array

            //Emit for loop if the input is a vector/matrix

            std::vector<std::string> forLoopIndexVars;
            std::vector<std::string> forLoopClose;
            //Will check if complex below
            std::string cStateInputVarDeref_Re = cStateInputVar.getCVarName(
                    false); //Will be deref-ed if the input type is a vector/matrix
            std::string cStateVarDeref_Re = cStateVar.getCVarName(
                    false); //Will be deref-ed if the input type is a vector/matrix

            std::string cStateInputVarDeref_Im;
            std::string cStateVarDeref_Im;
            if (cStateVar.getDataType().isComplex()) {
                cStateInputVarDeref_Im = cStateInputVar.getCVarName(
                        true); //Will be deref-ed if the input type is a vector/matrix
                cStateVarDeref_Im = cStateVar.getCVarName(true); //Will be deref-ed if the input type is a vector/matrix
            }

            if (!inputDT.isScalar()) {
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
                if (cStateVar.getDataType().isComplex()) {
                    cStateVarDeref_Im += EmitterHelpers::generateIndexOperation(forLoopIndexVars);
                    cStateInputVarDeref_Im += EmitterHelpers::generateIndexOperation(forLoopIndexVars);
                }
            }

            //Emit assignment
            cStatementQueue.push_back(cStateVarDeref_Re + " = " + cStateInputVarDeref_Re + ";");
            if (cStateVar.getDataType().isComplex()) {
                cStatementQueue.push_back(cStateVarDeref_Im + " = " + cStateInputVarDeref_Im + ";");
            }

            //close for loop
            if (!inputDT.isScalar()) {
                cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
            }

        } else {
            if (usesCircularBuffer()) {
                //TODO: Need to update to handle extra space circular buffers

                //We are using a circular buffer
                //instead of shifting values, we will insert the new value into the array and then update the offset
                //variable.  Depending on whether earliest first is selected, the offset is either incremented or decremented
                //Note that, due to the behavior of % for signed numbers, if the array size is not forced to be a power of 2
                //an if/else will be used to handle the wraparound condition.  Otherwise, a % operator will be used.
                //this is OK with unsigned wrapparound and masking with a power of 2 long array)

                if (earliestFirst) {
                    //The earliest values (newest values) are stored at the front of the array.
                    //The latest values (oldest values) are stored at the end of the array

                    if (allocateExtraSpace) {
                        if (requiresStandaloneCExprNextState()) {
                            //Before modifying the offset, the occupied elements are [offset, offset+delay] (because [offset] is the extra space)
                            //The new value is stored into the extra space ([offset])
                            assignInputToBuffer(circularBufferOffsetVar.getCVarName(false), cStatementQueue);
                        } else {
                            cStatementQueue.push_back("//Explicit next state assignment for " + getFullyQualifiedName() +
                                                      " [ID: " + GeneralHelper::to_string(id) + ", Type:" + typeNameStr() +
                                                      "] is not needed because a combinational path exists through the node and the input has already been captured in the Delay buffer");
                        }

                        //The offset can then be decremented.
                        decrementAndWrapCircularBufferOffset(cStatementQueue);
                        //The extra space will be at the [new_offset] address.
                        //The range of values not including the extra space are at [new_offset+1, new_offset+delay]
                    } else {
                        //Before modifying the offset, the occupied elements are [offset, offset+delay-1]
                        //The new value is stored at [offset-1]
                        //The offset is decremented
                        decrementAndWrapCircularBufferOffset(cStatementQueue);
                        //The new value is stored at [new_offset]
                        assignInputToBuffer(circularBufferOffsetVar.getCVarName(false), cStatementQueue);
                        //The range of values ranges from [new_offset, new_offset+delay-1]
                    }

                    //For a standard delay, the oldest value is returned
                    //    ExtraSpace: [new_offset+delay]
                    //    Standard: [new_offset+delay-1]

                } else {
                    //The latest values (oldest values) are stored at the front of the array.
                    //The earliest values (newest values) are stored at the end of the array.

                    //The insertion logic is the same regardless of whether an extra space was allocated or not:
                    //This is because the offset is where the last element in the array is in both the ExtraSpace and Standard case
                    //ExtraSpace:
                    //    At this point [offset, offset+delay] are occupied (because of the extra space at [offset+delay] allocated)
                    //    The new value is inserted into the extra space [offset+delay]
                    //    the offset is then incremented
                    //    The extra space is now at [new_offset+delay].
                    //    The range of values not including the extra space are [new_offset, new_offset+delay-1]
                    //Standard:
                    //    At this point [offset, offset+delay-1] are occupied
                    //    the new value is inserted into the next space at [offset+delay]
                    //    The offset is then incremented
                    //    The range of value is now [new_offset, new_offset+delay-1]

                    if (requiresStandaloneCExprNextState()) { //Includes check for allocateExtraSpace
                        std::string insertPosition = "(" + circularBufferOffsetVar.getCVarName(false) + "+" +
                                                     GeneralHelper::to_string(delayValue) + ")%" +
                                                     GeneralHelper::to_string(getBufferLength());
                        assignInputToBuffer(insertPosition, cStatementQueue);
                    } else {
                        cStatementQueue.push_back("//Explicit next state assignment for " + getFullyQualifiedName() +
                                                  " [ID: " + GeneralHelper::to_string(id) + ", Type:" + typeNameStr() +
                                                  "] is not needed because a combinational path exists through the node and the input has already been captured in the Delay buffer");
                    }
                    incrementAndWrapCircularBufferOffset(cStatementQueue);

                    //For a standard delay, both ExtraSpace and Standard return the oldest element which is at [new_offset]
                }
            } else {
                //This is a (pair) of arrays
                //Emit a for loop to perform the shift for each

                //----Shift State ----
                std::string loopVarNameUnsanitized = name + "_n" + GeneralHelper::to_string(id) + "_loopCounter";
                Variable loopVar(loopVarNameUnsanitized, DataType());
                std::string loopVarNameSanitized = loopVar.getCVarName(false);

                //Shift state loop
                std::string assignToInShiftLoop_Re = cStateVar.getCVarName(false) + "[" + loopVarNameSanitized + "]";
                std::string assignToInShiftLoop_Im;
                if (cStateVar.getDataType().isComplex()) {
                    assignToInShiftLoop_Im = cStateVar.getCVarName(true) + "[" + loopVarNameSanitized + "]";
                }

                int arrayLength = delayValue;
                if (allocateExtraSpace) {
                    arrayLength++;
                }

                std::string assignFromInShiftLoop_Re;
                std::string assignFromInShiftLoop_Im;
                if (earliestFirst) {
                    //If allocateExtraSpace, the extra space is at the front of the array.  The shift indexes now need to be increased by 1
                    //However, since the current value will have already been writen durring cEmit, can just add it to the shift
                    int lowInd = requiresStandaloneCExprNextState() ? 1 : 0;
                    cStatementQueue.push_back("for(unsigned long " + loopVarNameSanitized + " = " +
                                              GeneralHelper::to_string(arrayLength - 1) + "; " + loopVarNameSanitized +
                                              " > " + GeneralHelper::to_string(lowInd) + "; " + loopVarNameSanitized +
                                              "--){");
                    assignFromInShiftLoop_Re = cStateVar.getCVarName(false) + "[" + loopVarNameSanitized + "-1]";
                    if (cStateVar.getDataType().isComplex()) {
                        assignFromInShiftLoop_Im = cStateVar.getCVarName(true) + "[" + loopVarNameSanitized + "-1]";
                    }
                } else {
                    //If allocateExtraSpace, the extra space is at the end of the array, shifting does not need to extend to that ind so no additional
                    int endInd = requiresStandaloneCExprNextState() ? delayValue - 1 : delayValue;
                    cStatementQueue.push_back(
                            "for(unsigned long " + loopVarNameSanitized + " = 0; " + loopVarNameSanitized + " < " +
                            GeneralHelper::to_string(endInd) + "; " + loopVarNameSanitized + "++){");
                    assignFromInShiftLoop_Re = cStateVar.getCVarName(false) + "[" + loopVarNameSanitized + "+1]";
                    if (cStateVar.getDataType().isComplex()) {
                        assignFromInShiftLoop_Im = cStateVar.getCVarName(true) + "[" + loopVarNameSanitized + "+1]";
                    }
                }

                //If the input is not a scalar, insert additional nested for loops
                std::vector<std::string> forLoopShiftClose;
                if (!inputDT.isScalar()) {
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
                    if (cStateVar.getDataType().isComplex()) {
                        assignToInShiftLoop_Im += EmitterHelpers::generateIndexOperation(forLoopShiftIndexVars);
                        assignFromInShiftLoop_Im += EmitterHelpers::generateIndexOperation(forLoopShiftIndexVars);
                    }
                }

                //Assign shift
                cStatementQueue.push_back(assignToInShiftLoop_Re + " = " + assignFromInShiftLoop_Re + ";");
                if (cStateVar.getDataType().isComplex()) {
                    cStatementQueue.push_back(assignToInShiftLoop_Im + " = " + assignFromInShiftLoop_Im + ";");
                }

                //Close inner for loops
                if (!inputDT.isScalar()) {
                    cStatementQueue.insert(cStatementQueue.end(), forLoopShiftClose.begin(), forLoopShiftClose.end());
                }

                //Close shift loop
                cStatementQueue.push_back("}");

                //----Input store----
                if (requiresStandaloneCExprNextState()) {
                    std::string assignToInInputLoop_Re;
                    std::string assignToInInputLoop_Im;
                    if (earliestFirst) {
                        //Extra element from allocateExtraSpace is at the front of the array, the start of the buffer for the delay is now 1 index up
                        int ind = allocateExtraSpace ? 1 : 0;
                        assignToInInputLoop_Re = cStateVar.getCVarName(false) + "[" + GeneralHelper::to_string(ind) + "]";
                        if (cStateVar.getDataType().isComplex()) {
                            assignToInInputLoop_Im =
                                    cStateVar.getCVarName(true) + "[" + GeneralHelper::to_string(ind) + "]";
                        }
                    } else {
                        //Extra element from allocateExtraSpace is at the end of the array, no modification nessasary
                        assignToInInputLoop_Re =
                                cStateVar.getCVarName(false) + "[" + GeneralHelper::to_string(delayValue - 1) + "]";
                        if (cStateVar.getDataType().isComplex()) {
                            assignToInInputLoop_Im =
                                    cStateVar.getCVarName(true) + "[" + GeneralHelper::to_string(delayValue - 1) + "]";
                        }
                    }

                    std::string assignFromInInputLoop_Re = cStateInputVar.getCVarName(false);
                    std::string assignFromInInputLoop_Im;
                    if (cStateVar.getDataType().isComplex()) {
                        assignFromInInputLoop_Im = cStateInputVar.getCVarName(true);
                    }

                    //For loop if input is vector/matrix
                    //Deref is nessissary
                    std::vector<std::string> forLoopInputClose;
                    if (!inputDT.isScalar()) {
                        std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                                EmitterHelpers::generateVectorMatrixForLoops(inputDT.getDimensions());
                        std::vector<std::string> forLoopInputOpen = std::get<0>(forLoopStrs);
                        std::vector<std::string> forLoopInputIndexVars = std::get<1>(forLoopStrs);
                        forLoopInputClose = std::get<2>(forLoopStrs);
                        cStatementQueue.insert(cStatementQueue.end(), forLoopInputOpen.begin(), forLoopInputOpen.end());

                        assignToInInputLoop_Re += EmitterHelpers::generateIndexOperation(forLoopInputIndexVars);
                        assignFromInInputLoop_Re += EmitterHelpers::generateIndexOperation(forLoopInputIndexVars);
                        if (cStateVar.getDataType().isComplex()) {
                            assignToInInputLoop_Im += EmitterHelpers::generateIndexOperation(forLoopInputIndexVars);
                            assignFromInInputLoop_Im += EmitterHelpers::generateIndexOperation(forLoopInputIndexVars);
                        }
                    }

                    cStatementQueue.push_back(assignToInInputLoop_Re + " = " + assignFromInInputLoop_Re + ";");
                    if (cStateVar.getDataType().isComplex()) {
                        cStatementQueue.push_back(assignToInInputLoop_Im + " = " + assignFromInInputLoop_Im + ";");
                    }

                    //Close inner for loops
                    if (!inputDT.isScalar()) {
                        cStatementQueue.insert(cStatementQueue.end(), forLoopInputClose.begin(), forLoopInputClose.end());
                    }
                } else {
                    cStatementQueue.push_back("//Explicit next state assignment for " + getFullyQualifiedName() +
                                              " [ID: " + GeneralHelper::to_string(id) + ", Type:" + typeNameStr() +
                                              "] is not needed because a combinational path exists through the node and the input has already been captured in the Delay buffer");
                }
            }
        }
    }else{
        //block size > 1

        //For now, we will assume that, for block sizes > 1, circular buffers are being used with double array, earliestFirst=false
        //allocation.
        //TODO: Implement other variants
        if(earliestFirst || (!usesCircularBuffer() || circularBufferType != CircularBufferType::DOUBLE_LEN)){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Delay - With Block Size > 1, only circular buffers with double buffer length are currently supported", getSharedPointer()));
        }

        //This is similar to the logic for block size = 1, circular buffer, earliestFirst = false
        //It is even more similar to the logic for TappedDelay

        if (requiresStandaloneCExprNextState()) {//If the input is not processed as part of the output, we need to insert it here
            std::string insertPosition = circularBufferOffsetVar.getCVarName(false);
            assignInputToBuffer(insertPosition, cStatementQueue);
        } else {
            cStatementQueue.push_back("//Explicit next state assignment for " + getFullyQualifiedName() +
                                      " [ID: " + GeneralHelper::to_string(id) + ", Type:" + typeNameStr() +
                                      "] is not needed because a combinational path exists through the node and the input has already been captured in the Delay buffer");
        }
        incrementAndWrapCircularBufferOffset(cStatementQueue);

    }

    Node::emitCStateUpdate(cStatementQueue, schedType, stateUpdateSrc);
}

bool Delay::requiresStandaloneCExprNextState(){
    bool foundReOut = false;
    bool foundImOut = false;
    std::set<std::shared_ptr<Arc>> outputArcs = getDirectOutputArcs();
    for(auto const &outputArc : outputArcs){
        foundReOut = true;
        if(outputArc->getDataType().isComplex()){
            foundImOut = true;
        }
    }

    DataType inputDataType = getInputPort(0)->getDataType();
    bool outputExists = foundReOut && (inputDataType.isComplex() ? foundImOut : true);

    //Similar to when !allocateExtraSpace (ie. no combo path) with block size==1, the input is not processed
    //as part of the output emit when the delayValue>=blockSize.
    bool needExplicitAssignmentForBlocking = false;
    if(transactionBlockSize > 1){
        if(delayValue>=transactionBlockSize){
            needExplicitAssignmentForBlocking = true;
        }
    }

    return !allocateExtraSpace || !outputExists || needExplicitAssignmentForBlocking;
}

void Delay::emitCExprNextState(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType) {
    if(requiresStandaloneCExprNextState()) {
        DataType inputDataType = getInputPort(0)->getDataType();
        std::shared_ptr<OutputPort> srcPort = getInputPort(0)->getSrcOutputPort();
        int srcOutPortNum = srcPort->getPortNum();
        std::shared_ptr<Node> srcNode = srcPort->getParent();

        //Emit the upstream
        CExpr inputExprRe = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, false);
        CExpr inputExprIm;

        if (inputDataType.isComplex()) {
            inputExprIm = srcNode->emitC(cStatementQueue, schedType, srcOutPortNum, true);
        }

        //Assign the expr to a special variable defined here (before the state update)
        std::string stateInputName = name + "_n" + GeneralHelper::to_string(id) + "_state_input";
        DataType inputDT = getInputPort(0)->getDataType();
        Variable stateInputVar = Variable(stateInputName, inputDT);
        cStateInputVar = stateInputVar;

        //Declare the input variables here since input may be a vector/matrix
        cStatementQueue.push_back(stateInputVar.getCVarDecl(false, true, false, true) + ";");
        if (inputDataType.isComplex()) {
            cStatementQueue.push_back(stateInputVar.getCVarDecl(true, true, false, true) + ";");
        }

        //Create for loop here if input (not delay state) is a vector/matrix
        std::vector<std::string> forLoopIndexVars;
        std::vector<std::string> forLoopClose;
        //If the output is a vector, construct a for loop which puts the results in a temporary array
        if (!inputDT.isScalar()) {
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
        std::string stateInputDeclAssignRe = stateInputVar.getCVarName(false) +
                                             (inputDT.isScalar() ? "" : EmitterHelpers::generateIndexOperation(
                                                     forLoopIndexVars)) +
                                             " = " + inputExprRe.getExprIndexed(
                inputDT.isScalar() ? emptyArr : forLoopIndexVars, true) + ";";
        cStatementQueue.push_back(stateInputDeclAssignRe);
        if (inputDataType.isComplex()) {
            std::string stateInputDeclAssignIm = stateInputVar.getCVarName(true) +
                                                 (inputDT.isScalar() ? "" : EmitterHelpers::generateIndexOperation(
                                                         forLoopIndexVars)) +
                                                 " = " + inputExprIm.getExprIndexed(
                    inputDT.isScalar() ? emptyArr : forLoopIndexVars, true) + ";";
            cStatementQueue.push_back(stateInputDeclAssignIm);
        }

        //Close for loop
        if (!inputDT.isScalar()) {
            cStatementQueue.insert(cStatementQueue.end(), forLoopClose.begin(), forLoopClose.end());
        }
    }else{
        cStatementQueue.push_back("//Explicit next state for " + getFullyQualifiedName() +
                                  " [ID: " + GeneralHelper::to_string(id) + ", Type:" + typeNameStr() +
                                  "] is not needed because a combinational path exists through the node and the input has already been captured in the Delay buffer");
    }
}

Delay::Delay(std::shared_ptr<SubSystem> parent, Delay* orig) : PrimitiveNode(parent, orig), delayValue(orig->delayValue),
    initCondition(orig->initCondition), cStateVar(orig->cStateVar),
    cStateInputVar(orig->cStateInputVar), earliestFirst(orig->earliestFirst), allocateExtraSpace(orig->allocateExtraSpace),
    bufferImplementation(orig->bufferImplementation), circularBufferOffsetVar(orig->circularBufferOffsetVar),
    roundCircularBufferToPowerOf2(orig->roundCircularBufferToPowerOf2),
    circularBufferType(orig->circularBufferType),
    transactionBlockSize(orig->transactionBlockSize),
    blockingSpecializationDeferred(orig->blockingSpecializationDeferred),
    deferredBlockSize(orig->deferredBlockSize),
    deferredSubBlockSize(orig->deferredSubBlockSize),
    copyMethod(orig->copyMethod){
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
    if (delayValue == 0) {
        return true; //This is a passthrough
    } else {
        //For block size == 1, there is no combo path
        //Even if the delay is not split in 2 during sub-blocking (into multiples of the block length and one < block length), the
        if(delayValue >= transactionBlockSize){
            //There is no combo path because the delayed value is more than a block in the past
            //This is true even when using additional space allocated in the buffer.  For best results, should split delay
            //into delay that is a multiple of the block size and delay that is less than the block size
            return false;
        }else{
            //Since the delay is less than the block length, there is a dependence between blocks in the output.  This essentially creates
            //a combinational path
            return true;
        }
    }
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

Delay::BufferType Delay::getBufferImplementation() const {
    return bufferImplementation;
}

void Delay::setBufferImplementation(Delay::BufferType bufferImplementation) {
    Delay::bufferImplementation = bufferImplementation;
}

Delay::CircularBufferType Delay::getCircularBufferType() const {
    return circularBufferType;
}

void Delay::setCircularBufferType(Delay::CircularBufferType circularBufferType) {
    Delay::circularBufferType = circularBufferType;
}

bool Delay::usesCircularBuffer() {
    if(delayValue == 0){
        return false;
    }

    if(transactionBlockSize == 1) {
        if (delayValue == 1) {
            return false;
        }
    }

    if(bufferImplementation == BufferType::AUTO){
        //For now, only support circular buffer with double buffer allocation if block size > 1
        //TODO: Implement other options
        if(transactionBlockSize > 1){
            return true;
        }
        if (!getInputPort(0)->getDataType().isScalar()) {
            return true;
        }
        if(delayValue>2){
            return true;
        }
        return false;
    }else if(bufferImplementation == BufferType::SHIFT_REGISTER){
        return false;
    }else if(bufferImplementation == BufferType::CIRCULAR_BUFFER){
        return true;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown BufferType", getSharedPointer()));
    }

}

void Delay::decrementAndWrapCircularBufferOffset(std::vector<std::string> &cStatementQueue) {
    //Currently, do not support earliestFirst which is what uses this function
    if(transactionBlockSize > 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Delays with block size > 1 currently do not support earliestFirst", getSharedPointer()));
    }

    cStatementQueue.push_back("if(" + circularBufferOffsetVar.getCVarName(false) + "<=0){");
    cStatementQueue.push_back("\t" + circularBufferOffsetVar.getCVarName(false) + "=" + GeneralHelper::to_string(getBufferLength()-1) + ";");
    cStatementQueue.push_back("}else{");
    cStatementQueue.push_back("\t" + circularBufferOffsetVar.getCVarName(false) + "--" + ";");
    cStatementQueue.push_back("}");
}

void Delay::incrementAndWrapCircularBufferOffset(std::vector<std::string> &cStatementQueue) {
    if(transactionBlockSize == 1) {
        //For standard delays with block size == 1, the offset is initially at 0
        if(GeneralHelper::isPowOf2(getBufferLength())) {
            //Only use the mod method if the buffer is a power of 2
            //Avoids non-power-of-2 mod
            cStatementQueue.push_back(circularBufferOffsetVar.getCVarName(false) +
                                      "=(" + circularBufferOffsetVar.getCVarName(false) + "+1)%" +
                                      GeneralHelper::to_string(getBufferLength()) + ";");
        }else{
            cStatementQueue.push_back("if(" + circularBufferOffsetVar.getCVarName(false) + ">=" + GeneralHelper::to_string(getBufferLength()-1) + "){");
            cStatementQueue.push_back("\t" + circularBufferOffsetVar.getCVarName(false) + "=0;");
            cStatementQueue.push_back("}else{");
            cStatementQueue.push_back("\t" + circularBufferOffsetVar.getCVarName(false) + "++;");
            cStatementQueue.push_back("}");
        }
    }else{
        //For now, we will assume that, for block sizes > 1, circular buffers are being used with double array, earliestFirst=false
        //allocation.
        //TODO: Implement other variants
        if(earliestFirst || (!usesCircularBuffer() || circularBufferType != CircularBufferType::DOUBLE_LEN)){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Delay - With Block Size > 1, only circular buffers with double buffer length are currently supported", getSharedPointer()));
        }

        //For block size > 1, currently only support circular buffers with block
        if(GeneralHelper::isPowOf2(getBufferAllocatedLen())){
            //Use the mod method
            cStatementQueue.push_back(circularBufferOffsetVar.getCVarName(false) +
                                      "=((" + circularBufferOffsetVar.getCVarName(false) + "+" +
                                      GeneralHelper::to_string(transactionBlockSize) + ")%" +
                                      GeneralHelper::to_string(getBufferLength()) + ")+" +
                                      GeneralHelper::to_string(getBufferLength()) + ";");
        }else{
            cStatementQueue.push_back("if(" + circularBufferOffsetVar.getCVarName(false) + ">=" + GeneralHelper::to_string(getBufferAllocatedLen()-transactionBlockSize) + "){");
            cStatementQueue.push_back("\t" + circularBufferOffsetVar.getCVarName(false) + "=" + GeneralHelper::to_string(getBufferLength()) + ";");
            cStatementQueue.push_back("}else{");
            cStatementQueue.push_back("\t" + circularBufferOffsetVar.getCVarName(false) + "+=" + GeneralHelper::to_string(transactionBlockSize) + ";");
            cStatementQueue.push_back("}");
        }
    }
}

int Delay::getBufferLength() {
    int arrayLen = 0;

    //Buffer len is always 0 when delayLen is 0, simple passthrough
    if(delayValue > 0) {
        if (transactionBlockSize == 1) {
            //Additional space is not required for the standard delay when allocateExtraSpace is false and the block size is 1
            arrayLen = delayValue;
            if (allocateExtraSpace) {
                //To hold the current value.
                arrayLen++;
            }

            if (usesCircularBuffer()) {
                if (roundCircularBufferToPowerOf2) {
                    //Round the array length up the nearest power of 2
                    double arrayLenRounded = std::round(std::pow(2, std::ceil(log2(arrayLen))));
                    arrayLen = (int) arrayLenRounded;
                }
            }
        } else {
            //When the block size is > 1, the buffer length is impacted by the block size if it is desired that the
            //input vector not be split and if the buffer should be over allocated to allow easy stride 1 access.

            //One option for handling cases where delay < block size for a standard delay is to allocate just delayValue
            //slots in a buffer and to perform array concatinaton to produce the output.  The last elements of the input
            //are saved into the state buffer.  This requires less explicit state but requires constructing the output vector.
            //This version does not really implement a shift register and instead uses the buffer to carry some results
            //from the previous block into the current one.

            //For now, we will assume that, for block sizes > 1, circular buffers are being used with double array
            //allocation.
            //TODO: Implement other variants
            if (!usesCircularBuffer() || circularBufferType != CircularBufferType::DOUBLE_LEN) {
                throw std::runtime_error(ErrorHelpers::genErrorStr(
                        "With Block Size > 1, only circular buffers with double buffer length are currently supported",
                        shared_from_this()));
            }

            double bufferLenDiv2 = transactionBlockSize * (1 + std::ceil(((double) delayValue) / transactionBlockSize));
            arrayLen = (int) bufferLenDiv2;

            //For now, will restrict rounding up to a power of 2 if the nearest power of 2 is not a multiple of the block length
            if (roundCircularBufferToPowerOf2) {
                //Try rounding the array length up the nearest power of 2
                double arrayLenRounded = std::round(std::pow(2, std::ceil(std::log2(arrayLen))));
                if (((int) arrayLenRounded) % transactionBlockSize == 0) {
                    arrayLen = (int) arrayLenRounded;
                }
            }
        }
    }

    return arrayLen;
}

void Delay::assignInputToBuffer(std::string insertPositionIn, std::vector<std::string> &cStatementQueue) {
    DataType inputDT = getInputPort(0)->getDataType();

    std::string input_ReUnindexed = cStateInputVar.getCVarName(false);
    std::string input_ImUnindexed;
    if (cStateVar.getDataType().isComplex()) {
        input_ImUnindexed = cStateInputVar.getCVarName(true);
    }

    //Note: for the circular buffer, the new item is placed at the cursor (with the given offset)
    //It used to be that there was an offset when earliestFirst was false and redundant writing (either DOUBLE_LEN,
    //or PLUS_DELAY_LEN_M1 were used).  Neither DOUBLE_LEN or PLUS_DELAY_LEN_M1 are allowed for standard delays.
    //Therefore, this modification should not effect standard delay operation.  Tapped delay overrides functions to
    //handle DOUBLE_LEN or PLUS_DELAY_LEN_M1 with !earliestFirst.
    std::string insertPosition = insertPositionIn;

    //==== Do the first assignment ====
    {
        //==== Do the first assignment ====
        std::string assignTo_Re = cStateVar.getCVarName(false);
        if (transactionBlockSize == 1) {
            //For block size == 1, the dimension of the buffer is formed by taking the input dimensions and expanding by the block size (adding a dimension).  Reference
            assignTo_Re += "[" + insertPosition + "]";
        }
        std::string assignTo_Im;
        if (cStateVar.getDataType().isComplex()) {
            assignTo_Im = cStateVar.getCVarName(true);
            if (transactionBlockSize == 1) {
                //For block size == 1, the dimension of the buffer is formed by taking the input dimensions and expanding by the block size (adding a dimension).  Reference
                assignTo_Im += "[" + insertPosition + "]";
            }
        }

        //--- Create for Loop if Elements are Vectors/Matricies ---
        std::string input_Re = input_ReUnindexed; //Modified below if indexing required
        std::string input_Im = input_ImUnindexed; //Modified below if indexing required
        std::vector <std::string> forLoopInputClose;
        if (!inputDT.isScalar()) {
            std::tuple <std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                    EmitterHelpers::generateVectorMatrixForLoops(inputDT.getDimensions());
            std::vector <std::string> forLoopInputOpen = std::get<0>(forLoopStrs);
            std::vector <std::string> forLoopInputIndexVars = std::get<1>(forLoopStrs);
            forLoopInputClose = std::get<2>(forLoopStrs);
            cStatementQueue.insert(cStatementQueue.end(), forLoopInputOpen.begin(), forLoopInputOpen.end());
            std::vector <std::string> forLoopSrcIndexVars = forLoopInputIndexVars; //We do not want to modify the index expression used when indexing into the src
            if (transactionBlockSize > 1) {
                //For block sizes > 1, the dimension of the buffer is formed by taking the input dimension and scaling the outer dimension (no extra dimension added)
                //The index is in terms of the
                forLoopInputIndexVars[0] = "(" + insertPosition + "+" + forLoopInputIndexVars[0] + ")";
            }

            assignTo_Re += EmitterHelpers::generateIndexOperation(forLoopInputIndexVars);
            input_Re = input_ReUnindexed + EmitterHelpers::generateIndexOperation(forLoopSrcIndexVars);
            if (cStateVar.getDataType().isComplex()) {
                assignTo_Im += EmitterHelpers::generateIndexOperation(forLoopInputIndexVars);
                input_Im = input_ImUnindexed + EmitterHelpers::generateIndexOperation(forLoopSrcIndexVars);
            }
        }

        //Make the assignment
        cStatementQueue.push_back(assignTo_Re + "=" + input_Re + ";");
        if (cStateVar.getDataType().isComplex()) {
            cStatementQueue.push_back(assignTo_Im + "=" + input_Im + ";");
        }

        //Close the for loop (if needed)
        if (!inputDT.isScalar()) {
            cStatementQueue.insert(cStatementQueue.end(), forLoopInputClose.begin(), forLoopInputClose.end());
        } else {
            if (transactionBlockSize > 1) {
                throw std::runtime_error(
                        ErrorHelpers::genErrorStr("For block sizes > 1, the input type should not be a scalar",
                                                  getSharedPointer()));
            }
        }
    }

    //==== Do the second assignment if applicable ====
    if(usesCircularBuffer() && circularBufferType != CircularBufferType::NO_EXTRA_LEN){
        //Find the index to insert into.  Note that index may not be valid for CircularBufferType::PLUS_DELAY_LEN_M1
        std::string insertPositionSecond = getSecondIndex(insertPosition);

        //If CircularBufferType::PLUS_DELAY_LEN_M1, a condition check is required before writing to second entry
        if(circularBufferType == CircularBufferType::PLUS_DELAY_LEN_M1){
            cStatementQueue.push_back(getSecondWriteCheck(insertPositionIn));
        }

        std::string assignTo_Re = cStateVar.getCVarName(false);
        if(transactionBlockSize == 1) {
            //For block size == 1, the dimension of the buffer is formed by taking the input dimensions and expanding by the block size (adding a dimension).  Reference
            assignTo_Re += "[" + insertPositionSecond + "]";
        }
        std::string assignTo_Im;
        if (cStateVar.getDataType().isComplex()) {
            assignTo_Im = cStateVar.getCVarName(true);
            if(transactionBlockSize == 1) {
                //For block size == 1, the dimension of the buffer is formed by taking the input dimensions and expanding by the block size (adding a dimension).  Reference
                assignTo_Im += "[" + insertPositionSecond + "]";
            }
        }

        //--- Create for Loop if Elements are Vectors/Matricies ---
        std::string input_Re = input_ReUnindexed; //Modified below if indexing required
        std::string input_Im = input_ImUnindexed; //Modified below if indexing required
        std::vector<std::string> forLoopInputClose;
        if (!inputDT.isScalar()) {
            std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                    EmitterHelpers::generateVectorMatrixForLoops(inputDT.getDimensions());
            std::vector<std::string> forLoopInputOpen = std::get<0>(forLoopStrs);
            std::vector<std::string> forLoopInputIndexVars = std::get<1>(forLoopStrs);
            forLoopInputClose = std::get<2>(forLoopStrs);
            cStatementQueue.insert(cStatementQueue.end(), forLoopInputOpen.begin(), forLoopInputOpen.end());
            std::vector<std::string> forLoopSrcIndexVars = forLoopInputIndexVars; //We do not want to modify the index expression used when indexing into the src
            if(transactionBlockSize > 1){
                //For block sizes > 1, the dimension of the buffer is formed by taking the input dimension and scaling the outer dimension (no extra dimension added)
                //The index is in terms of the
                forLoopInputIndexVars[0] = "(" + insertPositionSecond + "+" + forLoopInputIndexVars[0] + ")";
            }

            assignTo_Re += EmitterHelpers::generateIndexOperation(forLoopInputIndexVars);
            input_Re = input_ReUnindexed + EmitterHelpers::generateIndexOperation(forLoopSrcIndexVars);
            if (cStateVar.getDataType().isComplex()) {
                assignTo_Im += EmitterHelpers::generateIndexOperation(forLoopInputIndexVars);
                input_Im = input_ImUnindexed + EmitterHelpers::generateIndexOperation(forLoopSrcIndexVars);
            }
        }

        //Make the assignment
        cStatementQueue.push_back(assignTo_Re + "=" + input_Re + ";");
        if (cStateVar.getDataType().isComplex()) {
            cStatementQueue.push_back(assignTo_Im + "=" + input_Im + ";");
        }

        //Close the for loop (if needed)
        if (!inputDT.isScalar()) {
            cStatementQueue.insert(cStatementQueue.end(), forLoopInputClose.begin(), forLoopInputClose.end());
        }

        //Close condition
        if(circularBufferType == CircularBufferType::PLUS_DELAY_LEN_M1){
            cStatementQueue.push_back("}");
        }
    }
}

void Delay::copyToBuffer(CExpr src, std::string insertPosition, bool imag, std::vector<std::string> &cStatementQueue){
    DataType inputDT = getInputPort(0)->getDataType();

    if(copyMethod==CopyMethod::FOR_LOOPS) {
        std::string assignTo = cStateVar.getCVarName(imag);
        if(transactionBlockSize == 1) {
            //For block size == 1, the dimension of the buffer is formed by taking the input dimensions and expanding by the block size (adding a dimension).  Reference
            assignTo += "[" + insertPosition + "]";
        }

        //--- Create for Loop if Elements are Vectors/Matricies ---
        std::string srcStr;
        std::vector<std::string> forLoopInputClose;
        if (!inputDT.isScalar()) {
            std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> forLoopStrs =
                    EmitterHelpers::generateVectorMatrixForLoops(inputDT.getDimensions());
            std::vector<std::string> forLoopInputOpen = std::get<0>(forLoopStrs);
            std::vector<std::string> forLoopInputIndexVars = std::get<1>(forLoopStrs);
            forLoopInputClose = std::get<2>(forLoopStrs);
            cStatementQueue.insert(cStatementQueue.end(), forLoopInputOpen.begin(), forLoopInputOpen.end());
            std::vector<std::string> forLoopSrcIndexVars = forLoopInputIndexVars; //We do not want to modify the index expression used when indexing into the src

            if(transactionBlockSize > 1){
                //For block sizes > 1, the dimension of the buffer is formed by taking the input dimension and scaling the outer dimension (no extra dimension added)
                //The index is in terms of the
                forLoopInputIndexVars[0] = "(" + insertPosition + "+" + forLoopInputIndexVars[0] + ")";
            }
            assignTo += EmitterHelpers::generateIndexOperation(forLoopInputIndexVars);
            srcStr = src.getExprIndexed(forLoopSrcIndexVars, true);
        }else{
            if(transactionBlockSize>1){
                throw std::runtime_error(ErrorHelpers::genErrorStr("For block sizes > 1, the input type should not be a scalar", getSharedPointer()));
            }
            srcStr = src.getExpr();
        }

        //Make the assignment
        cStatementQueue.push_back(assignTo + "=" + srcStr + ";");

        //Close the for loop (if needed)
        if (!inputDT.isScalar()) {
            cStatementQueue.insert(cStatementQueue.end(), forLoopInputClose.begin(), forLoopInputClose.end());
        }
    }else{
        std::string dst = cStateVar.getCVarName(imag) + "+" + insertPosition;
        std::string srcStr = (inputDT.isScalar() ? "&" : "") + src.getExpr();
        int elementsToCopy = inputDT.numberOfElements();
        std::string copySize = "sizeof(" + cStateVar.getDataType().toString(DataType::StringStyle::C, false, false) + ")*" + GeneralHelper::to_string(elementsToCopy);

        if(copyMethod==CopyMethod::FAST_COPY_UNALIGNED) {
            cStatementQueue.push_back(
                    "fast_copy_unaligned_ramp_in(" + dst + ", " + srcStr +
                    ", " + copySize + ", 1);");
        }else if(copyMethod==CopyMethod::MEMCPY) {
            cStatementQueue.push_back(
                    "memcpy(" + dst + ", " + srcStr +
                    ", " + copySize + ");");
        }else if(copyMethod==CopyMethod::CLANG_MEMCPY_INLINE){
            cStatementQueue.push_back(
                    "__builtin_memcpy_inline(" + dst + ", " + srcStr +
                    ", " + copySize + ");");
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown Copy Type", getSharedPointer()));
        }
    }
}

void
Delay::assignInputToBuffer(CExpr src, std::string insertPositionIn, bool imag, std::vector<std::string> &cStatementQueue) {
    //TODO: need to add logic for double writing in extra length circular buffers
    //      Including adding an offset for recent last case

    DataType inputDT = getInputPort(0)->getDataType();

    //Note: for the circular buffer, the new item is placed at the cursor (with the given offset)
    //It used to be that there was an offset when earliestFirst was false and redundant writing (either DOUBLE_LEN,
    //or PLUS_DELAY_LEN_M1 were used).  Neither DOUBLE_LEN or PLUS_DELAY_LEN_M1 are allowed for standard delays.
    //Therefore, this modification should not effect standard delay operation.  Tapped delay overrides functions to
    //handle DOUBLE_LEN or PLUS_DELAY_LEN_M1 with !earliestFirst.
    std::string insertPosition = insertPositionIn;

    //==== Do the first assignment ====

    if (inputDT.isScalar() && transactionBlockSize>1) {
        throw std::runtime_error(ErrorHelpers::genErrorStr("For block sizes > 1, the input type should not be a scalar", getSharedPointer()));
    }

    copyToBuffer(src, insertPosition, imag, cStatementQueue);

    //==== Do the second assignment if applicable ====
    if(usesCircularBuffer() && circularBufferType != CircularBufferType::NO_EXTRA_LEN) {
        //Find the index to insert into.  Note that index may not be valid for CircularBufferType::PLUS_DELAY_LEN_M1
        std::string insertPositionSecond = getSecondIndex(insertPosition); //Same index calc should work for block size > 1

        //If CircularBufferType::PLUS_DELAY_LEN_M1, a condition check is required before writing to second entry
        if (circularBufferType == CircularBufferType::PLUS_DELAY_LEN_M1) {
            cStatementQueue.push_back(getSecondWriteCheck(insertPositionIn));
        }

        copyToBuffer(src, insertPositionSecond, imag, cStatementQueue);

        //Close condition
        if(circularBufferType == CircularBufferType::PLUS_DELAY_LEN_M1){
            cStatementQueue.push_back("}");
        }
    }
}

bool Delay::canBreakBlockingDependency(int localSubBlockingLength){
    return delayValue >= localSubBlockingLength;
}

void Delay::specializeForBlockingWOptions(bool processArcs,
                                          int localBlockingLength,
                                          int localSubBlockingLength,
                                          std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                          std::vector<std::shared_ptr<Node>> &nodesToRemove,
                                          std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                          std::vector<std::shared_ptr<Arc>> &arcsToRemove,
                                          std::vector<std::shared_ptr<Node>> &nodesToRemoveFromTopLevel,
                                          std::map<std::shared_ptr<Arc>, int> &arcsWithDeferredBlockingExpansion){
    //TODO: Refactor?
    if(localSubBlockingLength != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("When specializing for blocking, currently expect the sub-blocking length to be 1.  This is consistent with inserting sub-blocking domains ", getSharedPointer()));
    }

    int delayRemainder = delayValue%localBlockingLength;
    int separateDelay = delayValue/localBlockingLength;

    if(delayValue<localBlockingLength){
        //Just wrap the delay
        //Node::specializeForBlocking(localBlockingLength, localSubBlockingLength, nodesToAdd, nodesToRemove, arcsToAdd, arcsToRemove, nodesToRemoveFromTopLevel, arcsWithDeferredBlockingExpansion);

        if(delayValue > 0) {
            //Can actually use a delay, reconfigured for the specific configuration currently supported.
            //  blockSize > 1, circular buffer, double buffer len, earliestFirst = false;
            // Do not reverse the initial conditions since the initialCondition array should now always be oriented the same way.  It is re-shaped by getInitConditionsReshapedForConfig
            earliestFirst = false;
            transactionBlockSize = localBlockingLength; //This delay is not a multiple of the block size and was not converted to a ve3ctor delay
            bufferImplementation = BufferType::CIRCULAR_BUFFER;
            circularBufferType = CircularBufferType::DOUBLE_LEN;
        }//If the delay is 0, just leave as is

        //For the arcs into an out of the delay, mark them for dimension expansion by the local blocking length
        if(processArcs) {
            std::set<std::shared_ptr<Arc>> arcs = getDirectInputArcs();
            std::set<std::shared_ptr<Arc>> outArcs = getDirectOutputArcs();
            arcs.insert(outArcs.begin(), outArcs.end());
            for (const std::shared_ptr<Arc> arc: arcs) {
                arcsWithDeferredBlockingExpansion[arc] = localBlockingLength;
            }
        }
    }else {
        if (delayRemainder > 0) {
            //Create a new delay and move the remaining initial conditions to it
            std::shared_ptr<Delay> leftoverDelay = splitDelay(nodesToAdd, arcsToAdd, separateDelay*localBlockingLength);

            //Call the new delay's specialization function which will force wrapping.  This will set arcsWithDeferredBlockingExpansion for the arcs connected to the
            leftoverDelay->specializeForBlocking(localBlockingLength, localSubBlockingLength, nodesToAdd, nodesToRemove, arcsToAdd, arcsToRemove, nodesToRemoveFromTopLevel, arcsWithDeferredBlockingExpansion);
        }

        if (separateDelay > 0) {
            //If the delay had to be split, the delay length was already adjusted to separateDelay*localBlockingLength;
            //It now needs to be reduced to separateDelay with the arcs in and out expanded by localBlockingLength

            //Need this delay configured to not be earliest first (ie. last first) for indexing in the blocking domains
            //The delay should read from the front of the array, new values stored at the end
            earliestFirst = false;

            //Correct the initial conditions before changing the delay and datatype.  With !earliestFirst, initial conditions
            //are in the standard order with the first value to be displayed in initConditions[0]
            //When blocking, the resulting type will have values arranged in ascending order, which is the order that BlockingInput and BlockingOutput expect
            delayValue = separateDelay;

            //For the arcs into an out of the delay, mark them for dimension expansion by the local blocking length
            if(processArcs) {
                std::set<std::shared_ptr<Arc>> arcs = getDirectInputArcs();
                std::set<std::shared_ptr<Arc>> outArcs = getDirectOutputArcs();
                arcs.insert(outArcs.begin(), outArcs.end());
                for (const std::shared_ptr<Arc> arc: arcs) {
                    arcsWithDeferredBlockingExpansion[arc] = localBlockingLength;
                }
            }
        }
    }
}

void Delay::specializeForBlockingArcExpandOnly(int localBlockingLength, std::map<std::shared_ptr<Arc>, int> &arcsWithDeferredBlockingExpansion){
    std::set<std::shared_ptr<Arc>> arcs = getDirectInputArcs();
    std::set<std::shared_ptr<Arc>> outArcs = getDirectOutputArcs();
    arcs.insert(outArcs.begin(), outArcs.end());
    for (const std::shared_ptr<Arc> &arc: arcs) {
        arcsWithDeferredBlockingExpansion[arc] = localBlockingLength;
    }
}

void Delay::specializeForBlockingDeferDelayReconfigReshape(int localBlockingLength,
                                                           int localSubBlockingLength,
                                                           std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                                           std::vector<std::shared_ptr<Node>> &nodesToRemove,
                                                           std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                                           std::vector<std::shared_ptr<Arc>> &arcsToRemove,
                                                           std::vector<std::shared_ptr<Node>> &nodesToRemoveFromTopLevel,
                                                           std::map<std::shared_ptr<Arc>, int> &arcsWithDeferredBlockingExpansion){
    blockingSpecializationDeferred = true;
    deferredBlockSize=localBlockingLength;
    deferredSubBlockSize=localSubBlockingLength;

    specializeForBlockingArcExpandOnly(localBlockingLength, arcsWithDeferredBlockingExpansion);
}

void Delay::specializeForBlocking(int localBlockingLength,
                           int localSubBlockingLength,
                           std::vector<std::shared_ptr<Node>> &nodesToAdd,
                           std::vector<std::shared_ptr<Node>> &nodesToRemove,
                           std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                           std::vector<std::shared_ptr<Arc>> &arcsToRemove,
                           std::vector<std::shared_ptr<Node>> &nodesToRemoveFromTopLevel,
                           std::map<std::shared_ptr<Arc>, int> &arcsWithDeferredBlockingExpansion){
    if(blockingSpecializationDeferred){
        specializeForBlockingWOptions(false, //Already processed arcs.  Delay split should copy the arc types which were expanded
                deferredBlockSize,
                deferredSubBlockSize,
                nodesToAdd,
                nodesToRemove,
                arcsToAdd,
                arcsToRemove,
                nodesToRemoveFromTopLevel,
                arcsWithDeferredBlockingExpansion);
        blockingSpecializationDeferred = false;
    }else{
        specializeForBlockingWOptions(true, //Already processed arcs.  Delay split should copy the arc types which were expanded
                                      deferredBlockSize,
                                      deferredSubBlockSize,
                                      nodesToAdd,
                                      nodesToRemove,
                                      arcsToAdd,
                                      arcsToRemove,
                                      nodesToRemoveFromTopLevel,
                                      arcsWithDeferredBlockingExpansion);
    }

}

bool Delay::specializesForBlocking(){
    return true;
}

void Delay::propagatePropertiesHelper() {
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

    //Do not perform any modifications apart from expansion
}

int Delay::getBufferAllocatedLen() {
    //For now, we will assume that, for block sizes > 1, circular buffers are being used with double array
    //allocation.
    //TODO: Implement other variants
    if(transactionBlockSize > 1 && (!usesCircularBuffer() || circularBufferType != CircularBufferType::DOUBLE_LEN)){
        throw std::runtime_error(ErrorHelpers::genErrorStr("With Block Size > 1, only circular buffers with double buffer length are currently supported", shared_from_this()));
    }

    int stdBufferLen = getBufferLength();

    if(usesCircularBuffer()) {
        if (circularBufferType == CircularBufferType::DOUBLE_LEN) {
            return stdBufferLen * 2;
        } else if (circularBufferType == CircularBufferType::PLUS_DELAY_LEN_M1) {
            //TODO: Update for block size > 1 when additional variants supported
            int fifoLen = delayValue;
            if (allocateExtraSpace) {
                fifoLen++;
            }

            return stdBufferLen+fifoLen-1;
        }
        //TODO: Update for block size > 1 when additional variants supported
    }

    return stdBufferLen;
}

std::string Delay::getSecondIndex(std::string firstIndex) {
    std::string insertPositionSecond = "";
    if(earliestFirst){
        insertPositionSecond = firstIndex + "+" + GeneralHelper::to_string(getBufferLength()); //Need the buffer length without extra circular buffer elements
    }else{
        //This is actually correct for circular buffer because the second write is behind the current ptr
        //However, this mode was not used until recently and is not implemented for non-tapped delays since there is no need to provide consistent stride 1 access to multiple elements
        insertPositionSecond = firstIndex + "-" + GeneralHelper::to_string(getBufferLength()); //Need the buffer length without extra circular buffer elements
    }

    return insertPositionSecond;
}

std::string Delay::getSecondWriteCheck(std::string firstIndex) {
    int fifoLen = delayValue;
    if(allocateExtraSpace){
        fifoLen++;
    }

    if(earliestFirst){
        return "if(" + firstIndex + "<" + GeneralHelper::to_string(fifoLen-1) + "){"; //Need the orig insert position
    }else{
        //We only write when the first index is > than its initial position
        //Ie, we don't write in the case where the first index is is in its initial location because that value would
        //never be used.  However, all the other positions are used by the wraparound.
        return "if(" + firstIndex + ">" + GeneralHelper::to_string(getCircBufferInitialIdx()) + "){";
    }
}

std::shared_ptr<Delay>
Delay::splitDelay(std::vector<std::shared_ptr<Node>> &nodesToAdd, std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                  int targetDelayLength) {
    if(targetDelayLength > delayValue){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Cannot split delay when target delay is > than current delay", getSharedPointer()));
    }
    if(targetDelayLength == delayValue){
        //No splitting required.  Take no action
        return nullptr;
    }

    DataType dt = getInputPort(0)->getDataType();
    int subElementsPerElement = dt.numberOfElements();
    if(blockingSpecializationDeferred){
        subElementsPerElement /= deferredBlockSize; //The arcs going into the arc were already expanded by the local block size
    }

    //Split the delay
    int delayToMove = delayValue - targetDelayLength;

    //Create the new delay node
    std::shared_ptr<Delay> leftoverDelay = NodeFactory::shallowCloneNode<Delay>(parent, this); //Create a copy of this node (so that we get config for free)
    leftoverDelay->setId(-1); //Need to assign this node a new ID
    leftoverDelay->setName(leftoverDelay->getName() + "_splitLeftover");
    if(!context.empty()){
        leftoverDelay->setContext(context);
        context[context.size()-1].getContextRoot()->addSubContextNode(context[context.size()-1].getSubContext(), leftoverDelay);
    }
    nodesToAdd.push_back(leftoverDelay);

    //Configure this delay
    delayValue = targetDelayLength;
    //Reshape the initial conditions - remove the initial conditions that will be displayed later
    //Initial conditions are now always in the standard DSP notation order where initCond[0] is displayed first
    //So, remove from the back of the array (the later values)
    initCondition.erase(initCondition.begin()+targetDelayLength*subElementsPerElement, initCondition.end());

    //TODO: Configure the new delay node
    leftoverDelay->delayValue = delayToMove;
    //Keep the values at the end of the array
    //Remove from the front
    leftoverDelay->initCondition.erase(leftoverDelay->initCondition.begin(), leftoverDelay->initCondition.begin()+targetDelayLength*subElementsPerElement);

    //Wire the delays
    std::shared_ptr<Arc> origArc = *getInputPort(0)->getArcs().begin();
    origArc->setDstPortUpdateNewUpdatePrev(leftoverDelay->getInputPortCreateIfNot(0));
    std::shared_ptr<Arc> arc = Arc::connectNodes(leftoverDelay->getOutputPortCreateIfNot(0), getInputPort(0), origArc->getDataType(), origArc->getSampleTime());
    arcsToAdd.push_back(arc);

    return leftoverDelay;
}

std::vector<NumericValue> Delay::reverseInitConds(std::vector<NumericValue> &initConds) {
    //std::reverse works as long as the datatype is scalar
    //Reverse the order of the blocks but not the items in the blocks
    DataType dt = getInputPort(0)->getDataType();
    int subElementsPerElement = dt.numberOfElements();

    std::vector<NumericValue> reversedInitCond;

    for(int elementBase = initConds.size() - subElementsPerElement; elementBase >= 0; elementBase -= subElementsPerElement){
        for(int subElement = 0; subElement<subElementsPerElement; subElement++){
            reversedInitCond.push_back(initConds[elementBase + subElement]);
        }
    }

    return reversedInitCond;
}

std::vector<NumericValue> Delay::getInitConditionsReshapedForConfig() {
    std::vector<NumericValue> initCondReshaped = initCondition;

    if(allocateExtraSpace && delayValue > 0){ //allocateExtraSpace only has an effect if the delay value > 0
        //Need to add initial conditions for the extra elements
        //It does not really matter as this extra element is only used by TappedDelay to pass the current value
        int elements = getInputPort(0)->getDataType().numberOfElements()/transactionBlockSize; //Input port should be scaled for

        std::vector<NumericValue> extraPosInit;
        for(int i = 0; i<elements; i++){
            extraPosInit.push_back(initCondReshaped[i]);
        }

        if(earliestFirst){
            //Insert initial conditions at the front (this is where the current value would go)
            initCondReshaped.insert(initCondReshaped.begin(), extraPosInit.begin(), extraPosInit.end());
        }else{
            //Insert initial conditions at the end (this is where the current value would go)
            initCondReshaped.insert(initCondReshaped.end(), extraPosInit.begin(), extraPosInit.end());
        }
    }

    int origArrayLen = delayValue;
    if(allocateExtraSpace){
        origArrayLen++;
    }

    if(delayValue > 0 && usesCircularBuffer() && (roundCircularBufferToPowerOf2 || transactionBlockSize>1)){
        //May need to add additional initial conditions when rounded up to a power of 2
        int allocatedSize = getBufferLength()*getInputPort(0)->getDataType().numberOfElements();
        int initialCount = getInputPort(0)->getDataType().numberOfElements() * origArrayLen;
        if(transactionBlockSize > 1){
            //In this case, the input port was expanded by the block size.  The allocation is in terms of the sub-elements within the block
            allocatedSize /= transactionBlockSize;
            initialCount /= transactionBlockSize;
        }

        if (initialCount != initCondReshaped.size()) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "Unexpected initial condition size when adding new initial values for circular buffer",
                    getSharedPointer()));
        }

        //BUGFIX for initial conditions.
        //Assuming earliestFirst = false (value at index[0] is displayed first
        //For shift resisters, insert extras into the back of the buffer.  The true initial conditons should be at the front

        //HOWEVER, this logic is only executed for circular buffers
        //For circular buffers, insert extras into the front of the buffer.  Initial conditions should be at the end.
        //Will insert the extra elements at the front of the buffer.  Does not matter what they are since they will be overwritten
        //TODO: This should be refactored as it can be hard to decypher
        //      Standard delays do not execute this segment of the function unless it has a block size > 1 and is using circular buffers with double length buffering

        for (unsigned long i = initialCount; i < allocatedSize; i++) {
            initCondReshaped.insert(initCondReshaped.begin(), NumericValue((long) 0));
        }

    }

    if(delayValue > 0 && usesCircularBuffer() && circularBufferType == CircularBufferType::DOUBLE_LEN){
        //Replicate all init conds
        //Does not matter if earliest first or not, need 2 copies of the initial conditions
        int initCondOrigSizeOrig = initCondReshaped.size();
        for(unsigned long i = 0; i<initCondOrigSizeOrig; i++){
            initCondReshaped.push_back(initCondReshaped[i]);
        }
    }else if(delayValue > 0 && usesCircularBuffer() && circularBufferType == CircularBufferType::PLUS_DELAY_LEN_M1){
        //Replicate some init conds
        int addedElements = getInputPort(0)->getDataType().numberOfElements()*(origArrayLen-1);
        if(earliestFirst){
            //Extra space is at the end of the array.  Number of additional elements is delayLen-1 (+1 if allocate extra space)
            for(unsigned long i = 0; i<addedElements; i++){
                initCondReshaped.push_back(initCondReshaped[i]);
            }
        }else{
            //Extra space is at the beginning of the array
            std::vector<NumericValue> initCondTmp;
            for(unsigned long i = 0; i<addedElements; i++){
                initCondTmp.push_back(initCondReshaped[initCondReshaped.size()-1-addedElements+i]);
            }
            initCondTmp.insert(initCondTmp.end(), initCondReshaped.begin(), initCondReshaped.end());

            initCondReshaped = initCondTmp;
        }
    }

    //The specified order for initial conditions is that the value at index[0] will be displayed first.
    //This works directly if the oldest value (the one to be output next by the delay) is at index 0
    //In other words, it works when earliestFirst is not true.
    //If earliestFirst is true, the initial conditions should be flipped
    if(earliestFirst) {
        std::vector<NumericValue> reversedInitCond = reverseInitConds(initCondReshaped);
        initCondReshaped = reversedInitCond;
    }

    return initCondReshaped;
}

int Delay::getTransactionBlockSize() const {
    return transactionBlockSize;
}

void Delay::setTransactionBlockSize(int transactionBlockSize) {
    Delay::transactionBlockSize = transactionBlockSize;
}

bool Delay::isBlockingSpecializationDeferred() const {
    return blockingSpecializationDeferred;
}

void Delay::setBlockingSpecializationDeferred(bool blockingSpecializationDeferred) {
    Delay::blockingSpecializationDeferred = blockingSpecializationDeferred;
}

int Delay::getDeferredBlockSize() const {
    return deferredBlockSize;
}

void Delay::setDeferredBlockSize(int deferredBlockSize) {
    Delay::deferredBlockSize = deferredBlockSize;
}

int Delay::getDeferredSubBlockSize() const {
    return deferredSubBlockSize;
}

void Delay::setDeferredSubBlockSize(int deferredSubBlockSize) {
    Delay::deferredSubBlockSize = deferredSubBlockSize;
}

Delay::CopyMethod Delay::getCopyMethod() const {
    return copyMethod;
}

void Delay::setCopyMethod(Delay::CopyMethod copyMethod) {
    Delay::copyMethod = copyMethod;
}

std::set<std::string> Delay::getExternalIncludes() {
    std::set<std::string> extIncludes = Node::getExternalIncludes();

    if(copyMethod == CopyMethod::MEMCPY || copyMethod == CopyMethod::CLANG_MEMCPY_INLINE){
        extIncludes.insert("#include <string.h>");
    }
}
