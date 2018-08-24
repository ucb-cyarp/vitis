//
// Created by Christopher Yarp on 7/16/18.
//

#include "Gain.h"

#include "GraphCore/ExpandedNode.h"
#include "PrimitiveNodes/Product.h"
#include "PrimitiveNodes/Constant.h"
#include "General/GeneralHelper.h"
#include <cmath>

Gain::Gain() {

}

Gain::Gain(std::shared_ptr<SubSystem> parent) : MediumLevelNode(parent) {

}

std::vector<NumericValue> Gain::getGain() const {
    return gain;
}

void Gain::setGain(const std::vector<NumericValue> &gain) {
    Gain::gain = gain;
}

std::shared_ptr<Gain>
Gain::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                        std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node & Set Common Properties ====
    std::shared_ptr<Gain> newNode = NodeFactory::createNode<Gain>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    std::string gainStr;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- Gain
        gainStr = dataKeyValueMap.at("Gain");
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Names -- Numeric.Gain
        gainStr = dataKeyValueMap.at("Numeric.Gain");
    } else
    {
        throw std::runtime_error("Unsupported Dialect when parsing XML - Gain");
    }

    std::vector<NumericValue> gainValue = NumericValue::parseXMLString(gainStr);

    newNode->setGain(gainValue);

    return newNode;
}

std::set<GraphMLParameter> Gain::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("Gain", "string", true));

    return parameters;
}

xercesc::DOMElement *
Gain::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "Gain");

    GraphMLHelper::addDataNode(doc, thisNode, "Gain", NumericValue::toString(gain));

    return thisNode;
}

std::string Gain::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: Gain\nGain:" + NumericValue::toString(gain);

    return label;
}

void Gain::validate() {
    Node::validate();

    if(inputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - Gain - Should Have Exactly 1 Input Port");
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - Gain - Should Have Exactly 1 Output Port");
    }

    //Check that there is at least 1 value
    if(gain.size() < 1){
        throw std::runtime_error("Validation Failed - Gain - Should Have At Least 1 Gain Value");
    }

    //Check that if any input is complex, the result is complex
    std::shared_ptr<InputPort> inputPort = getInputPort(0);

    if(inputPort->getDataType().isComplex()) {
        DataType outType = getOutputPort(0)->getDataType();
        if(!outType.isComplex()){
            throw std::runtime_error("Validation Failed - Gain - Input Port is Complex but Output is Real");
        }
    }
}

bool Gain::expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes,
                  std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs) {

    //Validate first to check that Gain is properly wired (ie. there is the proper number of ports, only 1 input arc, etc.)
    validate();

    //---- Expand the Gain Node to a multiply and constant ----
    std::shared_ptr<SubSystem> thisParent = parent;

    //Create Expanded Node and Add to Parent
    std::shared_ptr<ExpandedNode> expandedNode = NodeFactory::createNode<ExpandedNode>(thisParent, shared_from_this());

    //Remove Current Node from Parent and Set Parent to nullptr
    thisParent->removeChild(shared_from_this());
    parent = nullptr;
    //Add This node to the list of nodes to remove from the node vector
    deleted_nodes.push_back(shared_from_this());

    //Add Expanded Node to Node List
    new_nodes.push_back(expandedNode);

    //++++ Create Multiply Block and Rewire ++++
    std::shared_ptr<Product> multiplyNode = NodeFactory::createNode<Product>(thisParent);
    multiplyNode->setName("Multiply");
    multiplyNode->setInputOp({true, true}); //This is a multiply for 2 inputs
    new_nodes.push_back(multiplyNode);

    std::shared_ptr<Arc> inputArc = *(inputPorts[0]->getArcs().begin());
    multiplyNode->addInArcUpdatePrevUpdateArc(0, inputArc); //Should remove the arc from this node's port

    std::set<std::shared_ptr<Arc>> outputArcs = outputPorts[0]->getArcs(); //Make a copy of the arc set since re-assigning the arc will remove it from the port's set
    for(auto outputArc = outputArcs.begin(); outputArc != outputArcs.end(); outputArc++){
        multiplyNode->addOutArcUpdatePrevUpdateArc(0, *outputArc);
    }

    //++++ Create Constant Node and Wire ++++
    std::shared_ptr<Constant> constantNode = NodeFactory::createNode<Constant>(parent);
    constantNode->setName("Constant");
    constantNode->setValue(gain);
    new_nodes.push_back(constantNode);

    /*
     * Constant Type Logic:
     *   - If output type is a floating point type, the constant takes on the same type as the output
     *   - If the 1st input is a floating point type, the constant takes the same type as the 1st input.
     *   - If the output is an integer type, the constant takes the smallest integer type which accommodates the constant.
     *   - If the output is a fixed point type, the constant takes on a fixed point type with the integer portion is the smallest that supports the given constant
     *     the fractional bits are the max(fractional bits in output, fractional bits in 1st input).
     *
     * Signed is determined by checking if the constant value is negative, if the 1st input is negative
     *
     * Complexity and width are taken from the gain NumericValue
     */
    DataType outputType = (*outputArcs.begin())->getDataType();
    DataType firstInputType = inputArc->getDataType();


    DataType constantType = DataType();

    if(outputType.isFloatingPt()){
        //Floating Pt
        constantType = outputType;
    }else if(firstInputType.isFloatingPt()) {
        constantType = firstInputType;
    }else{
        //Integer or Fixed Point

        //Calc Integer Bits and Check if Constant is Signed
        unsigned long numIntBits = 0;
        bool constSigned = false;

        for (auto constant = gain.begin(); constant != gain.end(); constant++) {
            unsigned long localNumBits = constant->numIntegerBits();
            if (localNumBits > numIntBits) {
                numIntBits = localNumBits;
            }

            constSigned |= constant->isSigned();
        }

        if (outputType.getFractionalBits() == 0) {
            //Integer
            numIntBits = GeneralHelper::roundUpToCPUBits(numIntBits);

            //Set type information
            constantType.setFloatingPt(false);
            constantType.setTotalBits(numIntBits);
            constantType.setFractionalBits(0);
        }else {
            //Fixed Point
            unsigned long firstInputFractionalBits = firstInputType.getFractionalBits();
            unsigned long outputFractionalBits = outputType.getFractionalBits();

            unsigned long fractionalBits = firstInputFractionalBits > outputFractionalBits ? firstInputFractionalBits : outputFractionalBits;

            unsigned long totalBits = fractionalBits + numIntBits;

            constantType.setFloatingPt(false);
            constantType.setTotalBits(totalBits);
            constantType.setFractionalBits(fractionalBits);
        }

        constantType.setSignedType(constSigned); //This must be set for int or Fixed Point types.  Floating point should be true

    }

    //Get complexity and width from value
    //Width can be different from output width (ex. vector*scalar = vector)
    constantType.setWidth(gain.size());
    constantType.setComplex(gain[0].isComplex());

    //Connect constant node to multiply node
    std::shared_ptr<Arc> constantArc = Arc::connectNodes(constantNode, 0, multiplyNode, 1, constantType);
    new_arcs.push_back(constantArc);

    return true;
}

