//
// Created by Christopher Yarp on 8/22/18.
//

#include "DiscreteFIR.h"
#include "GraphCore/NodeFactory.h"
#include "General/ErrorHelpers.h"
#include "PrimitiveNodes/TappedDelay.h"
#include "PrimitiveNodes/InnerProduct.h"
#include "PrimitiveNodes/Constant.h"
#include "MediumLevelNodes/Gain.h"
#include "PrimitiveNodes/Product.h"

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

    //---- Expand the DiscreteFIR Node to a TappedDelay, InnerProduct, and possibly a Constant ----
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

    int numCoefs=0;
    if(coefSource == CoefSource::FIXED){
        numCoefs = coefs.size();
    }else if(coefSource == CoefSource::INPUT_PORT){
        numCoefs = getInputPort(1)->getDataType().getDimensions()[0];
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown Coef Source for DiscreteFIR", getSharedPointer()));
    }

    //++++ Expand ++++
    if(numCoefs==1) {
        std::shared_ptr<Node> newNode;
        //++++ Case where coef size is 1 ++++
        if (coefSource == CoefSource::FIXED) {
            //Expand to gain node
            std::shared_ptr<Gain> gainNode = NodeFactory::createNode<Gain>(expandedNode);
            gainNode->setName("Gain");
            gainNode->setGain(coefs);
            new_nodes.push_back(gainNode);
            newNode = gainNode;

            //Rewire input
            std::set<std::shared_ptr<Arc>> inputArc = getInputPort(0)->getArcs();
            //Already validated to have 1 input arc
            (*inputArc.begin())->setDstPortUpdateNewUpdatePrev(gainNode->getInputPortCreateIfNot(0));
        }else if (coefSource == CoefSource::INPUT_PORT) {
            //Expand to Mult node
            std::shared_ptr<Product> productNode = NodeFactory::createNode<Product>(expandedNode);
            productNode->setName("Product");
            productNode->setInputOp({true, true});
            new_nodes.push_back(productNode);

            //Rewire inputs (ports are swapped because following Harris semantics putting coef first)
            std::set<std::shared_ptr<Arc>> inputArc = getInputPort(0)->getArcs();
            //Already validated to have 1 input arc
            (*inputArc.begin())->setDstPortUpdateNewUpdatePrev(productNode->getInputPortCreateIfNot(1));

            std::set<std::shared_ptr<Arc>> coefArc = getInputPort(1)->getArcs();
            //Already validated to have 1 input arc
            (*coefArc.begin())->setDstPortUpdateNewUpdatePrev(productNode->getInputPortCreateIfNot(0));
        }else{
            throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown Coef Source for DiscreteFIR", getSharedPointer()));
        };

        //Rewire outputs
        DataType outputDT = getOutputPort(0)->getDataType();
        std::set<std::shared_ptr<Arc>> outArcs = getOutputPort(0)->getArcs();
        for (auto const &outArc : outArcs) {
            outArc->setSrcPortUpdateNewUpdatePrev(newNode->getOutputPortCreateIfNot(0));
        }

        //Rewire order constraint nodes to gain/product node
        //Rewire order constraint inputs to tapped delay and order constraint outputs to inner product
        std::set<std::shared_ptr<Arc>> orderConstraintInputArcs = getOrderConstraintInputArcs();
        for (auto const &orderConstraintInputArc : orderConstraintInputArcs) {
            orderConstraintInputArc->setDstPortUpdateNewUpdatePrev(
                    newNode->getOrderConstraintInputPortCreateIfNot());
        }

        std::set<std::shared_ptr<Arc>> orderConstraintOutputArcs = getOrderConstraintOutputArcs();
        for (auto const &orderConstraintOutputArc : orderConstraintOutputArcs) {
            orderConstraintOutputArc->setSrcPortUpdateNewUpdatePrev(
                    newNode->getOrderConstraintOutputPortCreateIfNot());
        }
    }else {
        //++++ Create TappedDelay and InnerProduct and Rewire ++++
        //Create Tapped Delay
        std::shared_ptr<TappedDelay> tappedDelayNode = NodeFactory::createNode<TappedDelay>(expandedNode);
        tappedDelayNode->setName("TappedDelay");
        //init value broadcast occurs in tapped delay
        tappedDelayNode->setInitCondition(initVals);
        tappedDelayNode->setDelayValue(numCoefs - 1);
        tappedDelayNode->setAllocateExtraSpace(true); //This passes the current value through for TappedDelay
        tappedDelayNode->setEarliestFirst(true); //The coefs are arranged in the standard order with earliest first
        new_nodes.push_back(tappedDelayNode);

        //Rewire Input to TappedDelay
        std::set<std::shared_ptr<Arc>> inputArc = getInputPort(0)->getArcs();
        //Already validated to have 1 input arc
        (*inputArc.begin())->setDstPortUpdateNewUpdatePrev(tappedDelayNode->getInputPortCreateIfNot(0));

        //Create Inner Product
        std::shared_ptr<InnerProduct> innerProductNode = NodeFactory::createNode<InnerProduct>(expandedNode);
        innerProductNode->setName("InnerProduct");
        innerProductNode->setComplexConjBehavior(
                InnerProduct::ComplexConjBehavior::NONE); //Do not complex conjugate for FIR
        new_nodes.push_back(innerProductNode);

        //Connect TappedDelay to InnerProduct
        DataType tappedDelayOutputDT = (*inputArc.begin())->getDataType();
        //We rely on the input being scalar and being expanded to a vector in the tapped delay
        tappedDelayOutputDT.setDimensions({numCoefs}); //The tapped delay is expanded by 1 for the current value
        std::shared_ptr<Arc> delayToInnerProd = Arc::connectNodes(tappedDelayNode->getOutputPortCreateIfNot(0),
                                                                  innerProductNode->getInputPortCreateIfNot(1),
                                                                  tappedDelayOutputDT);
        new_arcs.push_back(delayToInnerProd);

        //Connect Outputs to InnerProduct (Port swapped due to following Harris semantics putting coef first)
        DataType outputDT = getOutputPort(0)->getDataType();
        std::set<std::shared_ptr<Arc>> outArcs = getOutputPort(0)->getArcs();
        for (auto const &outArc : outArcs) {
            outArc->setSrcPortUpdateNewUpdatePrev(innerProductNode->getOutputPortCreateIfNot(0));
        }

        //++++ Create Constant if Needed ++++
        if (coefSource == CoefSource::FIXED) {
            std::shared_ptr<Constant> coefNode = NodeFactory::createNode<Constant>(expandedNode);
            coefNode->setName("Coefs");
            coefNode->setValue(coefs);
            new_nodes.push_back(coefNode);

            //-- Determine Datatype from numeric values --
            //      Check for Float
            //        If float, mimic output type
            //      Check for Signed
            //      If all int, find max number of bits required
            //Check if any of the coefs are floats
            bool isFloat = false;
            bool isComplex = false;
            for (int i = 0; i < coefs.size(); i++) {
                if (coefs[i].isFractional()) {
                    isFloat = true;
                }
                if (coefs[i].isComplex()) {
                    isComplex = true;
                }
            }

            DataType coefDataType;
            if (isFloat) {
                //Validated that, if any of the coefs are fractional, the output is floating pt
                //TODO: Change if fixed point implemented
                coefDataType = outputDT;
                coefDataType.setComplex(isComplex); //Output can be complex but input can be real
                coefDataType.setDimensions({numCoefs});
            } else {
                //All are integers, adopt an int type
                bool isSigned = false;
                for (int i = 0; i < coefs.size(); i++) {
                    if (coefs[i].isSigned()) {
                        isSigned = true;
                    }
                }

                int maxBits = 0;
                for (int i = 0; i < coefs.size(); i++) {
                    int bitsRequired = coefs[i].numIntegerBits();
                    if (!coefs[i].isSigned() && isSigned) {
                        bitsRequired++;
                    }
                    if (bitsRequired > maxBits) {
                        maxBits = bitsRequired;
                    }
                }

                //TODO: Change if fixed point implemented
                coefDataType = DataType(false, isSigned, isComplex, maxBits, 0, {numCoefs});
            }

            //Connect constant to inner product
            std::shared_ptr<Arc> coefArc = Arc::connectNodes(coefNode->getOutputPortCreateIfNot(0),
                                                             innerProductNode->getInputPortCreateIfNot(0),
                                                             coefDataType);
            new_arcs.push_back(coefArc);
        } else if (coefSource == CoefSource::INPUT_PORT) {
            //Rewire the coef input
            std::set<std::shared_ptr<Arc>> coefArc = getInputPort(1)->getArcs();
            //Was validated to only have 1 arc
            (*coefArc.begin())->setDstPortUpdateNewUpdatePrev(innerProductNode->getInputPortCreateIfNot(0));
        } else {
            throw std::runtime_error(
                    ErrorHelpers::genErrorStr("Unknown Coef Source for DiscreteFIR", getSharedPointer()));
        }

        //Rewire order constraint inputs to tapped delay and order constraint outputs to inner product
        std::set<std::shared_ptr<Arc>> orderConstraintInputArcs = getOrderConstraintInputArcs();
        for (auto const &orderConstraintInputArc : orderConstraintInputArcs) {
            orderConstraintInputArc->setDstPortUpdateNewUpdatePrev(
                    tappedDelayNode->getOrderConstraintInputPortCreateIfNot());
        }

        std::set<std::shared_ptr<Arc>> orderConstraintOutputArcs = getOrderConstraintOutputArcs();
        for (auto const &orderConstraintOutputArc : orderConstraintOutputArcs) {
            orderConstraintOutputArc->setSrcPortUpdateNewUpdatePrev(
                    innerProductNode->getOrderConstraintOutputPortCreateIfNot());
        }
    }

    return expandedNode;
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

    if(!getOutputPort(0)->getDataType().isScalar()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - DiscreteFIR - Output Should be Scalar", getSharedPointer()));
    }

    //Check that the number of initial values is #coef-1
    if(coefSource == CoefSource::FIXED){
        if(inputPorts.size() != 1){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - DiscreteFIR - Should Have Exactly 1 Input Port When Operating with Fixed Coef", getSharedPointer()));
        }

        if(coefs.size()<=0){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - DiscreteFIR - Should Have >=1 Coef Defined When Operating with Fixed Coef", getSharedPointer()));
        }

        bool coefIsFloat = false;
        for(int i = 0; i<coefs.size(); i++){
            if(coefs[i].isFractional()){
                coefIsFloat = true;
            }
        }

        if(coefIsFloat && !getOutputPort(0)->getDataType().isFloatingPt()){
            //TODO: Change if fixed point implemented
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - DiscreteFIR - If Coefs Are Fractional, Output Should be Floating Point", getSharedPointer()));
        }

        if(initVals.size() != 1){ //Note, init values can be given as a single number if all registers share the same value
            if(initVals.size() != (coefs.size()-1)){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - DiscreteFIR - Number of Initial Values Should be 1 Less Than the Number of Coefs", getSharedPointer()));
            }
        }
    }else if(coefSource == CoefSource::INPUT_PORT){
        if(inputPorts.size() != 2){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - DiscreteFIR - Should Have Exactly 2 Input Ports When Operating with Variable Coef", getSharedPointer()));
        }

        if(!getInputPort(1)->getDataType().isScalar() && !getInputPort(1)->getDataType().isVector()){
            //Scalar is possible if the filter has a single tap
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - DiscreteFIR - Coef port should be a vector or scalar", getSharedPointer()));
        }
        int coefWidth = getInputPort(1)->getDataType().getDimensions()[0];

        if(initVals.size() != 1) { //Initial value is allowed to be a single value that is broadcast
            if (initVals.size() != (coefWidth - 1)) { //Input port 1 is the coef input port
                throw std::runtime_error(ErrorHelpers::genErrorStr(
                        "Validation Failed - DiscreteFIR - Number of Initial Values Should be 1 Less Than the Number of Coefs",
                        getSharedPointer()));
            }
        }
    }else{
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unknown Coef Source", getSharedPointer()));
    }

    if(!getInputPort(0)->getDataType().isScalar()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - DiscreteFIR - Input Should be Scalar", getSharedPointer()));
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
