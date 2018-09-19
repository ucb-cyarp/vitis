//
// Created by Christopher Yarp on 7/23/18.
//

#include "CompareToConstant.h"
#include "GraphCore/ExpandedNode.h"
#include "PrimitiveNodes/Constant.h"
#include "General/GeneralHelper.h"
#include "GraphCore/NodeFactory.h"

CompareToConstant::CompareToConstant() : compareOp(Compare::CompareOp::LT) {

}

CompareToConstant::CompareToConstant(std::shared_ptr<SubSystem> parent) : MediumLevelNode(parent), compareOp(Compare::CompareOp::LT) {

}

std::vector<NumericValue> CompareToConstant::getCompareConst() const {
    return compareConst;
}

void CompareToConstant::setCompareConst(const std::vector<NumericValue> &compareConst) {
    CompareToConstant::compareConst = compareConst;
}

Compare::CompareOp CompareToConstant::getCompareOp() const {
    return compareOp;
}

void CompareToConstant::setCompareOp(Compare::CompareOp compareOp) {
    CompareToConstant::compareOp = compareOp;
}

std::shared_ptr<CompareToConstant>
CompareToConstant::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                                     std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    //==== Create Node & Set Common Properties ====
    std::shared_ptr<CompareToConstant> newNode = NodeFactory::createNode<CompareToConstant>(parent);
    newNode->setId(id);
    newNode->setName(name);

    //==== Import important properties ====
    std::string compareConstStr;
    std::string compareOpStr;

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- CompareConst, CompareOp
        compareConstStr = dataKeyValueMap.at("CompareConst");
        compareOpStr = dataKeyValueMap.at("CompareOp");

    //NOTE: While compare to constant exists in Simulink, it is a subsystem and will be parsed as primitive elements
    //Simulink is not a supported dialect for this block
    } else
    {
        throw std::runtime_error("Unsupported Dialect when parsing XML - CompareToConstant");
    }

    std::vector<NumericValue> compareConstVal = NumericValue::parseXMLString(compareConstStr);
    Compare::CompareOp compareOpVal = Compare::parseCompareOpString(compareOpStr);

    newNode->setCompareConst(compareConstVal);
    newNode->setCompareOp(compareOpVal);

    return newNode;
}

bool CompareToConstant::expand(std::vector<std::shared_ptr<Node>> &new_nodes,
                               std::vector<std::shared_ptr<Node>> &deleted_nodes,
                               std::vector<std::shared_ptr<Arc>> &new_arcs,
                               std::vector<std::shared_ptr<Arc>> &deleted_arcs) {

    //Validate first to check that the CompareToConstant block is properly wired
    validate();

    //---- Expand the CompareToConstant node into a compare and constant ----
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

    //++++ Create Compare Block and Rewire ++++
    std::shared_ptr<Compare> compareNode = NodeFactory::createNode<Compare>(thisParent);
    compareNode->setName("Compare");
    compareNode->setCompareOp(compareOp); //Set the proper comparison operation
    new_nodes.push_back(compareNode);

    std::shared_ptr<Arc> inputArc = *(inputPorts[0]->getArcs().begin());
    compareNode->addInArcUpdatePrevUpdateArc(0, inputArc); //Should remove the arc from this node's port

    std::set<std::shared_ptr<Arc>> outputArcs = outputPorts[0]->getArcs(); //Make a copy of the arc set since re-assigning the arc will remove it from the port's set
    for(auto outputArc = outputArcs.begin(); outputArc != outputArcs.end(); outputArc++){
        compareNode->addOutArcUpdatePrevUpdateArc(0, *outputArc);
    }

    //++++ Create Constant Node and Wire ++++
    std::shared_ptr<Constant> constantNode = NodeFactory::createNode<Constant>(parent);
    constantNode->setName("Constant");
    constantNode->setValue(compareConst); //set the proper constant to compare to
    new_nodes.push_back(constantNode);

    /*
     * Constant Type Logic:
     *   - If input type is a floating point type, the constant takes on the same type as the input
     *   - If the input is an integer type, the constant takes the smallest integer type which accommodates the constant.
     *   - If the input is a fixed point type, the constant takes on a fixed point type with the integer portion is the smallest that supports the given constant
     *     the number of fractional bits are the fractional bits in 1st input
     *
     *   Complexity and width are taken from the constant NumericValue
     */

    DataType firstInputType = inputArc->getDataType();

    DataType constantType = DataType();

    if(firstInputType.isFloatingPt()){
        //Floating Point
        constantType = firstInputType;
    }else{
        //Int or Fixed Pt

        //Calc Integer Bits and Check if Constant is Signed
        unsigned long numIntBits = 0;
        bool constSigned = false;

        for (auto constant = compareConst.begin(); constant != compareConst.end(); constant++) {
            unsigned long localNumBits = constant->numIntegerBits();
            if (localNumBits > numIntBits) {
                numIntBits = localNumBits;
            }

            constSigned |= constant->isSigned();
        }

        if (firstInputType.getFractionalBits() == 0) {
            //Integer
            numIntBits = GeneralHelper::roundUpToCPUBits(numIntBits);

            //Set type information
            constantType.setFloatingPt(false);
            constantType.setTotalBits(numIntBits);
            constantType.setFractionalBits(0);
        }else {
            //Fixed Point
            unsigned long fractionalBits = firstInputType.getFractionalBits();

            unsigned long totalBits = fractionalBits + numIntBits;

            constantType.setFloatingPt(false);
            constantType.setTotalBits(totalBits);
            constantType.setFractionalBits(fractionalBits);
        }

        constantType.setSignedType(constSigned); //This must be set for int or Fixed Point types.  Floating p
    }

    //Get complexity and width from value
    //Width can be different from output width (ex. vector*scalar = vector)
    constantType.setWidth(compareConst.size());
    constantType.setComplex(compareConst[0].isComplex());

    //Connect constant node to compare node
    std::shared_ptr<Arc> constantArc = Arc::connectNodes(constantNode, 0, compareNode, 1, constantType);
    new_arcs.push_back(constantArc);

    return true;
}

std::set<GraphMLParameter> CompareToConstant::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("CompareConst", "string", true));
    parameters.insert(GraphMLParameter("CompareOp", "string", true));

    return parameters;
}

xercesc::DOMElement *CompareToConstant::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode,
                                                    bool include_block_node_type) {
    //Create Node
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);

    //Add Parameters / Attributes to Node
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "CompareToConstant");

    GraphMLHelper::addDataNode(doc, thisNode, "CompareConst", NumericValue::toString(compareConst));
    GraphMLHelper::addDataNode(doc, thisNode, "CompareOp", Compare::compareOpToString(compareOp));

    return thisNode;
}

std::string CompareToConstant::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: CompareToConstant\nCompareOp: " + Compare::compareOpToString(compareOp) + "\nCompareConst:" + NumericValue::toString(compareConst);

    return label;
}

void CompareToConstant::validate() {
    Node::validate();

    if(inputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - CompareToConstant - Should Have Exactly 1 Input Port");
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - CompareToConstant - Should Have Exactly 1 Output Port");
    }

    //Check that there is at least 1 value for the comparison
    if(compareConst.size() < 1){
        throw std::runtime_error("Validation Failed - CompareToConstant - Should Have At Least 1 Constant Value");
    }
}

CompareToConstant::CompareToConstant(std::shared_ptr<SubSystem> parent, CompareToConstant* orig) : MediumLevelNode(parent, orig), compareConst(orig->compareConst), compareOp(orig->compareOp) {

}

std::shared_ptr<Node> CompareToConstant::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<CompareToConstant>(parent, this);
}


