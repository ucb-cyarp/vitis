//
// Created by Christopher Yarp on 6/27/18.
//

#include "GraphMLImporter.h"
#include "GraphMLHelper.h"

#include "GraphCore/NodeFactory.h"
#include "GraphCore/EnableInput.h"
#include "GraphCore/EnableOutput.h"
#include "GraphCore/ExpandedNode.h"
#include "GraphCore/SubSystem.h"
#include "GraphCore/EnabledSubSystem.h"
#include "MasterNodes/MasterInput.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterUnconnected.h"

#include "PrimitiveNodes/Sum.h"
#include "PrimitiveNodes/Product.h"
#include "PrimitiveNodes/Delay.h"
#include "PrimitiveNodes/Constant.h"
#include "PrimitiveNodes/Mux.h"
#include "PrimitiveNodes/DataTypeConversion.h"
#include "PrimitiveNodes/Compare.h"
#include "PrimitiveNodes/LUT.h"
#include "PrimitiveNodes/ComplexToRealImag.h"
#include "PrimitiveNodes/RealImagToComplex.h"
#include "PrimitiveNodes/DataTypeDuplicate.h"
#include "MediumLevelNodes/Gain.h"
#include "MediumLevelNodes/CompareToConstant.h"
#include "MediumLevelNodes/ThresholdSwitch.h"
#include "MediumLevelNodes/SimulinkMultiPortSwitch.h"
#include "HighLevelNodes/DiscreteFIR.h"
#include "BusNodes/VectorFan.h"
#include "BusNodes/VectorFanIn.h"
#include "BusNodes/VectorFanOut.h"

#include <iostream>
#include <fstream>
#include <string>

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>

using namespace xercesc;

std::unique_ptr<Design> GraphMLImporter::importGraphML(std::string filename, GraphMLDialect dialect)
{
    //Check if input exists (see https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c)
    std::ifstream inputFile;
    inputFile.open(filename);
    if(!inputFile.good()){
        inputFile.close();

        throw std::runtime_error("Error: Input file does not exist: " + filename);
    }
    inputFile.close();


    //Is a 3 phase pass over the GraphML file.  These passes may be combined

    //Pass 1 (Optional): Get attribute descriptions
    //    These may not be used as we know the types of the parameters we care about.  We may in fact override these
    //    parameters.  The parameters are needed, however, for external tools to parse the GraphML file and therefor
    //    need to be included.

    //Pass 2: Get nodes.  Create node objects.  Add node/nodeIDs to a map.  Add to design.
    //    Since nested graphs must be declared within the parent node's node context, parent nodes should always be
    //    declared before their child nodes.  I believe that ID numbers are only required if nodes are referred to.
    //    During the traversal, we keep track of the highest node ID number.  After the

    //Pass 3: Get Edges.  Create edge objects and link to referenced nodes.  Add to design.
    //    Ideally, this can be done in sequence with the node pass.  However, I cannot find a statement in the spec
    //    that requires edges to be created after the nodes they reference.  The only restriction I can see is that
    //    edges should either be declared in the highest level or in the lowest common ancestor graph containing both
    //    nodes.  In order to handle cases where edges can be declared out of

    //Now, we do not actually need to conduct these passes separately.  During the node pass, references to edge objects
    //can be saved into an array to parse later.  This avoids needing to re-traverse the entire DOM.

    //This function uses the Apache Xerces XML Parser
    //Some very helpful examples that were referenced when writing this function were
    //DOMPrint, DOMCount, and CreateDOMDocument

    //This function does not perform schema validation or exploit some of the various advanced options Xerces provides.

    //Note on Xerces.  Need to make sure all Xerces objects are released before calling the terminate function.

    //From Example:
    try
    {
        XMLPlatformUtils::Initialize();
    }
    catch (const XMLException& toCatch)
    {
        char* error_msg = XMLString::transcode(toCatch.getMessage());
        std::cerr << "Error during initialization! :\n" << error_msg << std::endl;
        XMLString::release(&error_msg);
        throw std::runtime_error("XML Parser Env could not initialize.  Could not import GraphML File.");
    }

    //From https://xerces.apache.org/xerces-c/program-dom-3.html and DOMCount.cpp
    XMLCh LS_literal[] = {chLatin_L, chLatin_S, chNull};
    DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(LS_literal);
    DOMLSParser* parser = ((DOMImplementationLS*)impl)->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, nullptr);

    // optionally you can set some features on this builder
    if (parser->getDomConfig()->canSetParameter(XMLUni::fgDOMValidate, false))
        parser->getDomConfig()->setParameter(XMLUni::fgDOMValidate, false);
    if (parser->getDomConfig()->canSetParameter(XMLUni::fgDOMNamespaces, true))
        parser->getDomConfig()->setParameter(XMLUni::fgDOMNamespaces, true);

    //According to https://xerces.apache.org/xerces-c/apiDocs-3/classDOMConfiguration.html, there is a method to remove
    //whitespace only nodes.  However, it makes reference to this only occurring during normalization
//    if (parser->getDomConfig()->canSetParameter(XMLUni::fgDOMElementContentWhitespace, false))
//        parser->getDomConfig()->setParameter(XMLUni::fgDOMElementContentWhitespace, false);


    DOMDocument *doc = nullptr;

    //Try Parsing File
    try {
        doc = parser->parseURI(filename.c_str());
    }
    //Catch the various exceptions
    catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        std::cerr << "Exception message is:" << std::endl << message << std::endl;
        XMLString::release(&message);
        throw std::runtime_error("XML Parsing Failed Due to XML Exception");
    }
    catch (const DOMException& toCatch) {
        char* message = XMLString::transcode(toCatch.msg);
        std::cout << "Exception message is:" << std::endl << message << std::endl;
        XMLString::release(&message);
        throw std::runtime_error("XML Parsing Failed Due to DOM Exception");
    }
    catch (...) {
        std::cerr << "Unexpected Exception" << std::endl;
        throw std::runtime_error("XML Parsing Failed Due to an Unknown Exception");
    }

    //Create Design Object
    std::unique_ptr<Design> design = std::unique_ptr<Design>(new Design());

    //It was suggested by Xerces that scoping be used to ensure proper destruction of Xerces objects before Terminate
    //function is called
    {
        DOMNode *node = doc;

        /*NOTE: It appears that each node has an accompanying TEXT node, even if there was not text in the node.
         *    When parsing, be cognisant that the data between the <></> nodes are in the child TEXT node for that
         *    node.  The data is in the value field of the text node.  However, attributes have name/value pairs
         *    in nodes returned by the attribute map.  See https://xerces.apache.org/xerces-c/apiDocs-3/classDOMNode.html
         *    for more info.
         *
         *Followup: The text nodes are being inserted because of whitespace in the XML file.
         *          For example, the xml fragment <node1><node2></node2><node1> does not include text nodes.
         *          However, <node1>
         *                       <node2><node2>
         *                   </node1>
         *          includes 2 Text nodes for node1, before and after the node2 child entry.
         *          If two newlines are used instead of 1 (to space out node2 from the node1 tags) there are still only
         *          2 text nodes but they both consist of 2 newline characters instead of 1.
         *
         *          This is expected behavior based on http://www.oracle.com/technetwork/articles/wang-whitespace-092897.html
         *          since XML treats whitespace as significant unless the parser is told otherwise (or the schema specifies
         *          otherwise).  There are some settings in Xerces concerning whitespace but they are part of the normalization
         *          routines.
         *
         *          Inserting whitespace text nodes will likely be important when emitting readable XML.
         */

        //Create Map and Edge Vector to be Populated Durring Node Traversal
        std::map<std::string, std::shared_ptr<Node>> nodeMap;
        std::vector<DOMNode*> edgeNodes;

        int numNodesImported = GraphMLImporter::importNodes(node, *design, nodeMap, edgeNodes, dialect);

        #ifdef DEBUG
        std::cout << "Nodes Imported: " << numNodesImported << std::endl;
        #endif

        int numArcsImported = importEdges(edgeNodes, *design, nodeMap, dialect);

        #ifdef DEBUG
        std::cout << "Arcs Imported: " << numArcsImported << std::endl;
        #endif

        //Propagate in nodes
        std::vector<std::shared_ptr<Node>> nodes = design->getNodes();
        for(auto node = nodes.begin(); node != nodes.end(); node++){
            (*node)->propagateProperties();
        }

        //Validate nodes (get the node vector again in case nodes were added)
        nodes = design->getNodes();
        for(auto node = nodes.begin(); node != nodes.end(); node++){
            (*node)->validate();
        }
    }


    //Cleanup
    //doc->release(); //Release the document
    parser->release(); //Should release the objects created by the parser

    XMLPlatformUtils::Terminate();

    return design;
}

std::string GraphMLImporter::getTextValueOfNode(xercesc::DOMNode *node)
{
    if(node->hasChildNodes()){
        DOMNode* child = node->getFirstChild();

        if(child->getNodeType() == DOMNode::NodeType::TEXT_NODE){
            std::string nodeValStr = GraphMLHelper::getTranscodedString(child->getNodeValue());

            DOMNode* nextChild = child->getNextSibling();
            if(nextChild == nullptr){
                return nodeValStr;
            }else{
                //More than 1 child
                return "";
            }
        }
        else{
            //Not a text node
            return "";
        }

    } else{
        return "";
    }

}

DOMNode* GraphMLImporter::graphMLDataMap(xercesc::DOMNode *node, std::map<std::string, std::string> &dataMap){
    DOMNode* subgraph = nullptr;

    if(node->hasChildNodes()){
        for(DOMNode* child = node->getFirstChild(); child != nullptr; child = child->getNextSibling()){
            if(child->getNodeType() == DOMNode::NodeType::ELEMENT_NODE) {
                std::string childName = GraphMLHelper::getTranscodedString(child->getNodeName());

                if (childName == "graph") {
                    if (subgraph != nullptr) {
                        throw std::runtime_error("Node with multiple graph children encountered");
                    }
                    subgraph = child;
                } else if (childName == "data") {
                    //This is a data node which we will add to the map

                    std::string dataKey;

                    //The key is an attribute
                    if(child->hasAttributes()) {
                        DOMNamedNodeMap *nodeAttributes = child->getAttributes();

                        XMLCh *keyXMLCh = XMLString::transcode("key");
                        DOMNode *keyNode = nodeAttributes->getNamedItem(keyXMLCh);
                        XMLString::release(&keyXMLCh);

                        if (keyNode == nullptr) {
                            throw std::runtime_error("Data node with no key encountered");
                        } else {
                            dataKey = GraphMLHelper::getTranscodedString(keyNode->getNodeValue());
                        }
                    }

                    //The value is in the text node
                    std::string dataValue = getTextValueOfNode(child);

                    dataMap[dataKey] = dataValue;
                }
            }
            //Ignore other node types
        }
    }

    return subgraph;
}

xercesc::DOMNode *
GraphMLImporter::graphMLDataAttributeMap(xercesc::DOMNode *node, std::map<std::string, std::string> &attributeMap, std::map<std::string, std::string> &dataMap) {
    //Get attributes from given node and add to map
    if(node->hasAttributes())
    {
        DOMNamedNodeMap *nodeAttributes = node->getAttributes();
        XMLSize_t numAttributes = nodeAttributes->getLength();

        for (XMLSize_t i = 0; i < numAttributes; i++) {
            DOMNode *attribute = nodeAttributes->item(i);
            std::string attrName = GraphMLHelper::getTranscodedString(attribute->getNodeName());

            const XMLCh *attrVal = attribute->getNodeValue();

            std::string attrValueStr;

            if(attrVal != nullptr)
            {
                attrValueStr = GraphMLHelper::getTranscodedString(attrVal);
            }
            else
            {
                attrValueStr = "";
            }
            attributeMap[attrName] = attrValueStr;
        }
    }

    DOMNode* subgraph = GraphMLImporter::graphMLDataMap(node, dataMap);

    return subgraph;
}


int GraphMLImporter::importNodes(xercesc::DOMNode *node, Design &design, std::map<std::string, std::shared_ptr<Node>> &nodeMap, std::vector<xercesc::DOMNode*> &edgeNodes, GraphMLDialect dialect){
    return GraphMLImporter::importNodes(node, design, nodeMap, edgeNodes, std::shared_ptr<SubSystem>(nullptr), dialect);
}

int GraphMLImporter::importNodes(xercesc::DOMNode *node, Design &design, std::map<std::string, std::shared_ptr<Node>> &nodeMap, std::vector<xercesc::DOMNode*> &edgeNodes, std::shared_ptr<SubSystem> parent, GraphMLDialect dialect)
{
    int importedNodes = 0;

    //Check for nullptr
    if(node != nullptr)
    {
        DOMNode::NodeType nodeType = node->getNodeType();

        //==== Parse Different Node Types ====
        if(nodeType==DOMNode::NodeType::DOCUMENT_NODE) {
            //This is the root of the DOM document.  Traverse the children
            if(node->hasChildNodes()) {
                for(DOMNode* child = node->getFirstChild(); child != nullptr; child = child->getNextSibling()){
                    importedNodes += GraphMLImporter::importNodes(child, design, nodeMap, edgeNodes, parent, dialect);
                }
            }
        } else if(nodeType==DOMNode::NodeType::COMMENT_NODE) {
            //Encountered a comment node, don't do anything
        } else if(nodeType==DOMNode::NodeType::TEXT_NODE) {
            //Encountering a Text node that is not directly handled by an element node in this parser should be ignored
            //It is likely whitespace

            //TODO: Should we actually check this?
            std::string text = GraphMLHelper::getTranscodedString(node->getNodeValue()); //Text of Test Node is NodeValue

            int textLen = text.size();
            for(int i = 0; i<textLen; i++){
                if(!std::isspace(text[i])){
                    throw std::runtime_error("Encountered a text node that is not whitespace");
                }
            }
        } else if(nodeType==DOMNode::NodeType::ELEMENT_NODE) {
            //==== Parse Element Node Types ====
            std::string nodeName = GraphMLHelper::getTranscodedString(node->getNodeName());

            if(nodeName=="graphml") {
                //This is the root of the design.
                //Right now, we do not look at the attributes
                //Itterate over all children
                if(node->hasChildNodes()){
                    for(DOMNode* child = node->getFirstChild(); child != nullptr; child = child->getNextSibling()){
                        importedNodes += GraphMLImporter::importNodes(child, design, nodeMap, edgeNodes, parent, dialect);
                    }
                }

            } else if(nodeName=="key") {
                //For now, we do not parse these keys

            } else if(nodeName=="graph") {
                //This is a graph node, it contains other nodes below it.  Process them
                if(node->hasChildNodes()){
                    for(DOMNode* child = node->getFirstChild(); child != nullptr; child = child->getNextSibling()){
                        importedNodes += GraphMLImporter::importNodes(child, design, nodeMap, edgeNodes, parent, dialect);
                    }
                }

            } else if(nodeName=="edge") {
                //Encountered an edge node.  For now, just add it to the edge vector
                edgeNodes.push_back(node);

            } else if(nodeName=="node") {
                importedNodes += GraphMLImporter::importNode(node, design, nodeMap, edgeNodes, parent, dialect);

            } else if(nodeName=="data") {
                throw std::runtime_error("Encountered a GraphML data node in an unexpected location");
            } else {
                throw std::runtime_error("Encountered an unknown GraphML tag: " + nodeName);
            }

        } else{
            throw std::runtime_error("Encountered a text node that is not whitespace");
        }
    }

    return importedNodes;

}

int GraphMLImporter::importNode(DOMNode *node, Design &design, std::map<std::string, std::shared_ptr<Node>> &nodeMap, std::vector<DOMNode*> &edgeNodes, std::shared_ptr<SubSystem> parent, GraphMLDialect dialect){
    //Now need to parse the different types of nodes

    int nodesImported = 1; //1 for this node

    //Get common info: ID
    std::string fullNodeID;

    if(node->hasAttributes()) {
        DOMNamedNodeMap *nodeAttributes = node->getAttributes();

        XMLCh* idXMLCh = XMLString::transcode("id");
        DOMNode* idNode = nodeAttributes->getNamedItem(idXMLCh);
        XMLString::release(&idXMLCh);

        if(idNode == nullptr){
            throw std::runtime_error("Node with no ID encountered");
        }
        else{
            fullNodeID = GraphMLHelper::getTranscodedString(idNode->getNodeValue());
        }


    }else
    {
        throw std::runtime_error("Node with no ID encountered");
    }

    //Construct a map of data values (children) of this node
    std::map<std::string, std::string> dataKeyValueMap;
    DOMNode* subgraph = GraphMLImporter::graphMLDataMap(node, dataKeyValueMap);

    //Get the human readable instance name if one exists
    std::string name = "";
    bool hasName = false;

    if(dataKeyValueMap.find("instance_name") != dataKeyValueMap.end()){
        name = dataKeyValueMap["instance_name"];
        hasName = true;
    }

    //Based of the node type, we construct nodes:
    std::string blockType = dataKeyValueMap.at("block_node_type");

    if(blockType == "Subsystem"){
        std::shared_ptr<SubSystem> newSubsystem = NodeFactory::createNode<SubSystem>(parent);
        newSubsystem->setId(Node::getIDFromGraphMLFullPath(fullNodeID));
        if(hasName){
            newSubsystem->setName(name);
        }

        //Add node to design
        design.addNode(newSubsystem);
        if(parent == nullptr){//If the parent is null, add this to the top level node list
            design.addTopLevelNode(newSubsystem);
        }
        //Add to map
        nodeMap[fullNodeID]=newSubsystem;

        //Traverse the children in the subgraph
        if(subgraph != nullptr)
        {
            nodesImported += GraphMLImporter::importNodes(subgraph, design, nodeMap, edgeNodes, newSubsystem, dialect);
        }

    } else if(blockType == "Enabled Subsystem"){
        std::shared_ptr<EnabledSubSystem> newEnabledSubsystem = NodeFactory::createNode<EnabledSubSystem>(parent);
        newEnabledSubsystem->setId(Node::getIDFromGraphMLFullPath(fullNodeID));
        if(hasName){
            newEnabledSubsystem->setName(name);
        }

        //Add node to design
        design.addNode(newEnabledSubsystem);
        if(parent == nullptr){//If the parent is null, add this to the top level node list
            design.addTopLevelNode(newEnabledSubsystem);
        }
        //Add to map
        nodeMap[fullNodeID]=newEnabledSubsystem;

        //Traverse the children in the subgraph
        if(subgraph != nullptr)
        {
            nodesImported += GraphMLImporter::importNodes(subgraph, design, nodeMap, edgeNodes, newEnabledSubsystem, dialect);
        }

    } else if(blockType == "Special Input Port"){
        std::shared_ptr<EnableInput> newNode = NodeFactory::createNode<EnableInput>(parent);
        newNode->setId(Node::getIDFromGraphMLFullPath(fullNodeID));
        if(hasName){
            newNode->setName(name);
        }

        //Add to enableInputs of parent (EnableNodes exist directly below their EnableSubsystem parent)
        std::shared_ptr<EnabledSubSystem> parentSubsystem = std::dynamic_pointer_cast<EnabledSubSystem>(parent);
        if(parentSubsystem){
            parentSubsystem->addEnableInput(newNode);
        }else{
            throw std::runtime_error("EnableInput parent is not an Enabled Subsystem");
        }

        //Add node to design
        design.addNode(newNode);
        if(parent == nullptr){//If the parent is null, add this to the top level node list
            design.addTopLevelNode(newNode);
        }
        //Add to map
        nodeMap[fullNodeID]=newNode;

    } else if(blockType == "Special Output Port"){
        std::shared_ptr<EnableOutput> newNode = NodeFactory::createNode<EnableOutput>(parent);
        newNode->setId(Node::getIDFromGraphMLFullPath(fullNodeID));
        if(hasName){
            newNode->setName(name);
        }

        //Add to enableOutputs of parent (EnableNodes exist directly below their EnableSubsystem parent)
        std::shared_ptr<EnabledSubSystem> parentSubsystem = std::dynamic_pointer_cast<EnabledSubSystem>(parent);
        if(parentSubsystem){
            parentSubsystem->addEnableOutput(newNode);
        }else{
            throw std::runtime_error("EnableOutput parent is not an Enabled Subsystem");
        }

        //Add node to design
        design.addNode(newNode);
        if(parent == nullptr){//If the parent is null, add this to the top level node list
            design.addTopLevelNode(newNode);
        }
        //Add to map
        nodeMap[fullNodeID]=newNode;

    } else if(blockType == "Top Level"){
        //Should not occur
        throw std::runtime_error("Encountered Top Level Node");
    } else if(blockType == "Master"){
        //Determine which master it is to add to the node map
        std::string instanceName = dataKeyValueMap.at("instance_name");

        if(instanceName == "Input Master"){
            std::shared_ptr<Node> master = design.getInputMaster();
            nodeMap[fullNodeID]=master;
            master->setId(Node::getIDFromGraphMLFullPath(fullNodeID));
            if(hasName){
                master->setName(name);
            }
            importNodePortNames(master, dataKeyValueMap, dialect);
        } else if(instanceName == "Output Master"){
            std::shared_ptr<Node> master = design.getOutputMaster();
            nodeMap[fullNodeID]=master;
            master->setId(Node::getIDFromGraphMLFullPath(fullNodeID));
            if(hasName){
                master->setName(name);
            }
            importNodePortNames(master, dataKeyValueMap, dialect);
        } else if(instanceName == "Visualization Master"){
            std::shared_ptr<Node> master = design.getVisMaster();
            nodeMap[fullNodeID]=master;
            master->setId(Node::getIDFromGraphMLFullPath(fullNodeID));
            if(hasName){
                master->setName(name);
            }
            importNodePortNames(master, dataKeyValueMap, dialect);
        } else if(instanceName == "Unconnected Master"){
            std::shared_ptr<Node> master = design.getUnconnectedMaster();
            nodeMap[fullNodeID]=master;
            master->setId(Node::getIDFromGraphMLFullPath(fullNodeID));
            if(hasName){
                master->setName(name);
            }
            importNodePortNames(master, dataKeyValueMap, dialect);
        } else if(instanceName == "Terminator Master"){
            std::shared_ptr<Node> master = design.getTerminatorMaster();
            nodeMap[fullNodeID]=master;
            master->setId(Node::getIDFromGraphMLFullPath(fullNodeID));
            if(hasName){
                master->setName(name);
            }
            importNodePortNames(master, dataKeyValueMap, dialect);
        } else {
            throw std::runtime_error("Unknown Master Type: " + instanceName);
        }

    } else if(blockType == "VectorFan"){ //This is the Simulink VectorFan type, will call the VectorFan constructor which will return either a VectorFanIn or VectorFanOut
        std::shared_ptr<Node> newNode = GraphMLImporter::importVectorFanNode(fullNodeID, dataKeyValueMap, parent, dialect);
        //Add new node to design and to name node map
        design.addNode(newNode);
        if(parent == nullptr){//If the parent is null, add this to the top level node list
            design.addTopLevelNode(newNode);
        }
        nodeMap[fullNodeID] = newNode;
    } else if(blockType == "Expanded"){
        std::shared_ptr<Node> origNode = GraphMLImporter::importStandardNode(fullNodeID, dataKeyValueMap, parent, dialect);
        //Do not add the orig node to the node list

        std::shared_ptr<ExpandedNode> expandedNode = NodeFactory::createNode<ExpandedNode>(parent, origNode);
        expandedNode->setId(Node::getIDFromGraphMLFullPath(fullNodeID));
        if(hasName){
            expandedNode->setName(name);
        }

        //Add node to design
        design.addNode(expandedNode);
        if(parent == nullptr){//If the parent is null, add this to the top level node list
            design.addTopLevelNode(expandedNode);
        }
        //Add to map
        nodeMap[fullNodeID]=expandedNode;

        //Iterate through the children
        //Traverse the children in the subgraph
        if(subgraph != nullptr)
        {
            nodesImported += GraphMLImporter::importNodes(subgraph, design, nodeMap, edgeNodes, expandedNode, dialect);
        }
    } else if(blockType == "Standard"){
        std::shared_ptr<Node> newNode = GraphMLImporter::importStandardNode(fullNodeID, dataKeyValueMap, parent, dialect);
        //Add new node to design and to name node map
        design.addNode(newNode);
        if(parent == nullptr){//If the parent is null, add this to the top level node list
            design.addTopLevelNode(newNode);
        }
        nodeMap[fullNodeID] = newNode;
    } else{
        throw std::runtime_error("Unknown Block Type");
    }

    return nodesImported;

}

void GraphMLImporter::printGraphmlDOM(std::string filename)
{
    //This function does not perform schema validation or exploit some of the various advanced options Xerces provides.

    //Note on Xerces.  Need to make sure all Xerces objects are released before calling the terminate function.

    //From Example:
    try
    {
        XMLPlatformUtils::Initialize();
    }
    catch (const XMLException& toCatch)
    {
        char* error_msg = XMLString::transcode(toCatch.getMessage());
        std::cerr << "Error during initialization! :\n" << error_msg << std::endl;
        XMLString::release(&error_msg);
        throw std::runtime_error("XML Parser Env could not initialize.  Could not import GraphML File.");
    }

    //From https://xerces.apache.org/xerces-c/program-dom-3.html and DOMCount.cpp
    XMLCh LS_literal[] = {chLatin_L, chLatin_S, chNull};
    DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(LS_literal);
    DOMLSParser* parser = ((DOMImplementationLS*)impl)->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, nullptr);

    // optionally you can set some features on this builder
    if (parser->getDomConfig()->canSetParameter(XMLUni::fgDOMValidate, false))
        parser->getDomConfig()->setParameter(XMLUni::fgDOMValidate, false);
    if (parser->getDomConfig()->canSetParameter(XMLUni::fgDOMNamespaces, true))
        parser->getDomConfig()->setParameter(XMLUni::fgDOMNamespaces, true);

    DOMDocument *doc = nullptr;

    //Try Parsing File
    try {
        doc = parser->parseURI(filename.c_str());
    }
        //Catch the various exceptions
    catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        std::cerr << "Exception message is:" << std::endl << message << std::endl;
        XMLString::release(&message);
        throw std::runtime_error("XML Parsing Failed Due to XML Exception");
    }
    catch (const DOMException& toCatch) {
        char* message = XMLString::transcode(toCatch.msg);
        std::cout << "Exception message is:" << std::endl << message << std::endl;
        XMLString::release(&message);
        throw std::runtime_error("XML Parsing Failed Due to DOM Exception");
    }
    catch (...) {
        std::cerr << "Unexpected Exception" << std::endl;
        throw std::runtime_error("XML Parsing Failed Due to an Unknown Exception");
    }

    //Create Design Object
    std::unique_ptr<Design> design = std::unique_ptr<Design>(new Design());

    //It was suggested by Xerces that scoping be used to ensure proper destruction of Xerces objects before Terminate
    //function is called
    {
        DOMNode *node = doc;

        GraphMLImporter::printXMLNodeAndChildren(node);
    }

    //Cleanup
    parser->release(); //Should release the objects created by the parser

    XMLPlatformUtils::Terminate();
}

void GraphMLImporter::printXMLNodeAndChildren(const DOMNode *node, int tabs)
{
    for(int tab = 0; tab<tabs; tab++)
    {
        std::cout << "\t";
    }
    std::cout << "Node {" << std::endl;

    //==== Print Name and Value ====
    std::string nodeName = GraphMLHelper::getTranscodedString(node->getNodeName());
    for(int tab = 0; tab<tabs+1; tab++)
    {
        std::cout << "\t";
    }
    std::cout << "Node Name: " << nodeName << std::endl;

    //==== Node Type ====
    DOMNode::NodeType nodeType = node->getNodeType();
    for(int tab = 0; tab<tabs+1; tab++)
    {
        std::cout << "\t";
    }
    std::cout << "Node Type: ";
    if(nodeType == DOMNode::NodeType::ELEMENT_NODE)
    {
        std::cout << "Element Node" << std::endl;
    }
    else if(nodeType == DOMNode::NodeType::ATTRIBUTE_NODE)
    {
        std::cout << "Attribute Node" << std::endl;
    }
    else if(nodeType == DOMNode::NodeType::TEXT_NODE)
    {
        std::cout << "Text Node" << std::endl;
    }
    else if(nodeType == DOMNode::NodeType::CDATA_SECTION_NODE)
    {
        std::cout << "CDate Section Node" << std::endl;
    }
    else if(nodeType == DOMNode::NodeType::ENTITY_REFERENCE_NODE)
    {
        std::cout << "Entity Reference Node" << std::endl;
    }
    else if(nodeType == DOMNode::NodeType::ENTITY_NODE)
    {
        std::cout << "Entity Node" << std::endl;
    }
    else if(nodeType == DOMNode::NodeType::PROCESSING_INSTRUCTION_NODE)
    {
        std::cout << "Processing Instruction Node" << std::endl;
    }
    else if(nodeType == DOMNode::NodeType::COMMENT_NODE)
    {
        std::cout << "Comment Node" << std::endl;
    }
    else if(nodeType == DOMNode::NodeType::DOCUMENT_NODE)
    {
        std::cout << "Document Node" << std::endl;
    }
    else if(nodeType == DOMNode::NodeType::DOCUMENT_TYPE_NODE)
    {
        std::cout << "Document Type Node" << std::endl;
    }
    else if(nodeType == DOMNode::NodeType::DOCUMENT_FRAGMENT_NODE)
    {
        std::cout << "Document Fragment Node" << std::endl;
    }
    else if(nodeType == DOMNode::NodeType::NOTATION_NODE)
    {
        std::cout << "Notation Node" << std::endl;
    }
    else
    {
        std::cout << "ERROR" << std::endl;
    }

    //==== Node Value ====
    const XMLCh *nodeVal = node->getNodeValue();
    if(nodeVal != nullptr)
    {
        std::string nodeValue = GraphMLHelper::getTranscodedString(nodeVal);
        for(int tab = 0; tab<tabs+1; tab++)
        {
            std::cout << "\t";
        }
        std::cout << "Node Value: " << nodeValue << std::endl;
    }

    //==== Print Attrs ====
    for(int tab = 0; tab<tabs+1; tab++)
    {
        std::cout << "\t";
    }
    std::cout << "Attributes=[" << std::endl;

    if(node->hasAttributes())
    {
        DOMNamedNodeMap *nodeAttributes = node->getAttributes();
        XMLSize_t numAttributes = nodeAttributes->getLength();

        for (XMLSize_t i = 0; i < numAttributes; i++) {
            DOMNode *attribute = nodeAttributes->item(i);
            std::string attrName = GraphMLHelper::getTranscodedString(attribute->getNodeName());

            const XMLCh *attrVal = attribute->getNodeValue();

            std::string attrValueStr;

            if(attrVal != nullptr)
            {
                attrValueStr = GraphMLHelper::getTranscodedString(attrVal);
            }
            else
            {
                attrValueStr = "";
            }

            for(int tab = 0; tab<tabs+2; tab++)
            {
                std::cout << "\t";
            }
            std::cout << attrName << "=" << attrValueStr << std::endl;
        }
    }

    for(int tab = 0; tab<tabs+1; tab++)
    {
        std::cout << "\t";
    }
    std::cout << "]" << std::endl;


    //==== Print Children ====
    for(int tab = 0; tab<tabs+1; tab++)
    {
        std::cout << "\t";
    }
    std::cout << "Children=[" << std::endl;

    if(node->hasChildNodes())
    {
        for(DOMNode* child = node->getFirstChild(); child != nullptr; child = child->getNextSibling())
        {
            GraphMLImporter::printXMLNodeAndChildren(child, tabs+2);
        }
    }

    for(int tab = 0; tab<tabs+1; tab++)
    {
        std::cout << "\t";
    }
    std::cout << "]" << std::endl;

    for(int tab = 0; tab<tabs; tab++)
    {
        std::cout << "\t";
    }
    std::cout << "}" << std::endl;
}

std::shared_ptr<Node> GraphMLImporter::importStandardNode(std::string idStr, std::map<std::string, std::string> dataKeyValueMap,
                                                                  std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {

    int id = Node::getIDFromGraphMLFullPath(idStr);

    std::string name = "";

    if(dataKeyValueMap.find("instance_name") != dataKeyValueMap.end()){
        name = dataKeyValueMap["instance_name"];
    }

    std::string blockFunction = dataKeyValueMap.at("block_function");

    std::shared_ptr<Node> newNode;

    if(blockFunction == "Sum"){
        newNode = Sum::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
    }else if(blockFunction == "Product"){
        newNode = Product::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
    }else if(blockFunction == "Delay") {
        newNode = Delay::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
    }else if(blockFunction == "Constant"){
        newNode = Constant::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
    }else if(blockFunction == "Gain"){
        newNode = Gain::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
    }else if(blockFunction == "Mux"){
        newNode = Mux::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
    }else if(blockFunction == "DataTypeConversion"){
        newNode = DataTypeConversion::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
    }else if(blockFunction == "Compare" || blockFunction == "RelationalOperator"){ //Vitis name is Compare, Simulink name is RelationalOperator
        newNode = Compare::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
    }else if(blockFunction == "LUT" || blockFunction == "Lookup_n-D"){ //Vitis name is LUT, Simulink name is Lookup_n-D
        newNode = LUT::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
    }else if(blockFunction == "CompareToConstant"){
        newNode = CompareToConstant::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
    }else if(blockFunction == "ThresholdSwitch" || blockFunction == "Switch"){ //Vitis name is ThresholdSwitch, Simulink name is Switch
        newNode = ThresholdSwitch::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
    }else if(blockFunction == "SimulinkMultiPortSwitch" || blockFunction == "MultiPortSwitch"){ //Vitis name is SimulinkMultiPortSwitch, Simulink name is MultiPortSwitch
        newNode = SimulinkMultiPortSwitch::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
    }else if(blockFunction == "DiscreteFIR" || blockFunction == "DiscreteFir"){ //Vitis name is DiscreteFIR, Simulink name is DiscreteFir
        newNode = DiscreteFIR::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
    }else if(blockFunction == "ComplexToRealImag"){
        newNode = ComplexToRealImag::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
    }else if(blockFunction == "RealImagToComplex"){
        newNode = RealImagToComplex::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
    }else if(blockFunction == "DataTypeDuplicate"){
        newNode = DataTypeDuplicate::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
    }else{
        throw std::runtime_error("Unknown block type: " + blockFunction);
    }

    return newNode;
}

std::shared_ptr<Node> GraphMLImporter::importVectorFanNode(std::string idStr, std::map<std::string, std::string> dataKeyValueMap,
                                                          std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    int id = Node::getIDFromGraphMLFullPath(idStr);

    std::string name = "";

    if(dataKeyValueMap.find("instance_name") != dataKeyValueMap.end()){
        name = dataKeyValueMap["instance_name"];
    }

    std::shared_ptr<Node> newNode;

    if(dialect == GraphMLDialect::VITIS){
        //Check the blockFunction for the type of VectorFan
        std::string blockFunction = dataKeyValueMap.at("block_function");

        if(blockFunction == "VectorFanIn"){
            newNode = VectorFanIn::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
        }else if(blockFunction == "VectorFanOut"){
            newNode = VectorFanOut::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
        }else{
            throw std::runtime_error("Unknown VectorFan type: " + blockFunction);
        }
    }else if(dialect == GraphMLDialect::SIMULINK_EXPORT){
        newNode = VectorFan::createFromGraphML(id, name, dataKeyValueMap, parent, dialect);
    }else{
        throw std::runtime_error("Unknown XML Dialect when Parsing VectorFan");
    }

    return newNode;
}

int GraphMLImporter::importEdges(std::vector<xercesc::DOMNode *> &edgeNodes, Design &design,
                                         std::map<std::string, std::shared_ptr<Node>> &nodeMap, GraphMLDialect dialect) {
    //Iterate through the list of edges
    unsigned long numEdges = edgeNodes.size();

    //Iterate through the list of edges
    for(unsigned long i = 0; i<numEdges; i++){
        std::map<std::string, std::string> dataKeyValueMap;
        std::map<std::string, std::string> attributeValueMap;

        GraphMLImporter::graphMLDataAttributeMap(edgeNodes[i], attributeValueMap, dataKeyValueMap);

        //==== Extract Information from "Edge" Entry ====
        int id = Arc::getIDFromGraphMLFullPath(attributeValueMap.at("id"));

        std::string srcFullPath = attributeValueMap.at("source");
        std::string dstFullPath = attributeValueMap.at("target");

        int srcPortNum = std::stoi(dataKeyValueMap.at("arc_src_port"));
        int dstPortNum = 1; //So that decrement puts it to 0 if dst is enable port (for error check below)
        //Handle case when dst port number may not be given if enabled.
        std::string dstPortType = dataKeyValueMap.at("arc_dst_port_type");
        bool standardDst = dstPortType == "Standard";
        if(standardDst){
            dstPortNum = std::stoi(dataKeyValueMap.at("arc_dst_port"));
        }

        std::string complexStr = dataKeyValueMap.at("arc_complex");
        bool complex = !(complexStr == "0" || complexStr == "false");

        int width = std::stoi(dataKeyValueMap.at("arc_width"));

        std::string dataTypeStr = dataKeyValueMap.at("arc_datatype");


        //==== Create DataType object ====
        DataType dataType(dataTypeStr, complex, width);

        //==== Lookup Nodes ====
        std::shared_ptr<Node> srcNode = nodeMap.at(srcFullPath);
        std::shared_ptr<Node> dstNode = nodeMap.at(dstFullPath);

        if(srcNode == nullptr){
            throw std::runtime_error("Null Src Node Ptr");
        }

        if(dstNode == nullptr){
            throw std::runtime_error("Null Dst Node Ptr");
        }

        if(dialect == GraphMLDialect::SIMULINK_EXPORT) {
            //==== Adjust Port Numbers to be 0 based rather than 1 based (only when importing from Simulink)====
            srcPortNum--;
            dstPortNum--;
        }

        if(srcPortNum < 0){
            throw std::runtime_error("Src Port Num < 0");
        }

        if(dstPortNum < 0){
            throw std::runtime_error("Dst Port Num < 0");
        }

        //==== Create the Arc ====
        std::shared_ptr<Arc> newArc;

        //For now, set sample time to -1
        //TODO: set sample time

        if(standardDst) {
            newArc = Arc::connectNodes(srcNode, srcPortNum, dstNode, dstPortNum, dataType);
        }else {
            std::shared_ptr<EnableNode> dstNodeEnabled = std::dynamic_pointer_cast<EnableNode>(dstNode);
            newArc = Arc::connectNodes(srcNode, srcPortNum, dstNodeEnabled, dataType);
        }

        newArc->setId(id);

        //==== Add Arc to Design ====
        design.addArc(newArc);
    }

    return (int) numEdges; //We import all edges/arcs in the list
}

void
GraphMLImporter::importNodePortNames(std::shared_ptr<Node> node, std::map<std::string, std::string> dataKeyValueMap,
                                     GraphMLDialect dialect) {
    unsigned long named_input_port_count = 0;
    unsigned long named_output_port_count = 0;

    auto named_input_port_count_str = dataKeyValueMap.find("named_input_ports");
    if(named_input_port_count_str != dataKeyValueMap.end()){
        if(!named_input_port_count_str->second.empty()) {
            named_input_port_count = std::stoul(named_input_port_count_str->second);
        }
    }

    auto named_output_port_count_str = dataKeyValueMap.find("named_output_ports");
    if(named_output_port_count_str != dataKeyValueMap.end()){
        if(!named_output_port_count_str->second.empty()) {
            named_output_port_count = std::stoul(named_output_port_count_str->second);
        }
    }

    for(int i = 0; i < named_input_port_count; i++){
        std::string input_port_name_query_str;
        if(dialect == GraphMLDialect::VITIS) {
            input_port_name_query_str = "input_port_name_" + std::to_string(i);
        } else if(dialect == GraphMLDialect::SIMULINK_EXPORT) {
            input_port_name_query_str = "input_port_name_" + std::to_string(i+1); //Need to correct for simulink numbering starting at 1
        } else {
            throw std::runtime_error("Unsupported Dialect when parsing XML");
        }

        std::string input_port_name = dataKeyValueMap.at(input_port_name_query_str);
        std::shared_ptr<Port> inputPort = node->getInputPortCreateIfNot(i);
        inputPort->setName(input_port_name);
    }

    for(int i = 0; i < named_output_port_count; i++){
        std::string output_port_name_query_str;
        if(dialect == GraphMLDialect::VITIS) {
            output_port_name_query_str = "output_port_name_" + std::to_string(i);
        } else if(dialect == GraphMLDialect::SIMULINK_EXPORT) {
            output_port_name_query_str = "output_port_name_" + std::to_string(i+1); //Need to correct for simulink numbering starting at 1
        } else {
            throw std::runtime_error("Unsupported Dialect when parsing XML");
        }

        std::string output_port_name = dataKeyValueMap.at(output_port_name_query_str);
        std::shared_ptr<Port> outputPort = node->getOutputPortCreateIfNot(i);
        outputPort->setName(output_port_name);
    }
}
