//
// Created by Christopher Yarp on 2019-02-28.
//

#include "DigitalModulator.h"
#include "General/GeneralHelper.h"
#include "General/DSPHelpers.h"

#include "PrimitiveNodes/LUT.h"

#include <map>
#include "math.h" //For M_PI_4

int DigitalModulator::getBitsPerSymbol() const {
    return bitsPerSymbol;
}

void DigitalModulator::setBitsPerSymbol(int bitsPerSymbol) {
    DigitalModulator::bitsPerSymbol = bitsPerSymbol;
}

double DigitalModulator::getRotation() const {
    return rotation;
}

void DigitalModulator::setRotation(double rotation) {
    DigitalModulator::rotation = rotation;
}

double DigitalModulator::getNormalization() const {
    return normalization;
}

void DigitalModulator::setNormalization(double normalization) {
    DigitalModulator::normalization = normalization;
}

bool DigitalModulator::isGrayCoded() const {
    return grayCoded;
}

void DigitalModulator::setGrayCoded(bool grayCoded) {
    DigitalModulator::grayCoded = grayCoded;
}

bool DigitalModulator::isAvgPwrNormalize() const {
    return avgPwrNormalize;
}

void DigitalModulator::setAvgPwrNormalize(bool avgPwrNormalize) {
    DigitalModulator::avgPwrNormalize = avgPwrNormalize;
}

DigitalModulator::DigitalModulator() : MediumLevelNode(), bitsPerSymbol(1), rotation(0), normalization(1), grayCoded(true), avgPwrNormalize(true) {

}

DigitalModulator::DigitalModulator(std::shared_ptr<SubSystem> parent) : MediumLevelNode(parent), bitsPerSymbol(1), rotation(0), normalization(1), grayCoded(true), avgPwrNormalize(true) {

}

DigitalModulator::DigitalModulator(std::shared_ptr<SubSystem> parent, DigitalModulator *orig) :  MediumLevelNode(parent, orig), bitsPerSymbol(orig->bitsPerSymbol), rotation(orig->rotation), normalization(orig->normalization), grayCoded(orig->grayCoded), avgPwrNormalize(orig->avgPwrNormalize) {

}

std::shared_ptr<DigitalModulator>
DigitalModulator::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                    std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node & Set Common Properties ====
    std::shared_ptr<DigitalModulator> newNode = NodeFactory::createNode<DigitalModulator>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    std::string bitsPerSymbolStr;
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
        //Simulink Names -- SimulinkBlockType (Contains BPSK_ModulatorBaseband, QPSK_ModulatorBaseband, RectangularQAM_ModulatorBaseband), M (QAM Only), Numeric.Ph, Enc, Numeric.AvgPow, Numeric.MinDist
        std::string simulinkBlockType = dataKeyValueMap.at("SimulinkBlockType");
        if(simulinkBlockType == "BPSK_ModulatorBaseband"){
            bitsPerSymbolStr = "1";
        }else if(simulinkBlockType == "QPSK_ModulatorBaseband"){
            bitsPerSymbolStr = "2";
        }else if(simulinkBlockType == "RectangularQAM_ModulatorBaseband"){
            bitsPerSymbolStr = dataKeyValueMap.at("M");
        }

        if(simulinkBlockType == "QPSK_ModulatorBaseband" || simulinkBlockType == "RectangularQAM_ModulatorBaseband"){
            std::string encStr = dataKeyValueMap.at("Enc");
            if(encStr == "Gray"){
                grayCodedParsed = true;
            }else if(encStr == "Binary"){
                grayCodedParsed = false;
            }else{
                throw std::runtime_error("Error Parsing Encoding - DigitalModulator");
            }

            std::string powTypeStr = dataKeyValueMap.at("PowType");
            if(powTypeStr == "Average Power"){
                avgPwrNormalizeParsed = true;
                normalizationStr = dataKeyValueMap.at("Numeric.AvgPow");
            }else if(powTypeStr == "Min. distance between symbols"){
                avgPwrNormalizeParsed = false;
                normalizationStr = dataKeyValueMap.at("Numeric.MinDist");
            }else{
                throw std::runtime_error("Unsupported Power Type: " + powTypeStr + " - DigitalModulator");
            }
        }else{
            grayCodedParsed = true;
            avgPwrNormalizeParsed = true;
        }

        rotationStr = dataKeyValueMap.at("Numeric.Ph");
    } else
    {
        throw std::runtime_error("Unsupported Dialect when parsing XML - DigitalModulator");
    }

    std::vector<NumericValue> bitsPerSymbolNV = NumericValue::parseXMLString(bitsPerSymbolStr);
    std::vector<NumericValue> rotationNv = NumericValue::parseXMLString(rotationStr);
    std::vector<NumericValue> normalizationNV = NumericValue::parseXMLString(normalizationStr);

    if(bitsPerSymbolNV.size() != 1){
        throw std::runtime_error("Error Parsing BitsPerSymbol - DigitalModulator");
    }
    if(bitsPerSymbolNV[0].isComplex() || bitsPerSymbolNV[0].isFractional()){
        throw std::runtime_error("Error Parsing BitsPerSymbol - DigitalModulator");
    }
    newNode->setBitsPerSymbol(bitsPerSymbolNV[0].getRealInt());

    if(rotationNv.size() != 1){
        throw std::runtime_error("Error Parsing Rotation - DigitalModulator");
    }
    if(rotationNv[0].isComplex()){
        throw std::runtime_error("Error Parsing Rotation - DigitalModulator");
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
        throw std::runtime_error("Error Parsing DitherBits - DigitalModulator");
    }
    if(normalizationNV[0].isComplex()){
        throw std::runtime_error("Error Parsing DitherBits - DigitalModulator");
    }
    if(normalizationNV[0].isFractional()) {
        newNode->setNormalization(normalizationNV[0].getComplexDouble().real());
    }else{
        newNode->setNormalization(normalizationNV[0].getRealInt());
    }

    return newNode;
}

std::set<GraphMLParameter> DigitalModulator::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("BitsPerSymbol", "string", true));
    parameters.insert(GraphMLParameter("Rotation", "string", true));
    parameters.insert(GraphMLParameter("Normalization", "string", true));
    parameters.insert(GraphMLParameter("GrayCoded", "bool", true));
    parameters.insert(GraphMLParameter("AvgPwrNormalize", "bool", true));

    return parameters;
}

xercesc::DOMElement *
DigitalModulator::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "DigitalModulator");

    GraphMLHelper::addDataNode(doc, thisNode, "BitsPerSymbol", GeneralHelper::to_string(bitsPerSymbol));
    GraphMLHelper::addDataNode(doc, thisNode, "Rotation", GeneralHelper::to_string(rotation));
    GraphMLHelper::addDataNode(doc, thisNode, "Normalization", GeneralHelper::to_string(normalization));
    GraphMLHelper::addDataNode(doc, thisNode, "GrayCoded", GeneralHelper::to_string(grayCoded));
    GraphMLHelper::addDataNode(doc, thisNode, "AvgPwrNormalize", GeneralHelper::to_string(avgPwrNormalize));

    return thisNode;
}

std::string DigitalModulator::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: DigitalModulator\nBitsPerSymbol:" + GeneralHelper::to_string(bitsPerSymbol) +
             "\nRotation:" + GeneralHelper::to_string(rotation) +
             "\nNormalization:" + GeneralHelper::to_string(normalization) +
             "\nGrayCoded:" + GeneralHelper::to_string(grayCoded) +
             "\nAvgPwrNormalize:" + GeneralHelper::to_string(avgPwrNormalize);

    return label;
}

std::shared_ptr<Node> DigitalModulator::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<DigitalModulator>(parent, this);
}

void DigitalModulator::validate() {
    Node::validate();

    if(inputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - DigitalModulator - Should Have Exactly 1 Input Port");
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - DigitalModulator - Should Have Exactly 1 Output Port");
    }

    if(inputPorts[0]->getDataType().isComplex()){
        throw std::runtime_error("Validation Failed - DigitalModulator - Input Should not be Complex");
    }

    //Output may be real (BPSK) or imagionary

    //TODO: implement more modulation schemes
    if(bitsPerSymbol != 1 && bitsPerSymbol != 2 && bitsPerSymbol != 4){
        throw std::runtime_error("Validation Failed - DigitalModulator - Currently Only Supports BPSK, QPSK/4QAM, and 16QAM");

    }

    //TODO: Implement Rotation Suport
    //TODO: Do not use small epsilon
    if(abs(rotation) > 0.001){
        throw std::runtime_error("Validation Failed - DigitalModulator - Rotation currently not supported");
    }
}

std::shared_ptr<ExpandedNode> DigitalModulator::expand(std::vector<std::shared_ptr<Node>> &new_nodes,
                                                       std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                                       std::vector<std::shared_ptr<Arc>> &new_arcs,
                                                       std::vector<std::shared_ptr<Arc>> &deleted_arcs) {
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
    std::shared_ptr<LUT> lut = NodeFactory::createNode<LUT>(expandedNode);
    new_nodes.push_back(lut);

    //Compute table to set in lut

    std::vector<std::vector<NumericValue>> breakpoints;

    std::map<int, int> code2BinMapping;

    std::vector<NumericValue> breakpoints1D;
    unsigned long numPts = GeneralHelper::intPow(2, bitsPerSymbol);
    for(unsigned long i = 0; i<numPts; i++){
        unsigned long code = grayCoded ? DSPHelpers::bin2Gray(i) : i;

        code2BinMapping[code] = i; //This is the mapping from the code to the binary number
                                   //This is used to determine what position the code is in the
                                   //constallation.  The constPts below start with 0 in the upper
                                   //left decending and wrap at the bottom
                                   //taking code2BinMapping[code] reveals that value's position
                                   //in constPts

        NumericValue val;
        val.setRealInt(i); //The table breakpoints need to be monotonically increating
        val.setComplex(false);
        val.setFractional(false);
        breakpoints1D.push_back(val);
    }
    breakpoints.push_back(breakpoints1D);

    std::vector<NumericValue> constPts;
    if(bitsPerSymbol == 1){
        constPts.push_back(NumericValue(1, 0, std::complex<double>(1, 0), true, false));
        constPts.push_back(NumericValue(-1, 0, std::complex<double>(-1, 0), true, false));
    }else if(bitsPerSymbol == 2){
        double scale_factor = sqrt(3.0/(2.0*(pow(2,bitsPerSymbol)-1)));

        //Special case dictated by simulink
        constPts.push_back(NumericValue(0, 0, std::complex<double>(scale_factor, scale_factor), true, true));
        constPts.push_back(NumericValue(0, 0, std::complex<double>(-scale_factor, scale_factor), true, true));
        constPts.push_back(NumericValue(0, 0, std::complex<double>(-scale_factor, -scale_factor), true, true));
        constPts.push_back(NumericValue(0, 0, std::complex<double>(scale_factor, -scale_factor), true, true));
    }else{
        double scale_factor = sqrt(3.0/(2.0*(pow(2,bitsPerSymbol)-1)));

        //TODO: Assuming power of 2
        int dimension = GeneralHelper::intPow(2, bitsPerSymbol-1);

        for(unsigned long col = 0; col<dimension; col++){
            double unscaledRe = col - (dimension/2) + 0.5;
            double scaledRe = unscaledRe*scale_factor;

            for(unsigned long row = 0; row<dimension; row++){
                bool down = (col%2 == 0);

                double unscaledIm;
                if(down){
                    unscaledRe = -(row - (dimension/2) + 0.5);
                }else{
                    unscaledRe = row - (dimension/2) + 0.5;
                }

                double scaledIm = unscaledRe*scale_factor;

                constPts.push_back(NumericValue(0, 0, std::complex<double>(scaledRe, scaledIm), true, true));
            }
        }
    }

    std::vector<NumericValue> constPtsSorted; //These are the constallation poins sorted by the code value rather than their position in the constallation
    for(unsigned long i = 0; i<numPts; i++){
        constPtsSorted.push_back(constPts[code2BinMapping[i]]);
    }

    lut->setInterpMethod(LUT::InterpMethod::FLAT);
    lut->setExtrapMethod(LUT::ExtrapMethod::NO_CHECK);
    lut->setSearchMethod(LUT::SearchMethod::EVENLY_SPACED_POINTS);
    lut->setBreakpoints(breakpoints);
    lut->setTableData(constPtsSorted);

    //---- Rewire ----
    std::shared_ptr<Arc> inputArc = *(inputPorts[0]->getArcs().begin());
    lut->addOutArcUpdatePrevUpdateArc(0, inputArc);

    std::set<std::shared_ptr<Arc>> outputArcs = outputPorts[0]->getArcs();
    for(auto outputArc = outputArcs.begin(); outputArc != outputArcs.end(); outputArc++){
        lut->addOutArcUpdatePrevUpdateArc(0, *outputArc);
    }

    return expandedNode;
}
