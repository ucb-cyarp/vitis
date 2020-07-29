//
// Created by Christopher Yarp on 8/22/18.
//

#include "DiscreteFIR.h"
#include "GraphCore/NodeFactory.h"
#include "General/ErrorHelpers.h"

DiscreteFIR::DiscreteFIR() {

}

DiscreteFIR::DiscreteFIR(std::shared_ptr<SubSystem> parent) : HighLevelNode(parent), coefSource(CoefSource::FIXED) {

}

DiscreteFIR::CoefSource DiscreteFIR::parseCoefSourceStr(std::string str) {
    if(str == "FIXED"){
        return CoefSource::FIXED;
    }else if(str == "INPUT_PORT"){
        return CoefSource::INPUT_PORT;
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown coef source type: " + str));
    }
}

std::string DiscreteFIR::coefSourceToString(DiscreteFIR::CoefSource coefSource) {
    if(coefSource == CoefSource::FIXED){
        return "FIXED";
    }else if(coefSource == CoefSource::INPUT_PORT){
        return "INPUT_PORT";
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown coef source type"));
    }
}

DiscreteFIR::CoefSource DiscreteFIR::getCoefSource() const {
    return coefSource;
}

void DiscreteFIR::setCoefSource(DiscreteFIR::CoefSource coefSource) {
    DiscreteFIR::coefSource = coefSource;
}

std::vector<NumericValue> DiscreteFIR::getCoefs() const {
    return coefs;
}

void DiscreteFIR::setCoefs(const std::vector<NumericValue> &coefs) {
    DiscreteFIR::coefs = coefs;
}

std::vector<NumericValue> DiscreteFIR::getInitVals() const {
    return initVals;
}

void DiscreteFIR::setInitVals(const std::vector<NumericValue> &initVals) {
    DiscreteFIR::initVals = initVals;
}

std::shared_ptr<DiscreteFIR>
DiscreteFIR::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                     std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node & Set Common Properties ====
    std::shared_ptr<DiscreteFIR> newNode = NodeFactory::createNode<DiscreteFIR>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====

    //Get Coef Source
    std::string coefSourceStr = dataKeyValueMap.at("CoefSource");
    CoefSource coefSource;

    if(dialect == GraphMLDialect::SIMULINK_EXPORT){
        if(coefSourceStr == "Dialog parameters"){
            coefSource = CoefSource::FIXED;
        }else if(coefSourceStr == "Input port"){
            coefSource = CoefSource::INPUT_PORT;
        }
    }else if(dialect == GraphMLDialect::VITIS){
        coefSource = parseCoefSourceStr(coefSourceStr);
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown GraphML Dialect", newNode));
    }

    newNode->setCoefSource(coefSource);

    //Get Coefs
    std::vector<NumericValue> coefs;
    if(coefSource == CoefSource::FIXED){
        std::string coefStr;
        if(dialect == GraphMLDialect::SIMULINK_EXPORT){
            coefStr = dataKeyValueMap.at("Numeric.Coefficients");
        }else if(dialect == GraphMLDialect::VITIS){
            coefStr = dataKeyValueMap.at("Coefficients");
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown GraphML Dialect", newNode));
        }

        coefs = NumericValue::parseXMLString(coefStr);
    }

    newNode->setCoefs(coefs);

    //Get init vals
    std::string initValStr;
    if(dialect == GraphMLDialect::SIMULINK_EXPORT){
        initValStr = dataKeyValueMap.at("Numeric.InitialStates");
    }else if(dialect == GraphMLDialect::VITIS){
        initValStr = dataKeyValueMap.at("InitialStates");
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown GraphML Dialect", newNode));
    }

    std::vector<NumericValue> initVals = NumericValue::parseXMLString(initValStr);

    newNode->setInitVals(initVals);

    return newNode;
}

std::shared_ptr<ExpandedNode>
DiscreteFIR::expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes,
                    std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                    std::shared_ptr<MasterUnconnected> &unconnected_master) {
    //Validate first to check that the DiscreteFIR block is properly wired
    validate();

    //TODO: Implement FIR Expansion
    throw std::runtime_error(ErrorHelpers::genErrorStr("FIR Expansion not yet implemented in VITIS.  If importing from Simulink, use Simulink expansion option in export script", getSharedPointer()));

    return Node::expand(new_nodes, deleted_nodes, new_arcs, deleted_arcs, unconnected_master);
}

std::set<GraphMLParameter> DiscreteFIR::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("CoefSource", "string", true));
    parameters.insert(GraphMLParameter("Coefficients", "string", true));
    parameters.insert(GraphMLParameter("InitialStates", "string", true));

    return parameters;
}

xercesc::DOMElement *
DiscreteFIR::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "DiscreteFIR");

    GraphMLHelper::addDataNode(doc, thisNode, "CoefSource", coefSourceToString(coefSource));
    GraphMLHelper::addDataNode(doc, thisNode, "Coefficients", NumericValue::toString(coefs));
    GraphMLHelper::addDataNode(doc, thisNode, "InitialStates", NumericValue::toString(initVals));

    return thisNode;
}

std::string DiscreteFIR::typeNameStr(){
    return "DiscreteFIR";
}

std::string DiscreteFIR::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: " + typeNameStr() + "\nCoefSource: " + coefSourceToString(coefSource);

    if(coefSource == CoefSource::FIXED){
        label += "\nCoefficients: " + NumericValue::toString(coefs);
    }

    label += "\nInitialStates: " + NumericValue::toString(initVals);

    return label;
}

void DiscreteFIR::validate() {
    Node::validate();

    if(coefSource == CoefSource::FIXED) {
        if (inputPorts.size() != 1) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "Validation Failed - DiscreteFIR - Should Have Exactly 1 Input Port With Fixed Coefs", getSharedPointer()));
        }
    }else if(coefSource == CoefSource::INPUT_PORT){
        if (inputPorts.size() != 2) {
            throw std::runtime_error(ErrorHelpers::genErrorStr(
                    "Validation Failed - DiscreteFIR - Should Have Exactly 2 Input Port With Input Port Coefs", getSharedPointer()));
        }
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown Coef Source", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - DiscreteFIR - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    //Check that the number of initial values is #coef-1
    if(coefSource == CoefSource::FIXED){
        if(initVals.size() != 1){ //Note, init values can be given as a single number if all registers share the same value
            if(initVals.size() != (coefs.size()-1)){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - DiscreteFIR - Number of Initial Values Should be 1 Less Than the Number of Coefs", getSharedPointer()));
            }
        }
    }else if(coefSource == CoefSource::INPUT_PORT){
        if(!getInputPort(1)->getDataType().isScalar() && !getInputPort(1)->getDataType().isVector()){
            //Scalar is possible if the filter has a single tap
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - DiscreteFIR - Coef port should be a vector or scalar", getSharedPointer()));
        }
        int coefWidth = getInputPort(1)->getDataType().getDimensions()[0];
        if(initVals.size() != (coefWidth-1)){ //Input port 1 is the coef input port
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - DiscreteFIR - Number of Initial Values Should be 1 Less Than the Number of Coefs", getSharedPointer()));
        }
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown Coef Source", getSharedPointer()));
    }
}

DiscreteFIR::DiscreteFIR(std::shared_ptr<SubSystem> parent, DiscreteFIR* orig) : HighLevelNode(parent, orig), coefSource(orig->coefSource), coefs(orig->coefs), initVals(orig->initVals) {

}

std::shared_ptr<Node> DiscreteFIR::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<DiscreteFIR>(parent, this);
}

bool DiscreteFIR::hasState(){
    return true;
}

bool DiscreteFIR::hasCombinationalPath(){
    return true;
}