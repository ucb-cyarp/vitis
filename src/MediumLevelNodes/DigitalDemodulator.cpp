//
// Created by Christopher Yarp on 2019-02-28.
//

#include "DigitalDemodulator.h"

#include "General/GeneralHelper.h"
#include "General/DSPHelpers.h"

#include "PrimitiveNodes/LUT.h"
#include "PrimitiveNodes/Mux.h"
#include "PrimitiveNodes/ComplexToRealImag.h"
#include "PrimitiveNodes/Constant.h"
#include "MediumLevelNodes/Gain.h"
#include "MediumLevelNodes/Saturate.h"
#include "PrimitiveNodes/DataTypeConversion.h"
#include "PrimitiveNodes/Sum.h"
#include "PrimitiveNodes/BitwiseOperator.h"

#include "MediumLevelNodes/CompareToConstant.h"

#include <map>
#include "math.h" //For M_PI_4



int DigitalDemodulator::getBitsPerSymbol() const {
    return bitsPerSymbol;
}

void DigitalDemodulator::setBitsPerSymbol(int bitsPerSymbol) {
    DigitalDemodulator::bitsPerSymbol = bitsPerSymbol;
}

double DigitalDemodulator::getRotation() const {
    return rotation;
}

void DigitalDemodulator::setRotation(double rotation) {
    DigitalDemodulator::rotation = rotation;
}

double DigitalDemodulator::getNormalization() const {
    return normalization;
}

void DigitalDemodulator::setNormalization(double normalization) {
    DigitalDemodulator::normalization = normalization;
}

bool DigitalDemodulator::isGrayCoded() const {
    return grayCoded;
}

void DigitalDemodulator::setGrayCoded(bool grayCoded) {
    DigitalDemodulator::grayCoded = grayCoded;
}

bool DigitalDemodulator::isAvgPwrNormalize() const {
    return avgPwrNormalize;
}

void DigitalDemodulator::setAvgPwrNormalize(bool avgPwrNormalize) {
    DigitalDemodulator::avgPwrNormalize = avgPwrNormalize;
}

DigitalDemodulator::DigitalDemodulator() : MediumLevelNode(), bitsPerSymbol(1), rotation(0), normalization(1), grayCoded(true), avgPwrNormalize(true) {

}

DigitalDemodulator::DigitalDemodulator(std::shared_ptr<SubSystem> parent) : MediumLevelNode(parent), bitsPerSymbol(1), rotation(0), normalization(1), grayCoded(true), avgPwrNormalize(true) {

}

DigitalDemodulator::DigitalDemodulator(std::shared_ptr<SubSystem> parent, DigitalDemodulator *orig) :  MediumLevelNode(parent, orig), bitsPerSymbol(orig->bitsPerSymbol), rotation(orig->rotation), normalization(orig->normalization), grayCoded(orig->grayCoded), avgPwrNormalize(orig->avgPwrNormalize) {

}

std::shared_ptr<DigitalDemodulator>
DigitalDemodulator::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                    std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node & Set Common Properties ====
    std::shared_ptr<DigitalDemodulator> newNode = NodeFactory::createNode<DigitalDemodulator>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    std::string bitsPerSymbolStr;
    std::string mStr;
    std::string rotationStr;
    std::string normalizationStr;
    bool grayCodedParsed;
    bool avgPwrNormalizeParsed;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- BitsPerSymbol, Rotation, Normalization, GrayCoded, AvgPwrNormalize
        bitsPerSymbolStr = dataKeyValueMap.at("BitsPerSymbol");
        rotationStr = dataKeyValueMap.at("Rotation");
        normalizationStr = dataKeyValueMap.at("Normalization");
        std::string grayCodedStr = dataKeyValueMap.at("GrayCoded");
        if(grayCodedStr == "0" || grayCodedStr == "false"){
            grayCodedParsed = false;
        }else{
            grayCodedParsed = true;
        }
        std::string avgPwrNormalizeStr = dataKeyValueMap.at("AvgPwrNormalize");
        if(avgPwrNormalizeStr == "0" || avgPwrNormalizeStr == "false"){
            avgPwrNormalizeParsed = false;
        }else{
            avgPwrNormalizeParsed = true;
        }
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Names -- SimulinkBlockType (Contains BPSK_DemodulatorBaseband, QPSK_DemodulatorBaseband, RectangularQAM_DemodulatorBaseband), M (QAM Only), Numeric.Ph, Enc, Numeric.AvgPow, Numeric.MinDist
        std::string simulinkBlockType = dataKeyValueMap.at("SimulinkBlockType");
        if(simulinkBlockType == "BPSK_DemodulatorBaseband"){
            mStr = "2";
        }else if(simulinkBlockType == "QPSK_DemodulatorBaseband"){
            mStr = "4";
        }else if(simulinkBlockType == "RectangularQAM_DemodulatorBaseband"){
            mStr = dataKeyValueMap.at("M");
        }

        if(simulinkBlockType == "QPSK_DemodulatorBaseband" || simulinkBlockType == "RectangularQAM_DemodulatorBaseband"){
            std::string encStr = dataKeyValueMap.at("Dec");
            if(encStr == "Gray"){
                grayCodedParsed = true;
            }else if(encStr == "Binary"){
                grayCodedParsed = false;
            }else{
                throw std::runtime_error("Error Parsing Encoding - DigitalDemodulator");
            }
        }else{
            grayCodedParsed = true;
            avgPwrNormalizeParsed = true;
        }

        if(simulinkBlockType == "RectangularQAM_DemodulatorBaseband"){
            //Only QAM>4 uses power normalization in the demodulator
            std::string powTypeStr = dataKeyValueMap.at("PowType");
            if(powTypeStr == "Average Power"){
                avgPwrNormalizeParsed = true;
                normalizationStr = dataKeyValueMap.at("Numeric.AvgPow");
            }else if(powTypeStr == "Min. distance between symbols"){
                avgPwrNormalizeParsed = false;
                normalizationStr = dataKeyValueMap.at("Numeric.MinDist");
            }else{
                throw std::runtime_error("Unsupported Power Type: " + powTypeStr + " - DigitalDemodulator");
            }
        }else{
            //BSPK and QPSK do not use power normalization since they simply slice at the origin.
            avgPwrNormalizeParsed = true;
            normalizationStr = "1";
        }

        rotationStr = dataKeyValueMap.at("Numeric.Ph");
    } else
    {
        throw std::runtime_error("Unsupported Dialect when parsing XML - DigitalDemodulator");
    }

    newNode->setGrayCoded(grayCodedParsed);
    newNode->setAvgPwrNormalize(avgPwrNormalizeParsed);

    std::vector<NumericValue> rotationNv = NumericValue::parseXMLString(rotationStr);
    std::vector<NumericValue> normalizationNV = NumericValue::parseXMLString(normalizationStr);

    if(dialect == GraphMLDialect::VITIS) {
        std::vector<NumericValue> bitsPerSymbolNV = NumericValue::parseXMLString(bitsPerSymbolStr);
        if (bitsPerSymbolNV.size() != 1) {
            throw std::runtime_error("Error Parsing BitsPerSymbol - DigitalModulator");
        }
        if (bitsPerSymbolNV[0].isComplex() || bitsPerSymbolNV[0].isFractional()) {
            throw std::runtime_error("Error Parsing BitsPerSymbol - DigitalModulator");
        }
        newNode->setBitsPerSymbol(bitsPerSymbolNV[0].getRealInt());
    }else if(dialect == GraphMLDialect::SIMULINK_EXPORT){
        std::vector<NumericValue> mNV = NumericValue::parseXMLString(mStr);
        if (mNV.size() != 1) {
            throw std::runtime_error("Error Parsing M - DigitalModulator");
        }
        if (mNV[0].isComplex() || mNV[0].isFractional()) {
            throw std::runtime_error("Error Parsing M - DigitalModulator");
        }

        int m = mNV[0].getRealInt();
        int bitsPerSymbolParsed = ceil(log2(m));

        newNode->setBitsPerSymbol(bitsPerSymbolParsed);
    }

    if(rotationNv.size() != 1){
        throw std::runtime_error("Error Parsing Rotation - DigitalDemodulator");
    }
    if(rotationNv[0].isComplex()){
        throw std::runtime_error("Error Parsing Rotation - DigitalDemodulator");
    }
    if(rotationNv[0].isFractional()) {
        newNode->setRotation(rotationNv[0].getComplexDouble().real());
    }else{
        newNode->setRotation(rotationNv[0].getRealInt());
    }

    //Handle simulink's view of QPSK rotation (rotation by pi/4 is our rotation by 0)
    if(dialect == GraphMLDialect::SIMULINK_EXPORT && newNode->getBitsPerSymbol()==2){
        double rotation = newNode->getRotation();
        newNode->setRotation(rotation-M_PI_4);
    }


    if(normalizationNV.size() != 1){
        throw std::runtime_error("Error Parsing Normalization - DigitalDemodulator");
    }
    if(normalizationNV[0].isComplex()){
        throw std::runtime_error("Error Parsing Normalization - DigitalDemodulator");
    }
    if(normalizationNV[0].isFractional()) {
        newNode->setNormalization(normalizationNV[0].getComplexDouble().real());
    }else{
        newNode->setNormalization(normalizationNV[0].getRealInt());
    }

    return newNode;
}

std::set<GraphMLParameter> DigitalDemodulator::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("BitsPerSymbol", "string", true));
    parameters.insert(GraphMLParameter("Rotation", "string", true));
    parameters.insert(GraphMLParameter("Normalization", "string", true));
    parameters.insert(GraphMLParameter("GrayCoded", "bool", true));
    parameters.insert(GraphMLParameter("AvgPwrNormalize", "bool", true));

    return parameters;
}

xercesc::DOMElement *
DigitalDemodulator::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "DigitalDemodulator");

    GraphMLHelper::addDataNode(doc, thisNode, "BitsPerSymbol", GeneralHelper::to_string(bitsPerSymbol));
    GraphMLHelper::addDataNode(doc, thisNode, "Rotation", GeneralHelper::to_string(rotation));
    GraphMLHelper::addDataNode(doc, thisNode, "Normalization", GeneralHelper::to_string(normalization));
    GraphMLHelper::addDataNode(doc, thisNode, "GrayCoded", GeneralHelper::to_string(grayCoded));
    GraphMLHelper::addDataNode(doc, thisNode, "AvgPwrNormalize", GeneralHelper::to_string(avgPwrNormalize));

    return thisNode;
}

std::string DigitalDemodulator::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: DigitalDemodulator\nBitsPerSymbol:" + GeneralHelper::to_string(bitsPerSymbol) +
             "\nRotation:" + GeneralHelper::to_string(rotation) +
             "\nNormalization:" + GeneralHelper::to_string(normalization) +
             "\nGrayCoded:" + GeneralHelper::to_string(grayCoded) +
             "\nAvgPwrNormalize:" + GeneralHelper::to_string(avgPwrNormalize);

    return label;
}

std::shared_ptr<Node> DigitalDemodulator::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<DigitalDemodulator>(parent, this);
}

void DigitalDemodulator::validate() {
    Node::validate();

    if(inputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - DigitalDemodulator - Should Have Exactly 1 Input Port");
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - DigitalDemodulator - Should Have Exactly 1 Output Port");
    }

    if(outputPorts[0]->getDataType().isComplex()){
        throw std::runtime_error("Validation Failed - DigitalDemodulator - Output Should not be Complex");
    }

    if(!inputPorts[0]->getDataType().isComplex()){
        throw std::runtime_error("Validation Failed - DigitalDemodulator - Input Should be Complex");
    }

    //Output may be real (BPSK) or imagionary

    //TODO: implement more modulation schemes
    if(bitsPerSymbol != 1 && bitsPerSymbol != 2 && bitsPerSymbol != 4){
        throw std::runtime_error("Validation Failed - DigitalDemodulator - Currently Only Supports BPSK, QPSK/4QAM, and 16QAM");

    }

    //TODO: Implement Rotation Suport
    //TODO: Do not use small epsilon
    if(abs(rotation) > 0.001){
        throw std::runtime_error("Validation Failed - DigitalDemodulator - Rotation currently not supported");
    }
}

std::shared_ptr<ExpandedNode> DigitalDemodulator::expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                         std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                                         std::shared_ptr<MasterUnconnected> &unconnected_master) {
    //Validate first to check that node is properly wired (ie. there is the proper number of ports, only 1 input arc, etc.)
    validate();

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

    //++++ Create LUT Block and Rewire ++++
    DataType inputDT = getInputPort(0)->getDataType();
    DataType inputRealDT = inputDT;
    inputRealDT.setComplex(false);

    std::shared_ptr<Node> outNode;

    if(bitsPerSymbol == 1 || bitsPerSymbol == 2){
        std::shared_ptr<ComplexToRealImag> complexToRealImag = NodeFactory::createNode<ComplexToRealImag>(expandedNode);
        new_nodes.push_back(complexToRealImag);

        //Rewire Input
        std::shared_ptr<Arc> inputArc = *(inputPorts[0]->getArcs().begin());
        complexToRealImag->addInArcUpdatePrevUpdateArc(0, inputArc);

        DataType outDT = getOutputPort(0)->getDataType();

        std::shared_ptr<CompareToConstant> compareToZeroReal = NodeFactory::createNode<CompareToConstant>(expandedNode);
        compareToZeroReal->setCompareOp(Compare::CompareOp::GEQ);
        compareToZeroReal->setCompareConst({NumericValue(0, 0, std::complex<double>(0, 0), false, true)});
        new_nodes.push_back(compareToZeroReal);

        //Connect real component of input to comparison
        std::shared_ptr<Arc> realInputToComparison = Arc::connectNodes(complexToRealImag, 0, compareToZeroReal, 0, inputRealDT);
        new_arcs.push_back(realInputToComparison);

        DataType boolDT = DataType(false, false, false, 1, 0, 1);

        if(bitsPerSymbol == 1) {
            //Create Mux and return demodulated signal
            std::shared_ptr<Mux> mux = NodeFactory::createNode<Mux>(expandedNode);
            mux->setBooleanSelect(true);
            mux->setUseSwitch(false);
            new_nodes.push_back(mux);

            //Connect real mux comparison
            std::shared_ptr<Arc> muxCtrl = Arc::connectNodes(compareToZeroReal, 0, mux, boolDT);
            new_arcs.push_back(realInputToComparison);

            //Connect imag component to unconnected node
            std::shared_ptr<Arc> imagInputToUnconnected = Arc::connectNodes(complexToRealImag, 1, unconnected_master, 0,
                                                                            inputRealDT);
            new_arcs.push_back(realInputToComparison);

            //Create Constants for Mux
            std::shared_ptr<Constant> constZero = NodeFactory::createNode<Constant>(expandedNode);
            constZero->setValue({NumericValue(0, 0, std::complex<double>(0, 0), false, false)});
            new_nodes.push_back(constZero);

            std::shared_ptr<Constant> constOne = NodeFactory::createNode<Constant>(expandedNode);
            constOne->setValue({NumericValue(1, 0, std::complex<double>(0, 0), false, false)});
            new_nodes.push_back(constOne);

            std::shared_ptr<Arc> constZeroToMux = Arc::connectNodes(constZero, 0, mux, 0, outDT);
            new_arcs.push_back(constZeroToMux);

            std::shared_ptr<Arc> constOneToMux = Arc::connectNodes(constOne, 0, mux, 1, outDT);
            new_arcs.push_back(constOneToMux);

            outNode = mux;
        }else{
            //bitsPerSymbol == 2
            std::shared_ptr<CompareToConstant> compareToZeroImag = NodeFactory::createNode<CompareToConstant>(expandedNode);
            compareToZeroImag->setCompareOp(Compare::CompareOp::GEQ);
            compareToZeroImag->setCompareConst({NumericValue(0, 0, std::complex<double>(0, 0), false, true)});
            new_nodes.push_back(compareToZeroImag);

            //Connect imag component of input to comparison (type is real due to complex->real/imag block)
            std::shared_ptr<Arc> imagInputToComparison = Arc::connectNodes(complexToRealImag, 1, compareToZeroImag, 0, inputRealDT);
            new_arcs.push_back(imagInputToComparison);

            //Creat mux tree
            //        Im  >=0
            //       T       F
            //       |       |
            //    Re >=0   Re >=0
            //    T    F   T    F
            //    |    |   |    |
            //    Q1   Q2  Q4   Q3

            //Mux tree should work well here as the number of points is small (avoids shift, or, lookup)
            //TODO: Implement a version using a LUT or 4 entry MUX

            std::shared_ptr<Mux> realMux_ImGEQ0 = NodeFactory::createNode<Mux>(expandedNode);
            realMux_ImGEQ0->setBooleanSelect(true);
            realMux_ImGEQ0->setUseSwitch(false);
            new_nodes.push_back(realMux_ImGEQ0);

            std::shared_ptr<Arc> realComparisonToRealMuxGEQ = Arc::connectNodes(compareToZeroReal, 0, realMux_ImGEQ0, boolDT);
            new_arcs.push_back(imagInputToComparison);

            std::shared_ptr<Constant> q1Const = NodeFactory::createNode<Constant>(expandedNode);
            q1Const->setValue({NumericValue(0, 0, std::complex<double>(0, 0), false, false)}); //Q1 is always 0
            new_nodes.push_back(q1Const);

            std::shared_ptr<Arc> q1ToMux = Arc::connectNodes(q1Const, 0, realMux_ImGEQ0, 0, outDT);
            new_arcs.push_back(q1ToMux);

            std::shared_ptr<Constant> q2Const = NodeFactory::createNode<Constant>(expandedNode);
            q2Const->setValue({NumericValue(1, 0, std::complex<double>(0, 0), false, false)}); //Q2 is always 1
            new_nodes.push_back(q2Const);

            std::shared_ptr<Arc> q2ToMux = Arc::connectNodes(q2Const, 0, realMux_ImGEQ0, 1, outDT);
            new_arcs.push_back(q2ToMux);

            std::shared_ptr<Mux> realMux_ImLT0 = NodeFactory::createNode<Mux>(expandedNode);
            realMux_ImLT0->setBooleanSelect(true);
            realMux_ImLT0->setUseSwitch(false);
            new_nodes.push_back(realMux_ImLT0);

            std::shared_ptr<Arc> realComparisonToRealMuxLT = Arc::connectNodes(compareToZeroReal, 0, realMux_ImLT0, boolDT);
            new_arcs.push_back(realComparisonToRealMuxLT);

            std::shared_ptr<Constant> q4Const = NodeFactory::createNode<Constant>(expandedNode);
            int q4ConstVal = grayCoded ? 2 : 3;
            q4Const->setValue({NumericValue(q4ConstVal, 0, std::complex<double>(0, 0), false, false)});
            new_nodes.push_back(q4Const);

            std::shared_ptr<Arc> q4ToMux = Arc::connectNodes(q4Const, 0, realMux_ImLT0, 0, outDT);
            new_arcs.push_back(q1ToMux);

            std::shared_ptr<Constant> q3Const = NodeFactory::createNode<Constant>(expandedNode);
            int q3ConstVal = grayCoded ? 3 : 2;
            q3Const->setValue({NumericValue(q3ConstVal, 0, std::complex<double>(0, 0), false, false)});
            new_nodes.push_back(q3Const);

            std::shared_ptr<Arc> q3ToMux = Arc::connectNodes(q3Const, 0, realMux_ImLT0, 1, outDT);
            new_arcs.push_back(q3ToMux);

            std::shared_ptr<Mux> imagMux = NodeFactory::createNode<Mux>(expandedNode);
            imagMux->setBooleanSelect(true);
            realMux_ImLT0->setUseSwitch(false);
            new_nodes.push_back(imagMux);

            std::shared_ptr<Arc> imagComparisonToImagMux = Arc::connectNodes(compareToZeroImag, 0, imagMux, boolDT);
            new_arcs.push_back(imagComparisonToImagMux);

            std::shared_ptr<Arc> realMux_ImGEQ0ToImagMux = Arc::connectNodes(realMux_ImGEQ0, 0, imagMux, 0, outDT);
            new_arcs.push_back(realMux_ImGEQ0ToImagMux);

            std::shared_ptr<Arc> realMux_ImLT0ToImagMux = Arc::connectNodes(realMux_ImLT0, 0, imagMux, 1, outDT);
            new_arcs.push_back(realMux_ImLT0ToImagMux);

            outNode = imagMux;
        }
    }else{
        //QAM > 4
        double scale_factor;
        //This is the reciprocal of the scale factor used in the modulator
        if(avgPwrNormalize) {
            scale_factor = 1/(normalization*2*sqrt(3.0 / (2.0 * (pow(2, bitsPerSymbol) - 1))));
        }else{
            scale_factor = 1/normalization;
        }

        std::shared_ptr<Gain> denormalizer = NodeFactory::createNode<Gain>(expandedNode);
        denormalizer->setGain({NumericValue(0, 0, std::complex<double>(scale_factor, 0), false, true)});

        //Rewire input
        std::shared_ptr<Arc> inputArc = *(inputPorts[0]->getArcs().begin());
        denormalizer->addInArcUpdatePrevUpdateArc(0, inputArc);

        //Offset the signal
        //TODO: Assuming square const
        int dimension = GeneralHelper::intPow(2, bitsPerSymbol/2);

        std::shared_ptr<Sum> signalOffsetSum = NodeFactory::createNode<Sum>(expandedNode);
        signalOffsetSum->setInputSign({true, true});
        new_nodes.push_back(signalOffsetSum);

        std::shared_ptr<Constant> offsetConst = NodeFactory::createNode<Constant>(expandedNode);
        offsetConst->setValue({NumericValue(0, 0, std::complex<double>(dimension/2, dimension/2), true, true)});
        new_nodes.push_back(offsetConst);

        std::shared_ptr<Arc> denormalizerToOffset = Arc::connectNodes(denormalizer, 0, signalOffsetSum, 0, inputDT);
        new_arcs.push_back(denormalizerToOffset);

        std::shared_ptr<Arc> offsetConstToOffset = Arc::connectNodes(offsetConst, 0, signalOffsetSum, 1, inputDT);
        new_arcs.push_back(offsetConstToOffset);

        std::shared_ptr<ComplexToRealImag> complexToRealImag = NodeFactory::createNode<ComplexToRealImag>(expandedNode);
        new_nodes.push_back(complexToRealImag);

        std::shared_ptr<Arc> offsetToCplxToRealImag = Arc::connectNodes(signalOffsetSum, 0, complexToRealImag, 0, inputDT);
        new_arcs.push_back(offsetToCplxToRealImag);

        //Saturate first to prevent going out of bounds
        //TODO: avoid using an epsilon
        double epsilon = scale_factor/10.0;

        std::shared_ptr<Saturate> realSaturate = NodeFactory::createNode<Saturate>(expandedNode);
        realSaturate->setLowerLimit(NumericValue(0, 0, std::complex<double>(0, 0), false, true));
        realSaturate->setUpperLimit(NumericValue(0, 0, std::complex<double>(dimension-epsilon, 0), false, true));
        new_nodes.push_back(realSaturate);

        std::shared_ptr<Arc> toRealSaturate = Arc::connectNodes(complexToRealImag, 0, realSaturate, 0, inputRealDT);
        new_arcs.push_back(toRealSaturate);

        std::shared_ptr<Saturate> imagSaturate = NodeFactory::createNode<Saturate>(expandedNode);
        imagSaturate->setLowerLimit(NumericValue(0, 0, std::complex<double>(0, 0), false, true));
        imagSaturate->setUpperLimit(NumericValue(0, 0, std::complex<double>(dimension-epsilon, 0), false, true));
        new_nodes.push_back(imagSaturate);

        std::shared_ptr<Arc> toImagSaturate = Arc::connectNodes(complexToRealImag, 1, imagSaturate, 0, inputRealDT);
        new_arcs.push_back(toImagSaturate);

        //Down-cast to int (truncate)
        //Can be unsigned since saturation occured first and forced the value to be in the domain [0, #rows/cols)
        DataType castIntDT = DataType(false, false, false, bitsPerSymbol, 0, 1);

        std::shared_ptr<DataTypeConversion> realDataTypeConversion = NodeFactory::createNode<DataTypeConversion>(expandedNode);
        realDataTypeConversion->setTgtDataType(castIntDT);
        new_nodes.push_back(realDataTypeConversion);

        std::shared_ptr<Arc> toRealDataTypeConversion = Arc::connectNodes(realSaturate, 0, realDataTypeConversion, 0, inputRealDT);
        new_arcs.push_back(toRealDataTypeConversion);

        std::shared_ptr<DataTypeConversion> imagDataTypeConversion = NodeFactory::createNode<DataTypeConversion>(expandedNode);
        imagDataTypeConversion->setTgtDataType(castIntDT);
        new_nodes.push_back(imagDataTypeConversion);

        std::shared_ptr<Arc> toImagDataTypeConversion = Arc::connectNodes(imagSaturate, 0, imagDataTypeConversion, 0, inputRealDT);
        new_arcs.push_back(toImagDataTypeConversion);

        //Shift and concat
        std::shared_ptr<BitwiseOperator> realShifter = NodeFactory::createNode<BitwiseOperator>(expandedNode);
        realShifter->setOp(BitwiseOperator::BitwiseOp::SHIFT_LEFT);
        new_nodes.push_back(realShifter);

        std::shared_ptr<Constant> shiftAmt = NodeFactory::createNode<Constant>(expandedNode);
        shiftAmt->setValue({NumericValue(bitsPerSymbol/2, 0, std::complex<double>(0, 0), false, false)});
        new_nodes.push_back(shiftAmt);

        std::shared_ptr<Arc> toRealShifter = Arc::connectNodes(realDataTypeConversion, 0, realShifter, 0, castIntDT);
        new_arcs.push_back(toRealShifter);

        std::shared_ptr<Arc> constToRealShifter = Arc::connectNodes(shiftAmt, 0, realShifter, 1, castIntDT);
        new_arcs.push_back(constToRealShifter);

        std::shared_ptr<BitwiseOperator> concat = NodeFactory::createNode<BitwiseOperator>(expandedNode);
        concat->setOp(BitwiseOperator::BitwiseOp::OR);
        new_nodes.push_back(concat);

        std::shared_ptr<Arc> imagToConcat = Arc::connectNodes(imagDataTypeConversion, 0, concat, 0, castIntDT);
        new_arcs.push_back(imagToConcat);

        std::shared_ptr<Arc> shiftedRealToConcat = Arc::connectNodes(realShifter, 0, concat, 1, castIntDT);
        new_arcs.push_back(shiftedRealToConcat);

        std::shared_ptr<LUT> decoderTable = NodeFactory::createNode<LUT>(expandedNode);
        new_nodes.push_back(decoderTable);

        std::vector<NumericValue> breakpoints;
        std::vector<NumericValue> tableData;

        for(unsigned long col = 0; col < dimension; col++){
            for(unsigned long row = 0; row < dimension; row++){
                breakpoints.push_back(NumericValue(col*dimension+row, 0, std::complex<double>(0, 0), false, false));

                //Check if col is even or odd
                bool even = ((col%2) == 0);

                unsigned long rowInd = even ? dimension-1-row : row; //The numbering snakes from the top left down then goes up on the next column
                //(0, 0) is the top left of the constallation but the origin of the signal coming from the concat is in the bottom left
                //Therefore, we reverse the indecies of the even number rows

                unsigned long binaryInd = dimension*col+rowInd;

                unsigned long code = grayCoded ? DSPHelpers::bin2Gray(binaryInd) : binaryInd;

                tableData.push_back(NumericValue(code, 0, std::complex<double>(0, 0), false, false));
            }
        }

        decoderTable->setBreakpoints({breakpoints});
        decoderTable->setTableData(tableData);

        std::shared_ptr<Arc> concatToDecoderTable = Arc::connectNodes(concat, 0, decoderTable, 0, castIntDT);
        new_arcs.push_back(concatToDecoderTable);

        outNode = decoderTable;
    }

    //Rewire Outputs
    std::set<std::shared_ptr<Arc>> outputArcs = getOutputPort(0)->getArcs();

    for(auto outArcIt = outputArcs.begin(); outArcIt != outputArcs.end(); outArcIt++){
        outNode->addOutArcUpdatePrevUpdateArc(0, *outArcIt);
    }

    return expandedNode;
}
