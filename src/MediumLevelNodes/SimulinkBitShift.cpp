//
// Created by Christopher Yarp on 6/2/20.
//

#include "SimulinkBitShift.h"
#include "General/ErrorHelpers.h"
#include "PrimitiveNodes/BitwiseOperator.h"
#include "PrimitiveNodes/ReinterpretCast.h"
#include "PrimitiveNodes/Constant.h"

SimulinkBitShift::ShiftMode SimulinkBitShift::parseShiftMode(std::string str) {
    if(str == "SHIFT_LEFT_LOGICAL" || str == "Shift Left Logical"){
        return ShiftMode::SHIFT_LEFT_LOGICAL;
    }else if(str == "SHIFT_RIGHT_LOGICAL" || str == "Shift Right Logical"){
        return ShiftMode::SHIFT_RIGHT_LOGICAL;
    }else if(str == "SHIFT_RIGHT_ARITHMETIC" || str == "Shift Right Arithmetic"){
        return ShiftMode::SHIFT_RIGHT_ARITHMETIC;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown ShiftMode"));
    }
}

std::string SimulinkBitShift::shiftModeToString(SimulinkBitShift::ShiftMode mode) {
    if(mode == ShiftMode::SHIFT_LEFT_LOGICAL){
        return "SHIFT_LEFT_LOGICAL";
    }else if(mode == ShiftMode::SHIFT_RIGHT_LOGICAL){
        return "SHIFT_RIGHT_LOGICAL";
    }else if(mode == ShiftMode::SHIFT_RIGHT_ARITHMETIC){
        return "SHIFT_RIGHT_ARITHMETIC";
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown ShiftMode"));
    }
}

SimulinkBitShift::ShiftMode SimulinkBitShift::getShiftMode() const {
    return shiftMode;
}

void SimulinkBitShift::setShiftMode(SimulinkBitShift::ShiftMode shiftMode) {
    SimulinkBitShift::shiftMode = shiftMode;
}

int SimulinkBitShift::getShiftAmt() const {
    return shiftAmt;
}

void SimulinkBitShift::setShiftAmt(int shiftAmt) {
    SimulinkBitShift::shiftAmt = shiftAmt;
}

SimulinkBitShift::SimulinkBitShift() : shiftMode(ShiftMode::SHIFT_LEFT_LOGICAL), shiftAmt(0){

}

SimulinkBitShift::SimulinkBitShift(std::shared_ptr<SubSystem> parent) : MediumLevelNode(parent), shiftMode(ShiftMode::SHIFT_LEFT_LOGICAL), shiftAmt(0) {

}

SimulinkBitShift::SimulinkBitShift(std::shared_ptr<SubSystem> parent, SimulinkBitShift *orig) : MediumLevelNode(parent, orig), shiftMode(orig->shiftMode), shiftAmt(orig->shiftAmt) {

}

std::shared_ptr<SimulinkBitShift>
SimulinkBitShift::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                    std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node & Set Common Properties ====
    std::shared_ptr<SimulinkBitShift> newNode = NodeFactory::createNode<SimulinkBitShift>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    std::string shiftModeStr;
    std::string shiftAmtStr;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- ShiftMode, ShiftAmt
        shiftModeStr = dataKeyValueMap.at("ShiftMode");
        shiftAmtStr = dataKeyValueMap.at("ShiftAmt");
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Names -- mode, Numeric.N
        shiftModeStr = dataKeyValueMap.at("mode");
        shiftAmtStr = dataKeyValueMap.at("Numeric.N");
    } else {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - SimulinkBitShift", newNode));
    }

    newNode->shiftMode = parseShiftMode(shiftModeStr);
    newNode->shiftAmt = std::stoi(shiftAmtStr);

    return newNode;
}

std::set<GraphMLParameter> SimulinkBitShift::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("ShiftMode", "string", true));
    parameters.insert(GraphMLParameter("ShiftAmt", "int", true));

    return parameters;
}

xercesc::DOMElement *
SimulinkBitShift::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "SimulinkBitShift");

    GraphMLHelper::addDataNode(doc, thisNode, "ShiftMode", shiftModeToString(shiftMode));
    GraphMLHelper::addDataNode(doc, thisNode, "ShiftAmt", GeneralHelper::to_string(shiftAmt));

    return thisNode;
}

std::string SimulinkBitShift::typeNameStr() {
    return "SimulinkBitShift";
}

std::string SimulinkBitShift::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() +
             "\nShiftMode:" + shiftModeToString(shiftMode) +
             "\nShiftAmt:" + GeneralHelper::to_string(shiftAmt);

    return label;
}

std::shared_ptr<Node> SimulinkBitShift::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<SimulinkBitShift>(parent, this);
}

void SimulinkBitShift::validate() {
    Node::validate();

    if(inputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkBitShift - Should Have Exactly 1 Input Port", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkBitShift - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    DataType inputDT = getInputPort(0)->getDataType();
    DataType outputDT = getOutputPort(0)->getDataType();

    //TODO: Should type changes be allowed?  My thougt is that type expansion/contraction should be accomplished with casting
    if(inputDT != outputDT){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkBitShift - Input and Output Should Have Same Type", getSharedPointer()));
    }

    if(inputDT.isComplex()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkBitShift - Input Should be Real", getSharedPointer()));
    }

    if(inputDT.isFloatingPt()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkBitShift - Input Should be Integer", getSharedPointer()));
    }

    if(shiftAmt <0){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - SimulinkBitShift - Shift Amt should be >0 0", getSharedPointer()));
    }
}

std::shared_ptr<ExpandedNode> SimulinkBitShift::expand(std::vector<std::shared_ptr<Node>> &new_nodes,
                                                       std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                       std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                       std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                       std::shared_ptr<MasterUnconnected> &unconnected_master) {
    //Validate first to check that SimulinkBitShift is properly configured
    validate();

    //---- Expand the Gain Node to a multiply and constant ----
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

    //++++ Create BitwiseOperator ++++
    std::shared_ptr<BitwiseOperator> bitwiseOperatorNode = NodeFactory::createNode<BitwiseOperator>(expandedNode);
    bitwiseOperatorNode->setName("BitwiseOperator");
    if(shiftMode == ShiftMode::SHIFT_LEFT_LOGICAL){
        bitwiseOperatorNode->setOp(BitwiseOperator::BitwiseOp::SHIFT_LEFT);
    }else if(shiftMode == ShiftMode::SHIFT_RIGHT_ARITHMETIC || shiftMode == ShiftMode::SHIFT_RIGHT_LOGICAL){
        bitwiseOperatorNode->setOp(BitwiseOperator::BitwiseOp::SHIFT_RIGHT);
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown ShiftMode", getSharedPointer()));
    }
    new_nodes.push_back(bitwiseOperatorNode);

    //++++ Create constant and wire to bitwise operator second port ++++
    NumericValue shiftAmtVal = NumericValue((long) shiftAmt);
    //Verified that this value is >=0 so datatype can be unsigned
    int bits = shiftAmtVal.numIntegerBits();
    DataType shiftAmtDT = DataType(false, false, false, bits, 0, {1});
    shiftAmtDT = shiftAmtDT.getCPUStorageType(); //Do not want unessasary masking operation to occur

    std::shared_ptr<Constant> shiftAmtNode = NodeFactory::createNode<Constant>(expandedNode);
    shiftAmtNode->setName("ShiftAmt");
    shiftAmtNode->setValue({shiftAmtVal});
    new_nodes.push_back(shiftAmtNode);

    std::shared_ptr<Arc> shiftAmtArc = Arc::connectNodes(shiftAmtNode->getOutputPortCreateIfNot(0), bitwiseOperatorNode->getInputPortCreateIfNot(1), shiftAmtDT);
    new_arcs.push_back(shiftAmtArc);

    //++++ If Shift Logical and Signed, Reinterpret Cast to Unsigned then Back to Signed ++++
    //Already validated that input and output have same type, that the type is real, and that the type is integer
    DataType dataType = getInputPort(0)->getDataType();
    if(shiftMode == ShiftMode::SHIFT_RIGHT_LOGICAL && dataType.isSignedType()){
        //TODO: Reinterpret Cast to Unsigned then Back to Signed
        DataType unsignedType = dataType;
        unsignedType.setSignedType(false);

        //Create CastToUnsigned
        std::shared_ptr<ReinterpretCast> castToUnsignedNode = NodeFactory::createNode<ReinterpretCast>(expandedNode);
        castToUnsignedNode->setName("CastToUnsigned");
        castToUnsignedNode->setTgtDataType(unsignedType);
        new_nodes.push_back(castToUnsignedNode);

        //Connect CastToUnsigned to BitwiseOperator
        std::shared_ptr<Arc> castToUnsignedArc = Arc::connectNodes(castToUnsignedNode->getOutputPortCreateIfNot(0), bitwiseOperatorNode->getInputPortCreateIfNot(0), unsignedType);
        new_arcs.push_back(castToUnsignedArc);

        //Rewire Input
        std::set<std::shared_ptr<Arc>> inputArc = getInputPort(0)->getArcs();
        (*inputArc.begin())->setDstPortUpdateNewUpdatePrev(castToUnsignedNode->getInputPortCreateIfNot(0));

        //Create CastToSigned
        std::shared_ptr<ReinterpretCast> castToSignedNode = NodeFactory::createNode<ReinterpretCast>(expandedNode);
        castToSignedNode->setName("CastToSigned");
        castToSignedNode->setTgtDataType(dataType);
        new_nodes.push_back(castToSignedNode);

        //Wire BitwiseOperatorToCastToSigned
        std::shared_ptr<Arc> castToSignedArc = Arc::connectNodes(bitwiseOperatorNode->getOutputPortCreateIfNot(0), castToSignedNode->getInputPortCreateIfNot(0), unsignedType);
        new_arcs.push_back(castToSignedArc);

        //Rewire Outputs
        std::set<std::shared_ptr<Arc>> outputArcs = getOutputPort(0)->getArcs();
        for(const auto & outputArc : outputArcs){
            outputArc->setSrcPortUpdateNewUpdatePrev(castToSignedNode->getOutputPortCreateIfNot(0));
        }

        //Rewire order constraint arcs to Casts
        std::set<std::shared_ptr<Arc>> orderConstraintInputArcs = getOrderConstraintInputArcs();
        for(const auto & orderConstraintInputArc : orderConstraintInputArcs){
            orderConstraintInputArc->setDstPortUpdateNewUpdatePrev(castToUnsignedNode->getOrderConstraintInputPortCreateIfNot());
        }

        std::set<std::shared_ptr<Arc>> orderConstraintOutputArcs = getOrderConstraintOutputArcs();
        for(const auto & orderConstraintOutputArc : orderConstraintOutputArcs){
            orderConstraintOutputArc->setSrcPortUpdateNewUpdatePrev(castToSignedNode->getOrderConstraintOutputPortCreateIfNot());
        }
    }else{
        //++++ Else Wire directly to bitwise operator ++++
        //Rewire input arc (validated to be a single arc)
        std::set<std::shared_ptr<Arc>> inputArc = getInputPort(0)->getArcs();
        (*inputArc.begin())->setDstPortUpdateNewUpdatePrev(bitwiseOperatorNode->getInputPortCreateIfNot(0));

        //Rewire output arcs
        std::set<std::shared_ptr<Arc>> outputArcs = getOutputPort(0)->getArcs();
        for(const auto & outputArc : outputArcs){
            outputArc->setSrcPortUpdateNewUpdatePrev(bitwiseOperatorNode->getOutputPortCreateIfNot(0));
        }

        //Rewire order constraint arc to bitwise operator
        std::set<std::shared_ptr<Arc>> orderConstraintInputArcs = getOrderConstraintInputArcs();
        for(const auto & orderConstraintInputArc : orderConstraintInputArcs){
            orderConstraintInputArc->setDstPortUpdateNewUpdatePrev(bitwiseOperatorNode->getOrderConstraintInputPortCreateIfNot());
        }

        std::set<std::shared_ptr<Arc>> orderConstraintOutputArcs = getOrderConstraintOutputArcs();
        for(const auto & orderConstraintOutputArc : orderConstraintOutputArcs){
            orderConstraintOutputArc->setSrcPortUpdateNewUpdatePrev(bitwiseOperatorNode->getOrderConstraintOutputPortCreateIfNot());
        }
    }

    return expandedNode;
}


