//
// Created by Christopher Yarp on 2019-02-18.
//

#include <General/GeneralHelper.h>
#include "NCO.h"

#include "PrimitiveNodes/Sum.h"
#include "PrimitiveNodes/BitwiseOperator.h"
#include "PrimitiveNodes/ReinterpretCast.h"
#include "PrimitiveNodes/Mux.h"
#include "PrimitiveNodes/LUT.h"
#include "PrimitiveNodes/Delay.h"
#include "PrimitiveNodes/Constant.h"
#include "PrimitiveNodes/RealImagToComplex.h"
#include "PrimitiveNodes/DataTypeConversion.h"
#include "MediumLevelNodes/Gain.h"
#include "General/ErrorHelpers.h"

#include <cmath>

NCO::NCO() : MediumLevelNode(), lutAddrBits(0), accumulatorBits(0), ditherBits(0), complexOut(false) {

}

NCO::NCO(std::shared_ptr<SubSystem> parent) : MediumLevelNode(parent), lutAddrBits(0), accumulatorBits(0), ditherBits(0), complexOut(false) {

}

NCO::NCO(std::shared_ptr<SubSystem> parent, NCO *orig) : MediumLevelNode(parent, orig), lutAddrBits(orig->lutAddrBits), accumulatorBits(orig->accumulatorBits), ditherBits(orig->ditherBits), complexOut(orig->complexOut) {

}

int NCO::getLutAddrBits() const {
    return lutAddrBits;
}

void NCO::setLutAddrBits(int lutAddrBits) {
    NCO::lutAddrBits = lutAddrBits;
}

int NCO::getAccumulatorBits() const {
    return accumulatorBits;
}

void NCO::setAccumulatorBits(int accumulatorBits) {
    NCO::accumulatorBits = accumulatorBits;
}

int NCO::getDitherBits() const {
    return ditherBits;
}

void NCO::setDitherBits(int ditherBits) {
    NCO::ditherBits = ditherBits;
}

bool NCO::isComplexOut() const {
    return complexOut;
}

void NCO::setComplexOut(bool complexOut) {
    NCO::complexOut = complexOut;
}

std::shared_ptr<NCO>
NCO::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                       std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node & Set Common Properties ====
    std::shared_ptr<NCO> newNode = NodeFactory::createNode<NCO>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    std::string lutAddrBitsStr;
    std::string accumulatorBitsStr;
    std::string ditherBitsStr;
    bool complexOutParsed;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- LutAddrBits, AccumulatorBits, DitherBits, ComplexOut
        lutAddrBitsStr = dataKeyValueMap.at("LutAddrBits");
        accumulatorBitsStr = dataKeyValueMap.at("AccumulatorBits");
        ditherBitsStr = dataKeyValueMap.at("DitherBits");
        std::string complexOutStr = dataKeyValueMap.at("ComplexOut");
        if(complexOutStr == "0" || complexOutStr == "false"){
            complexOutParsed = false;
        }else{
            complexOutParsed = true;
        }
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Names -- Numeric.TableDepth, Numeric.AccumWL, Numeric.DitherWL, Formula
        lutAddrBitsStr = dataKeyValueMap.at("Numeric.TableDepth");
        accumulatorBitsStr = dataKeyValueMap.at("Numeric.AccumWL");
        if(dataKeyValueMap.at("HasDither") == "off"){
            ditherBitsStr = "0";
        }else {
            ditherBitsStr = dataKeyValueMap.at("Numeric.DitherWL");
        }
        std::string formula = dataKeyValueMap.at("Formula");
        if(formula == "Complex exponential"){
            complexOutParsed = true;
        }else if(formula == "Cosine"){
            complexOutParsed = false;
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("NCO Type: " + formula + " is not supported yet - NCO", newNode));
        }
    } else
    {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - NCO", newNode));
    }

    std::vector<NumericValue> lutAddrBitsNV = NumericValue::parseXMLString(lutAddrBitsStr);
    std::vector<NumericValue> accumulatorBitsNV = NumericValue::parseXMLString(accumulatorBitsStr);
    std::vector<NumericValue> ditherBitsNV = NumericValue::parseXMLString(ditherBitsStr);

    if(lutAddrBitsNV.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Error Parsing LutAddrBits - NCO", newNode));
    }
    if(lutAddrBitsNV[0].isComplex() || lutAddrBitsNV[0].isFractional()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Error Parsing LutAddrBits - NCO", newNode));
    }
    newNode->setLutAddrBits(lutAddrBitsNV[0].getRealInt());

    if(accumulatorBitsNV.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Error Parsing AccumulatorBits - NCO", newNode));
    }
    if(accumulatorBitsNV[0].isComplex() || accumulatorBitsNV[0].isFractional()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Error Parsing AccumulatorBits - NCO", newNode));
    }
    newNode->setAccumulatorBits(accumulatorBitsNV[0].getRealInt());

    if(ditherBitsNV.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Error Parsing DitherBits - NCO", newNode));
    }
    if(ditherBitsNV[0].isComplex() || ditherBitsNV[0].isFractional()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Error Parsing DitherBits - NCO", newNode));
    }
    newNode->setDitherBits(ditherBitsNV[0].getRealInt());

    newNode->setComplexOut(complexOutParsed);

    return newNode;
}

std::set<GraphMLParameter> NCO::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("LutAddrBits", "string", true));
    parameters.insert(GraphMLParameter("AccumulatorBits", "string", true));
    parameters.insert(GraphMLParameter("DitherBits", "string", true));
    parameters.insert(GraphMLParameter("ComplexOut", "boolean", true));

    return parameters;
}

std::string NCO::typeNameStr(){
    return "NCO";
}

std::string NCO::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() + "\nLutAddrBits:" + GeneralHelper::to_string(lutAddrBits) +
             "\nAccumulatorBits:" + GeneralHelper::to_string(accumulatorBits) +
             "\nDitherBits:" + GeneralHelper::to_string(ditherBits) +
             "\nComplexOut:" + GeneralHelper::to_string(complexOut);

    return label;
}

xercesc::DOMElement *
NCO::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "NCO");

    GraphMLHelper::addDataNode(doc, thisNode, "LutAddrBits", GeneralHelper::to_string(lutAddrBits));
    GraphMLHelper::addDataNode(doc, thisNode, "AccumulatorBits", GeneralHelper::to_string(accumulatorBits));
    GraphMLHelper::addDataNode(doc, thisNode, "DitherBits", GeneralHelper::to_string(ditherBits));
    GraphMLHelper::addDataNode(doc, thisNode, "ComplexOut", GeneralHelper::to_string(complexOut));

    return thisNode;
}

void NCO::validate() {
    Node::validate();

    if(inputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - NCO - Should Have Exactly 1 Input Port", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - NCO - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    if(accumulatorBits <= lutAddrBits){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - NCO - AccumulatorBits should be larger than LUTAddrBits", getSharedPointer()));
    }

    //TODO: handle dithering
    if(ditherBits != 0){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - NCO - Dithering is currently not supported", getSharedPointer()));
    }
}

std::shared_ptr<Node> NCO::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<NCO>(parent, this);
}

std::shared_ptr<ExpandedNode> NCO::expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                          std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                          std::shared_ptr<MasterUnconnected> &unconnected_master) {
    //Validate first to check that node is properly wired (ie. there is the proper number of ports, only 1 input arc, etc.)
    validate();

    //TODO enable sin output
    bool calcCos = true;
    bool calcSin = complexOut;

    //---- Create the expanded node ----
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

    //---- Create nodes and rewire ----
    //DataType incDT = getInputPort(0)->getDataType();
    DataType incDT(false, true, false, accumulatorBits+1, 0, 1); //Accumulator bits
    //TODO: For this implementation, we round up to the next CPU type.  Comment out if true fixed point is needed
    incDT = incDT.getCPUStorageType();

    std::shared_ptr<DataTypeConversion> inputDTConvert = NodeFactory::createNode<DataTypeConversion>(expandedNode);
    inputDTConvert->setInheritType(DataTypeConversion::InheritType::INHERIT_FROM_OUTPUT);
    inputDTConvert->setName("NCO_Input_DT_Convert");
    new_nodes.push_back(inputDTConvert);

    //Rewire input
    inputDTConvert->addInArcUpdatePrevUpdateArc(0, *(getInputPort(0)->getArcs().begin()));

    //+ Delay +
    std::shared_ptr<Delay> delay = NodeFactory::createNode<Delay>(expandedNode);
    delay->setDelayValue(1);
    delay->setInitCondition({NumericValue(0, 0, std::complex<double>(0, 0), false, false)}); //TODO: implement initial phase
    delay->setName("NCO_Accumulator_State");
    new_nodes.push_back(delay);

    //+++ Accum +++
    std::shared_ptr<Sum> accum = NodeFactory::createNode<Sum>(expandedNode);
    accum->setInputSign(std::vector<bool>({true, true}));
    accum->setName("NCO_Accumulator_Sum");
    new_nodes.push_back(accum);

    std::shared_ptr<Arc> dataTypeConvertInToAccum = Arc::connectNodes(inputDTConvert, 0, accum, 0, incDT);
    new_arcs.push_back(dataTypeConvertInToAccum);

    std::shared_ptr<Arc> delayToSum = Arc::connectNodes(delay, 0, accum, 1, incDT);
    new_arcs.push_back(delayToSum);

    //+Accum Mask+
    unsigned long maskVal = GeneralHelper::twoPow(accumulatorBits)-1;

    std::shared_ptr<Constant> accumMaskConst = NodeFactory::createNode<Constant>(expandedNode);
    accumMaskConst->setValue({NumericValue(maskVal, 0, std::complex<double>(0,0), false, false)});
    new_nodes.push_back(accumMaskConst);

    std::shared_ptr<BitwiseOperator> accumMask = NodeFactory::createNode<BitwiseOperator>(expandedNode);
    accumMask->setOp(BitwiseOperator::BitwiseOp::AND);
    accumMask->setName("NCO_Accum_Mask");
    new_nodes.push_back(accumMask);

    std::shared_ptr<Arc> sumToAccumMask = Arc::connectNodes(accum, 0, accumMask, 0, incDT);
    new_arcs.push_back(sumToAccumMask);

    std::shared_ptr<Arc> constantToAccumMask = Arc::connectNodes(accumMaskConst, 0, accumMask, 1, incDT);
    new_arcs.push_back(constantToAccumMask);

    std::shared_ptr<Arc> accumMaskToDelay = Arc::connectNodes(accumMask, 0, delay, 0, incDT);
    new_arcs.push_back(accumMaskToDelay);

    //+++ Treat as Unsigned +++
    DataType unsignedIncDT = incDT;
    unsignedIncDT.setSignedType(false);

    std::shared_ptr<ReinterpretCast> reinterpretCast = NodeFactory::createNode<ReinterpretCast>(expandedNode);
    reinterpretCast->setTgtDataType(unsignedIncDT);
    new_nodes.push_back(reinterpretCast);

    std::shared_ptr<Arc> accumMaskToReinterpretCast = Arc::connectNodes(delay, 0, reinterpretCast, 0, incDT);
    new_arcs.push_back(accumMaskToReinterpretCast);

    //+++ Quantize +++
    std::shared_ptr<BitwiseOperator> quantizer = NodeFactory::createNode<BitwiseOperator>(expandedNode);
    quantizer->setOp(BitwiseOperator::BitwiseOp::SHIFT_RIGHT);
    new_nodes.push_back(quantizer);

    std::shared_ptr<Constant> quantizeConst = NodeFactory::createNode<Constant>(expandedNode);
    quantizeConst->setValue({NumericValue(accumulatorBits-lutAddrBits, 0, std::complex<double>(0, 0), false, false)});
    new_nodes.push_back(quantizeConst);

    std::shared_ptr<Arc> reinterpretCastToQuantizer = Arc::connectNodes(reinterpretCast, 0, quantizer, 0, unsignedIncDT);
    new_arcs.push_back(reinterpretCastToQuantizer);

    DataType quantizeConstType(false, false, false, ceil(log2(accumulatorBits+1)), 0, 1);
    std::shared_ptr<Arc> quantizeConstToQuantizer = Arc::connectNodes(quantizeConst, 0, quantizer, 1, quantizeConstType);
    new_arcs.push_back(quantizeConstToQuantizer);

    DataType quantizerType = unsignedIncDT;
    quantizerType.setTotalBits(lutAddrBits); //The accumulator is quantized to
    //TODO: For this implementation, we round up to the next CPU type.  Comment out if true fixed point is needed
    quantizerType = quantizerType.getCPUStorageType();

    //Insert another datatype convert
    std::shared_ptr<DataTypeConversion> quantizeDTConvert = NodeFactory::createNode<DataTypeConversion>(expandedNode);
    quantizeDTConvert->setInheritType(DataTypeConversion::InheritType::INHERIT_FROM_OUTPUT);
    new_nodes.push_back(quantizeDTConvert);

    std::shared_ptr<Arc> quantizerToDTConvert = Arc::connectNodes(quantizer, 0, quantizeDTConvert, 0, unsignedIncDT);
    new_arcs.push_back(quantizerToDTConvert);

    //+++ Quad and Base Index Calculation +++
    //+Quad Calc+
    std::shared_ptr<BitwiseOperator> quadCalc = NodeFactory::createNode<BitwiseOperator>(expandedNode);
    quadCalc->setOp(BitwiseOperator::BitwiseOp::SHIFT_RIGHT);
    new_nodes.push_back(quadCalc);

    std::shared_ptr<Constant> quadCalcConst = NodeFactory::createNode<Constant>(expandedNode);
    quadCalcConst->setValue({NumericValue(lutAddrBits-2, 0, std::complex<double>(0, 0), false, false)});
    new_nodes.push_back(quadCalcConst);

    std::shared_ptr<Arc> quantizerToQuadCalc = Arc::connectNodes(quantizeDTConvert, 0, quadCalc, 0, quantizerType);
    new_arcs.push_back(quantizerToQuadCalc);

    DataType quantizeQuadConstType(false, false, false, ceil(log2(lutAddrBits-2)), 0, 1);

    std::shared_ptr<Arc> quadCalcConstToQuadCalc = Arc::connectNodes(quadCalcConst, 0, quadCalc, 1, quantizeQuadConstType);
    new_arcs.push_back(quadCalcConstToQuadCalc);

    DataType quadDT = DataType(false, false, false, 2, 0, 1);

    //+Base Ind Calc+
    std::shared_ptr<BitwiseOperator> baseIndCalc = NodeFactory::createNode<BitwiseOperator>(expandedNode);
    baseIndCalc->setOp(BitwiseOperator::BitwiseOp::AND);
    new_nodes.push_back(baseIndCalc);

    std::shared_ptr<Constant> baseIndMask = NodeFactory::createNode<Constant>(expandedNode);
    baseIndMask->setValue({NumericValue(GeneralHelper::twoPow(lutAddrBits-2)-1, 0, std::complex<double>(0, 0), false, false)});
    new_nodes.push_back(baseIndMask);

    std::shared_ptr<Arc> quantizerToBaseIndCalc = Arc::connectNodes(quantizer, 0, baseIndCalc, 0, quantizerType);
    new_arcs.push_back(quantizerToBaseIndCalc);

    std::shared_ptr<Arc> baseIndMaskToBaseIndCalc = Arc::connectNodes(baseIndMask, 0, baseIndCalc, 1, quantizerType);
    new_arcs.push_back(baseIndMaskToBaseIndCalc);

    DataType baseIndDT = DataType(false, false, false, lutAddrBits-1, 0, 1); //This should be -2 but an additional bit is needed due to the extra entry in the LUT (boundary)

    //+++ Cos & Sin Index Calculation +++
    std::shared_ptr<Constant> indOffsetConst = NodeFactory::createNode<Constant>(expandedNode);
    indOffsetConst->setValue({NumericValue(GeneralHelper::twoPow(lutAddrBits-2), 0, std::complex<double>(0, 0), false, false)});
    new_nodes.push_back(indOffsetConst);

    std::shared_ptr<Sum> baseIndSum = NodeFactory::createNode<Sum>(expandedNode);
    baseIndSum->setInputSign({true, false});
    new_nodes.push_back(baseIndSum);

    std::shared_ptr<Arc> indOffsetConstToBaseInSum = Arc::connectNodes(indOffsetConst, 0, baseIndSum, 0, baseIndDT);
    new_arcs.push_back(indOffsetConstToBaseInSum);

    std::shared_ptr<Arc> baseIndCalcToBaseInSum = Arc::connectNodes(baseIndCalc, 0, baseIndSum, 1, baseIndDT);
    new_arcs.push_back(baseIndCalcToBaseInSum);

    std::shared_ptr<Mux> cosIndMux;
    std::shared_ptr<Mux> sinIndMux;

    if(calcCos){
        cosIndMux = NodeFactory::createNode<Mux>(expandedNode);
        cosIndMux->setBooleanSelect(false);
        cosIndMux->setUseSwitch(true); //TODO: Check this
        new_nodes.push_back(cosIndMux);

        //Connect the select line
        std::shared_ptr<Arc> quadToCosIndMuxSelect = Arc::connectNodes(quadCalc, 0, cosIndMux, quadDT);
        new_arcs.push_back(quadToCosIndMuxSelect);

        //Connect the data lines
        std::shared_ptr<Arc> cosIndMux0 = Arc::connectNodes(baseIndCalc, 0, cosIndMux, 0, baseIndDT);
        new_arcs.push_back(cosIndMux0);

        std::shared_ptr<Arc> cosIndMux1 = Arc::connectNodes(baseIndSum, 0, cosIndMux, 1, baseIndDT);
        new_arcs.push_back(cosIndMux1);

        std::shared_ptr<Arc> cosIndMux2 = Arc::connectNodes(baseIndCalc, 0, cosIndMux, 2, baseIndDT);
        new_arcs.push_back(cosIndMux2);

        std::shared_ptr<Arc> cosIndMux3 = Arc::connectNodes(baseIndSum, 0, cosIndMux, 3, baseIndDT);
        new_arcs.push_back(cosIndMux3);
    }

    if(calcSin){
        sinIndMux = NodeFactory::createNode<Mux>(expandedNode);
        sinIndMux->setBooleanSelect(false);
        sinIndMux->setUseSwitch(true); //TODO: Check this
        new_nodes.push_back(sinIndMux);

        //Connect the select line
        std::shared_ptr<Arc> quadToSinIndMuxSelect = Arc::connectNodes(quadCalc, 0, sinIndMux, quadDT);
        new_arcs.push_back(quadToSinIndMuxSelect);

        //Connect the data lines
        std::shared_ptr<Arc> sinIndMux0 = Arc::connectNodes(baseIndSum, 0, sinIndMux, 0, baseIndDT);
        new_arcs.push_back(sinIndMux0);

        std::shared_ptr<Arc> sinIndMux1 = Arc::connectNodes(baseIndCalc, 0, sinIndMux, 1, baseIndDT);
        new_arcs.push_back(sinIndMux1);

        std::shared_ptr<Arc> sinIndMux2 = Arc::connectNodes(baseIndSum, 0, sinIndMux, 2, baseIndDT);
        new_arcs.push_back(sinIndMux2);

        std::shared_ptr<Arc> sinIndMux3 = Arc::connectNodes(baseIndCalc, 0, sinIndMux, 3, baseIndDT);
        new_arcs.push_back(sinIndMux3);
    }

    //+++ LUT +++
    std::shared_ptr<LUT> quarterWaveLut = NodeFactory::createNode<LUT>(expandedNode);
    new_nodes.push_back(quarterWaveLut);

    //Compute the LUT entries
    std::vector<NumericValue> breakpoints;
    std::vector<NumericValue> tableData;

    unsigned long lutEntries = GeneralHelper::twoPow(lutAddrBits-2)+1; //Note, the table ends at the value of 2^(lutAddrBits-2)
    for(unsigned long i = 0; i<lutEntries; i++){
        breakpoints.push_back(NumericValue(i, 0, std::complex<double>(0, 0), false, false));

        double tableDataPt = cos((M_PI*((double) i))/((double) GeneralHelper::twoPow(lutAddrBits-1))); //TODO: probably don't need all these cases, just got paranoid'
        tableData.push_back(NumericValue(0, 0, std::complex<double>(tableDataPt, 0), false, true));
    }

    std::vector<std::vector<NumericValue>> breakpoint2D;
    breakpoint2D.push_back(breakpoints);

    quarterWaveLut->setBreakpoints(breakpoint2D);
    quarterWaveLut->setTableData(tableData);
    quarterWaveLut->setSearchMethod(LUT::SearchMethod::EVENLY_SPACED_POINTS);
    quarterWaveLut->setInterpMethod(LUT::InterpMethod::FLAT);
    quarterWaveLut->setExtrapMethod(LUT::ExtrapMethod::NO_CHECK);

    if(calcCos){
        std::shared_ptr<Arc> cosIndToLUT = Arc::connectNodes(cosIndMux, 0, quarterWaveLut, 0, baseIndDT);
        new_arcs.push_back(cosIndToLUT);
    }

    if(calcSin && calcCos){
        std::shared_ptr<Arc> sinIndToLUT = Arc::connectNodes(sinIndMux, 0, quarterWaveLut, 1, baseIndDT);
        new_arcs.push_back(sinIndToLUT);
    }else{
        std::shared_ptr<Arc> sinIndToLUT = Arc::connectNodes(sinIndMux, 0, quarterWaveLut, 0, baseIndDT);
        new_arcs.push_back(sinIndToLUT);
    }

    //+++ Cos & Sin Sign Adjust +++
    DataType outputDT = getOutputPort(0)->getDataType();
    outputDT.setComplex(false);

    std::shared_ptr<Mux> cosSignMux;
    std::shared_ptr<Mux> sinSignMux;

    //cosPortInd is always 0
    if(calcCos){
        std::shared_ptr<Gain> cosNegate = NodeFactory::createNode<Gain>(expandedNode);
        cosNegate->setGain({NumericValue(0, 0, std::complex<double>(-1, 0), false, true)});
        new_nodes.push_back(cosNegate);

        std::shared_ptr<Arc> cosToCosNegate = Arc::connectNodes(quarterWaveLut, 0, cosNegate, 0, outputDT);
        new_arcs.push_back(cosToCosNegate);

        cosSignMux = NodeFactory::createNode<Mux>(expandedNode);
        cosSignMux->setBooleanSelect(false);
        cosSignMux->setUseSwitch(true); //TODO: check
        new_nodes.push_back(cosSignMux);

        //Connect the select lines
        std::shared_ptr<Arc> quadToCosSignMuxSelect = Arc::connectNodes(quadCalc, 0, cosSignMux, quadDT);
        new_arcs.push_back(quadToCosSignMuxSelect);

        //Connect the data lines
        std::shared_ptr<Arc> cosSignMux0 = Arc::connectNodes(quarterWaveLut, 0, cosSignMux, 0, outputDT);
        new_arcs.push_back(cosSignMux0);

        std::shared_ptr<Arc> cosSignMux1 = Arc::connectNodes(cosNegate, 0, cosSignMux, 1, outputDT);
        new_arcs.push_back(cosSignMux1);

        std::shared_ptr<Arc> cosSignMux2 = Arc::connectNodes(cosNegate, 0, cosSignMux, 2, outputDT);
        new_arcs.push_back(cosSignMux2);

        std::shared_ptr<Arc> cosSignMux3 = Arc::connectNodes(quarterWaveLut, 0, cosSignMux, 3, outputDT);
        new_arcs.push_back(cosSignMux3);
    }

    int sinPortInd = (calcCos) ? 1 : 0;

    if(calcSin){
        std::shared_ptr<Gain> sinNegate = NodeFactory::createNode<Gain>(expandedNode);
        sinNegate->setGain({NumericValue(0, 0, std::complex<double>(-1, 0), false, true)});
        new_nodes.push_back(sinNegate);

        std::shared_ptr<Arc> sinToCosNegate = Arc::connectNodes(quarterWaveLut, sinPortInd, sinNegate, 0, outputDT);
        new_arcs.push_back(sinToCosNegate);

        sinSignMux = NodeFactory::createNode<Mux>(expandedNode);
        sinSignMux->setBooleanSelect(false);
        sinSignMux->setUseSwitch(true); //TODO: check
        new_nodes.push_back(sinSignMux);

        //Connect the select lines
        std::shared_ptr<Arc> quadToSinSignMuxSelect = Arc::connectNodes(quadCalc, 0, sinSignMux, quadDT);
        new_arcs.push_back(quadToSinSignMuxSelect);

        //Connect the data lines
        std::shared_ptr<Arc> sinSignMux0 = Arc::connectNodes(quarterWaveLut, sinPortInd, sinSignMux, 0, outputDT);
        new_arcs.push_back(sinSignMux0);

        std::shared_ptr<Arc> sinSignMux1 = Arc::connectNodes(quarterWaveLut, sinPortInd, sinSignMux, 1, outputDT);
        new_arcs.push_back(sinSignMux1);

        std::shared_ptr<Arc> sinSignMux2 = Arc::connectNodes(sinNegate, 0, sinSignMux, 2, outputDT);
        new_arcs.push_back(sinSignMux2);

        std::shared_ptr<Arc> sinSignMux3 = Arc::connectNodes(sinNegate, 0, sinSignMux, 3, outputDT);
        new_arcs.push_back(sinSignMux3);
    }

    //+++ Recombine (If Necessary) & Rewire +++
    std::shared_ptr<Node> lastNode;

    if(calcCos && calcSin){
        std::shared_ptr<RealImagToComplex> realImagToComplex = NodeFactory::createNode<RealImagToComplex>(expandedNode);
        new_nodes.push_back(realImagToComplex);

        std::shared_ptr<Arc> cosSignMuxToRealImag = Arc::connectNodes(cosSignMux, 0, realImagToComplex, 0, outputDT);
        new_arcs.push_back(cosSignMuxToRealImag);

        std::shared_ptr<Arc> sinSignMuxToRealImag = Arc::connectNodes(sinSignMux, 0, realImagToComplex, 1, outputDT);
        new_arcs.push_back(sinSignMuxToRealImag);

        lastNode = realImagToComplex;
    }else{
        //Just rewire to the appropriate sign LUT
        if(calcCos){
            lastNode = cosSignMux;
        }else{
            //Calc Sin
            lastNode = sinSignMux;
        }
    }

    //Rewire outputs
    std::set<std::shared_ptr<Arc>> outArcs = getOutputPort(0)->getArcs();

    for(auto outArcIt = outArcs.begin(); outArcIt != outArcs.end(); outArcIt++){
        (*outArcIt)->setSrcPortUpdateNewUpdatePrev(lastNode->getOutputPortCreateIfNot(0));
    }

    return expandedNode;
}
