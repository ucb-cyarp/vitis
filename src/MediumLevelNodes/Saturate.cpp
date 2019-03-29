//
// Created by Christopher Yarp on 9/13/18.
//

#include "Saturate.h"

#include "PrimitiveNodes/ComplexToRealImag.h"
#include "PrimitiveNodes/RealImagToComplex.h"
#include "PrimitiveNodes/Mux.h"
#include "PrimitiveNodes/Constant.h"
#include "PrimitiveNodes/Compare.h"
#include "General/ErrorHelpers.h"

Saturate::Saturate() {

}

Saturate::Saturate(std::shared_ptr<SubSystem> parent) : MediumLevelNode(parent) {

}

NumericValue Saturate::getLowerLimit() const {
    return lowerLimit;
}

void Saturate::setLowerLimit(const NumericValue &lowerLimit) {
    Saturate::lowerLimit = lowerLimit;
}

NumericValue Saturate::getUpperLimit() const {
    return upperLimit;
}

void Saturate::setUpperLimit(const NumericValue &upperLimit) {
    Saturate::upperLimit = upperLimit;
}

std::shared_ptr<Saturate>
Saturate::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                            std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node & Set Common Properties ====
    std::shared_ptr<Saturate> newNode = NodeFactory::createNode<Saturate>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    std::string lowerLimitStr;
    std::string upperLimitStr;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- LowerLimit, UpperLimit
        lowerLimitStr = dataKeyValueMap.at("LowerLimit");
        upperLimitStr = dataKeyValueMap.at("UpperLimit");
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Names -- Numeric.Gain
        lowerLimitStr = dataKeyValueMap.at("Numeric.LowerLimit");
        upperLimitStr = dataKeyValueMap.at("Numeric.UpperLimit");
    } else
    {
        throw std::runtime_error(ErrorHelpers::genErrorStr("Unsupported Dialect when parsing XML - Saturate", newNode));
    }

    std::vector<NumericValue> lowerLimit = NumericValue::parseXMLString(lowerLimitStr);
    std::vector<NumericValue> upperLimit = NumericValue::parseXMLString(upperLimitStr);

    if(lowerLimit.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Error parsing XML - Saturate - Lower Limit Should be a Single Value", newNode));
    }

    if(upperLimit.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Error parsing XML - Saturate - Upper Limit Should be a Single Value", newNode));
    }

    newNode->setLowerLimit(lowerLimit[0]);
    newNode->setUpperLimit(upperLimit[0]);

    return newNode;
}

std::set<GraphMLParameter> Saturate::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("LowerLimit", "string", true));
    parameters.insert(GraphMLParameter("UpperLimit", "string", true));

    return parameters;
}

xercesc::DOMElement *
Saturate::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Saturate");

    GraphMLHelper::addDataNode(doc, thisNode, "LowerLimit", lowerLimit.toString());
    GraphMLHelper::addDataNode(doc, thisNode, "UpperLimit", upperLimit.toString());

    return thisNode;
}

std::string Saturate::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: Saturate\nLowerLimit: " + lowerLimit.toString() + "\nUpperLimit: " + upperLimit.toString();

    return label;
}

void Saturate::validate() {
    Node::validate();

    if(inputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Saturate - Should Have Exactly 1 Input Port", getSharedPointer()));
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Saturate - Should Have Exactly 1 Output Port", getSharedPointer()));
    }

    //Check that the input and output datatypes are the same
    if(getInputPort(0)->getDataType() != getOutputPort(0)->getDataType()){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Saturate - Input and Output Should Have Same Type", getSharedPointer()));
    }

    if(lowerLimit.isFractional() || upperLimit.isFractional()){
        double lower = lowerLimit.isFractional() ? lowerLimit.getComplexDouble().real() : lowerLimit.getRealInt();
        double upper = upperLimit.isFractional() ? upperLimit.getComplexDouble().real() : upperLimit.getRealInt();

        if(upper < lower){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Saturate - Real Component of Upper Limit Must be Greater Than or Equal To the Real Component of the Lower Limit", getSharedPointer()));
        }
    }else{
        int64_t lower = lowerLimit.getRealInt();
        int64_t upper = upperLimit.getRealInt();

        if(upper < lower){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Saturate - Real Component of Upper Limit Must be Greater Than or Equal To the Real Component of the Lower Limit", getSharedPointer()));
        }
    }

    if(lowerLimit.isComplex() || upperLimit.isComplex()){
        if(lowerLimit.isFractional() || upperLimit.isFractional()){
            double lower = lowerLimit.isComplex() ? (lowerLimit.isFractional() ? lowerLimit.getComplexDouble().imag() : lowerLimit.getImagInt()) : 0;
            double upper = upperLimit.isComplex() ? (upperLimit.isFractional() ? upperLimit.getComplexDouble().imag() : upperLimit.getImagInt()) : 0;

            if(upper < lower){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Saturate - Imag Component of Upper Limit Must be Greater Than or Equal To the Imag Component of the Lower Limit", getSharedPointer()));
            }
        }else{
            int64_t lower = lowerLimit.isComplex() ? lowerLimit.getImagInt() : 0;
            int64_t upper = upperLimit.isComplex() ? upperLimit.getImagInt() : 0;

            if(upper < lower){
                throw std::runtime_error(ErrorHelpers::genErrorStr("Validation Failed - Saturate - Imag Component of Upper Limit Must be Greater Than or Equal To the Imag Component of the Lower Limit", getSharedPointer()));
            }
        }
    }
}

std::shared_ptr<ExpandedNode> Saturate::expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                               std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                               std::shared_ptr<MasterUnconnected> &unconnected_master) {

    //Validate first to check that Saturate is properly wired
    validate();

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


    //==== Get Datatypes ====
    DataType inOutType = getInputPort(0)->getDataType(); //Validated earlier that input and output types are the same
    DataType intermediateType = inOutType;
    bool complex = inOutType.isComplex();

    if(complex){
        //If we are dealing with a complex inputs, the intermediate wires become real after the ComplexToRealImag block
        intermediateType.setComplex(false);
    }

    DataType boolType = DataType(false, false, false, 1, 0, 1);

    //==== Begin Creating new Nodes and Re-wireing ====

    std::shared_ptr<Node> inputSrcNode;
    int inputSrcNodeOutputPortNumber;

    //Only used for complex output
    std::shared_ptr<RealImagToComplex> realImagToComplexNode;

    if(complex){
        std::shared_ptr<ComplexToRealImag> complexToRealImagNode = NodeFactory::createNode<ComplexToRealImag>(expandedNode);
        complexToRealImagNode->setName("ComplexToRealImag");
        new_nodes.push_back(complexToRealImagNode);

        //Rewire input
        std::shared_ptr<Arc> inputArc = *(inputPorts[0]->getArcs().begin());
        complexToRealImagNode->addInArcUpdatePrevUpdateArc(0, inputArc); //Should remove the arc from this node's port

        //Set Input src node
        inputSrcNode = complexToRealImagNode;
        inputSrcNodeOutputPortNumber = 0; //Used in the real expansion where the output of the ComplexToRealImag is used

        realImagToComplexNode = NodeFactory::createNode<RealImagToComplex>(expandedNode);
        realImagToComplexNode->setName("RealImagToComplex");
        new_nodes.push_back(realImagToComplexNode);

        //Rewire Outputs
        std::set<std::shared_ptr<Arc>> outputArcs = outputPorts[0]->getArcs(); //Make a copy of the arc set since re-assigning the arc will remove it from the port's set
        for(auto outputArc = outputArcs.begin(); outputArc != outputArcs.end(); outputArc++){
            realImagToComplexNode->addOutArcUpdatePrevUpdateArc(0, *outputArc);
        }
    }else{
        std::shared_ptr<Arc> inputArc = *(inputPorts[0]->getArcs().begin());
        std::shared_ptr<OutputPort> inputArcOutputPort = inputArc->getSrcPort();
        inputSrcNode = inputArcOutputPort->getParent();
        inputSrcNodeOutputPortNumber = inputArcOutputPort->getPortNum();
    }

    //Create Constants, compares, muxes for real component
    NumericValue lowerLimitReal = lowerLimit;
    lowerLimitReal.setComplex(false);
    NumericValue upperLimitReal = upperLimit;
    upperLimitReal.setComplex(false);

    std::shared_ptr<Constant> upperLimitNode = NodeFactory::createNode<Constant>(expandedNode);
    upperLimitNode->setName("UpperLimitReal");
    upperLimitNode->setValue(std::vector<NumericValue>{upperLimitReal});
    new_nodes.push_back(upperLimitNode);

    std::shared_ptr<Constant> lowerLimitNode = NodeFactory::createNode<Constant>(expandedNode);
    lowerLimitNode->setValue(std::vector<NumericValue>{lowerLimitReal});
    lowerLimitNode->setName("LowerLimitReal");

    new_nodes.push_back(lowerLimitNode);

    std::shared_ptr<Compare> upperLimitCompare = NodeFactory::createNode<Compare>(expandedNode);
    upperLimitCompare->setName("UpperLimitRealCompare");
    upperLimitCompare->setCompareOp(Compare::CompareOp::GT);
    new_nodes.push_back(upperLimitCompare);

    std::shared_ptr<Compare> lowerLimitCompare = NodeFactory::createNode<Compare>(expandedNode);
    lowerLimitCompare->setName("LowerLimitRealCompare");
    lowerLimitCompare->setCompareOp(Compare::CompareOp::LT);
    new_nodes.push_back(lowerLimitCompare);

    std::shared_ptr<Mux> upperLimitMux = NodeFactory::createNode<Mux>(expandedNode);
    upperLimitMux->setName("UpperLimitRealMux");
    upperLimitMux->setBooleanSelect(true); //Treat port 0 as the true port
    new_nodes.push_back(upperLimitMux);

    std::shared_ptr<Mux> lowerLimitMux = NodeFactory::createNode<Mux>(expandedNode);
    lowerLimitMux->setName("LowerLimitRealMux");
    lowerLimitMux->setBooleanSelect(true); //Treat port 0 as the true port
    new_nodes.push_back(lowerLimitMux);

    //Connect Arcs
    if(!complex){
        //Special case for moving input arc rather than creating new one
        std::shared_ptr<Arc> inputArc = *(inputPorts[0]->getArcs().begin());
        upperLimitCompare->addInArcUpdatePrevUpdateArc(0, inputArc); //Should remove the arc from this node's port
    }else{
        //The input node is the complexToRealImag, use port 0 for real
        std::shared_ptr<Arc> inputToUpperLimitCompare = Arc::connectNodes(inputSrcNode, inputSrcNodeOutputPortNumber, upperLimitCompare, 0, intermediateType);
        new_arcs.push_back(inputToUpperLimitCompare);
    }

    std::shared_ptr<Arc> upperLimitToUpperLimitCompare = Arc::connectNodes(upperLimitNode, 0, upperLimitCompare, 1, intermediateType);
    new_arcs.push_back(upperLimitToUpperLimitCompare);

    std::shared_ptr<Arc> upperLimitCompareToUpperLimitMux = Arc::connectNodes(upperLimitCompare, 0, upperLimitMux, boolType);
    new_arcs.push_back(upperLimitCompareToUpperLimitMux);

    std::shared_ptr<Arc> upperLimitToUpperLimitMux = Arc::connectNodes(upperLimitNode, 0, upperLimitMux, 0, intermediateType);
    new_arcs.push_back(upperLimitToUpperLimitMux);

    std::shared_ptr<Arc> inputToUpperLimitMux = Arc::connectNodes(inputSrcNode, inputSrcNodeOutputPortNumber, upperLimitMux, 1, intermediateType);
    new_arcs.push_back(inputToUpperLimitMux);

    std::shared_ptr<Arc> upperLimitMuxToLowerLimitMux = Arc::connectNodes(upperLimitMux, 0, lowerLimitMux, 1, intermediateType);
    new_arcs.push_back(upperLimitMuxToLowerLimitMux);

    std::shared_ptr<Arc> lowerLimitToLowerLimitMux = Arc::connectNodes(lowerLimitNode, 0, lowerLimitMux, 0, intermediateType);
    new_arcs.push_back(lowerLimitToLowerLimitMux);

    std::shared_ptr<Arc> inputToLowerLimitCompare = Arc::connectNodes(inputSrcNode, inputSrcNodeOutputPortNumber, lowerLimitCompare, 0, intermediateType);
    new_arcs.push_back(inputToLowerLimitCompare);

    std::shared_ptr<Arc> lowerLimitToLowerLimitCompare = Arc::connectNodes(lowerLimitNode, 0, lowerLimitCompare, 1, intermediateType);
    new_arcs.push_back(lowerLimitToLowerLimitCompare);

    std::shared_ptr<Arc> lowerLimitCompareToLowerLimitMux = Arc::connectNodes(lowerLimitCompare, 0, lowerLimitMux, boolType);
    new_arcs.push_back(lowerLimitCompareToLowerLimitMux);

    if(complex){
        //The output node is the realImagToComplex, use port 0 for real
        std::shared_ptr<Arc> lowerLimitMuxToOut = Arc::connectNodes(lowerLimitMux, 0, realImagToComplexNode, 0, intermediateType);
        new_arcs.push_back(lowerLimitMuxToOut);
    }else{
        //Rewire Outputs to Lower Limit Mux
        std::set<std::shared_ptr<Arc>> outputArcs = outputPorts[0]->getArcs(); //Make a copy of the arc set since re-assigning the arc will remove it from the port's set
        for(auto outputArc = outputArcs.begin(); outputArc != outputArcs.end(); outputArc++){
            lowerLimitMux->addOutArcUpdatePrevUpdateArc(0, *outputArc);
        }
    }

    //Imagionary side - A slightly modified version of the above logic
    if(complex){
        NumericValue lowerLimitImag = lowerLimit;
        if(lowerLimitImag.isFractional()){
            std::complex<double> val = lowerLimitImag.getComplexDouble();
            val.real(val.imag());
            val.imag(0);
            lowerLimitImag.setComplexDouble(val);
        }else{
            lowerLimitImag.setRealInt(lowerLimitImag.getImagInt());
            lowerLimitImag.setImagInt(0);
        }
        lowerLimitImag.setComplex(false);

        NumericValue upperLimitImag = upperLimit;
        if(upperLimitImag.isFractional()){
            std::complex<double> val = upperLimitImag.getComplexDouble();
            val.real(val.imag());
            val.imag(0);
            upperLimitImag.setComplexDouble(val);
        }else{
            upperLimitImag.setRealInt(upperLimitImag.getImagInt());
            upperLimitImag.setImagInt(0);
        }
        upperLimitImag.setComplex(false);

        std::shared_ptr<Constant> upperLimitNode = NodeFactory::createNode<Constant>(expandedNode);
        upperLimitNode->setName("UpperLimitImag");
        upperLimitNode->setValue(std::vector<NumericValue>{upperLimitImag});
        new_nodes.push_back(upperLimitNode);

        std::shared_ptr<Constant> lowerLimitNode = NodeFactory::createNode<Constant>(expandedNode);
        lowerLimitNode->setName("LowerLimitImag");
        lowerLimitNode->setValue(std::vector<NumericValue>{lowerLimitImag});
        new_nodes.push_back(lowerLimitNode);

        std::shared_ptr<Compare> upperLimitCompare = NodeFactory::createNode<Compare>(expandedNode);
        upperLimitCompare->setName("UpperLimitImagCompare");
        upperLimitCompare->setCompareOp(Compare::CompareOp::GT);
        new_nodes.push_back(upperLimitCompare);

        std::shared_ptr<Compare> lowerLimitCompare = NodeFactory::createNode<Compare>(expandedNode);
        lowerLimitCompare->setName("LowerLimitImagCompare");
        lowerLimitCompare->setCompareOp(Compare::CompareOp::LT);
        new_nodes.push_back(lowerLimitCompare);

        std::shared_ptr<Mux> upperLimitMux = NodeFactory::createNode<Mux>(expandedNode);
        upperLimitMux->setName("UpperLimitImagMux");
        upperLimitMux->setBooleanSelect(true); //Treat port 0 as the true port
        new_nodes.push_back(upperLimitMux);

        std::shared_ptr<Mux> lowerLimitMux = NodeFactory::createNode<Mux>(expandedNode);
        lowerLimitMux->setName("LowerLimitImagMux");
        lowerLimitMux->setBooleanSelect(true); //Treat port 0 as the true port
        new_nodes.push_back(lowerLimitMux);

        //The input node is the complexToRealImag, use port 1 for imag
        std::shared_ptr<Arc> inputToUpperLimitCompare = Arc::connectNodes(inputSrcNode, 1, upperLimitCompare, 0, intermediateType);
        new_arcs.push_back(inputToUpperLimitCompare);

        std::shared_ptr<Arc> upperLimitToUpperLimitCompare = Arc::connectNodes(upperLimitNode, 0, upperLimitCompare, 1, intermediateType);
        new_arcs.push_back(upperLimitToUpperLimitCompare);

        std::shared_ptr<Arc> upperLimitCompareToUpperLimitMux = Arc::connectNodes(upperLimitCompare, 0, upperLimitMux, boolType);
        new_arcs.push_back(upperLimitCompareToUpperLimitMux);

        std::shared_ptr<Arc> upperLimitToUpperLimitMux = Arc::connectNodes(upperLimitNode, 0, upperLimitMux, 0, intermediateType);
        new_arcs.push_back(upperLimitToUpperLimitMux);

        std::shared_ptr<Arc> inputToUpperLimitMux = Arc::connectNodes(inputSrcNode, 1, upperLimitMux, 1, intermediateType);
        new_arcs.push_back(inputToUpperLimitMux);

        std::shared_ptr<Arc> upperLimitMuxToLowerLimitMux = Arc::connectNodes(upperLimitMux, 0, lowerLimitMux, 1, intermediateType);
        new_arcs.push_back(upperLimitMuxToLowerLimitMux);

        std::shared_ptr<Arc> lowerLimitToLowerLimitMux = Arc::connectNodes(lowerLimitNode, 0, lowerLimitMux, 1, intermediateType);
        new_arcs.push_back(lowerLimitToLowerLimitMux);

        std::shared_ptr<Arc> inputToLowerLimitCompare = Arc::connectNodes(inputSrcNode, 1, lowerLimitCompare, 0, intermediateType);
        new_arcs.push_back(inputToLowerLimitCompare);

        std::shared_ptr<Arc> lowerLimitToLowerLimitCompare = Arc::connectNodes(lowerLimitNode, 0, lowerLimitCompare, 1, intermediateType);
        new_arcs.push_back(lowerLimitToLowerLimitCompare);

        std::shared_ptr<Arc> lowerLimitCompareToLowerLimitMux = Arc::connectNodes(lowerLimitCompare, 0, lowerLimitMux, boolType);
        new_arcs.push_back(lowerLimitCompareToLowerLimitMux);

        //The output node is the realImagToComplex, use port 1 for imag
        std::shared_ptr<Arc> lowerLimitMuxToOut = Arc::connectNodes(lowerLimitMux, 0, realImagToComplexNode, 1, intermediateType);
        new_arcs.push_back(lowerLimitMuxToOut);
    }

    return expandedNode;
}

Saturate::Saturate(std::shared_ptr<SubSystem> parent, Saturate* orig) : MediumLevelNode(parent, orig), lowerLimit(orig->lowerLimit), upperLimit(orig->upperLimit){

}

std::shared_ptr<Node> Saturate::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<Saturate>(parent, this);
}

