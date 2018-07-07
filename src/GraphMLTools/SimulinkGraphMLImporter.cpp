//
// Created by Christopher Yarp on 6/27/18.
//

#include "SimulinkGraphMLImporter.h"
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

#include <iostream>
#include <string>

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>

using namespace xercesc;

std::unique_ptr<Design> SimulinkGraphMLImporter::importSimulinkGraphML(std::string filename)
{
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

        SimulinkGraphMLImporter::printXMLNodeAndChildren(node);


    }


    //Cleanup
    //doc->release(); //Release the document
    parser->release(); //Should release the objects created by the parser

    XMLPlatformUtils::Terminate();

    return design;
}

std::string SimulinkGraphMLImporter::getTranscodedString(const XMLCh *xmlStr)
{
    char *transcoded = XMLString::transcode(xmlStr);
    std::string transcodedStr = transcoded;
    XMLString::release(&transcoded); //Need to release transcode object

    return transcodedStr;
}

std::string SimulinkGraphMLImporter::getTextValueOfNode(xercesc::DOMNode *node)
{
    if(node->hasChildNodes()){
        DOMNode* child = node->getFirstChild();

        if(child->getNodeType() == DOMNode::NodeType::TEXT_NODE){
            std::string nodeValStr = SimulinkGraphMLImporter::getTranscodedString(child->getNodeValue());

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

DOMNode* SimulinkGraphMLImporter::graphMLDataMap(xercesc::DOMNode *node, std::map<std::string, std::string> &dataMap){
    DOMNode* subgraph = nullptr;

    if(node->hasChildNodes()){
        for(DOMNode* child = node->getFirstChild(); child != nullptr; child = child->getNextSibling()){
            if(child->getNodeType() == DOMNode::NodeType::ELEMENT_NODE) {
                std::string childName = SimulinkGraphMLImporter::getTranscodedString(child->getNodeName());

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
                            dataKey = SimulinkGraphMLImporter::getTranscodedString(keyNode->getNodeValue());
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

int SimulinkGraphMLImporter::importNodes(DOMNode *node, Design &design, std::map<std::string, std::shared_ptr<Node>> &nodeMap, std::vector<DOMNode*> &edgeNodes){
    return SimulinkGraphMLImporter::importNodes(node, design, nodeMap, edgeNodes, std::shared_ptr<SubSystem>(nullptr));
}

int SimulinkGraphMLImporter::importNodes(DOMNode *node, Design &design, std::map<std::string, std::shared_ptr<Node>> &nodeMap, std::vector<DOMNode*> &edgeNodes, std::shared_ptr<SubSystem> parent)
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
                    importedNodes += SimulinkGraphMLImporter::importNodes(child, design, nodeMap, edgeNodes, parent);
                }
            }
        } else if(nodeType==DOMNode::NodeType::COMMENT_NODE) {
            //Encountered a comment node, don't do anything
        } else if(nodeType==DOMNode::NodeType::TEXT_NODE) {
            //Encountering a Text node that is not directly handled by an element node in this parser should be ignored
            //It is likely whitespace

            //TODO: Should we actually check this?
            std::string text = SimulinkGraphMLImporter::getTranscodedString(node->getNodeValue()); //Text of Test Node is NodeValue

            int textLen = text.size();
            for(int i = 0; i<textLen; i++){
                if(!std::isspace(text[i])){
                    throw std::runtime_error("Encountered a text node that is not whitespace");
                }
            }
        } else if(nodeType==DOMNode::NodeType::ELEMENT_NODE) {
            //==== Parse Element Node Types ====
            std::string nodeName = SimulinkGraphMLImporter::getTranscodedString(node->getNodeName());

            if(nodeName=="graphml") {
                //This is the root of the design.
                //Right now, we do not look at the attributes
                //Itterate over all children
                if(node->hasChildNodes()){
                    for(DOMNode* child = node->getFirstChild(); child != nullptr; child = child->getNextSibling()){
                        importedNodes += SimulinkGraphMLImporter::importNodes(child, design, nodeMap, edgeNodes, parent);
                    }
                }

            } else if(nodeName=="key") {
                //For now, we do not parse these keys

            } else if(nodeName=="graph") {
                //This is a graph node, it contains other nodes below it.  Process them
                if(node->hasChildNodes()){
                    for(DOMNode* child = node->getFirstChild(); child != nullptr; child = child->getNextSibling()){
                        importedNodes += SimulinkGraphMLImporter::importNodes(child, design, nodeMap, edgeNodes, parent);
                    }
                }

            } else if(nodeName=="edge") {
                //Encountered an edge node.  For now, just add it to the edge vector
                edgeNodes.push_back(node);

            } else if(nodeName=="node") {
                importedNodes += SimulinkGraphMLImporter::importNode(node, design, nodeMap, edgeNodes, parent);

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

int SimulinkGraphMLImporter::importNode(DOMNode *node, Design &design, std::map<std::string, std::shared_ptr<Node>> &nodeMap, std::vector<DOMNode*> &edgeNodes, std::shared_ptr<SubSystem> parent){
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
            fullNodeID = SimulinkGraphMLImporter::getTranscodedString(idNode->getNodeValue());
        }


    }else
    {
        throw std::runtime_error("Node with no ID encountered");
    }

    //Construct a map of data values (children) of this node
    std::map<std::string, std::string> dataKeyValueMap;
    DOMNode* subgraph = SimulinkGraphMLImporter::graphMLDataMap(node, dataKeyValueMap);

    //Based of the node type, we construct nodes:
    std::string blockType = dataKeyValueMap.at("block_node_type");

    if(blockType == "Subsystem"){
        std::shared_ptr<SubSystem> newSubsystem = NodeFactory::createNode<SubSystem>(parent);
        newSubsystem->setId(Node::getIDFromGraphMLFullPath(fullNodeID));

        //Add node to design
        design.addNode(newSubsystem);
        //Add to map
        nodeMap[fullNodeID]=newSubsystem;

        //Traverse the children in the subgraph
        if(subgraph != nullptr)
        {
            nodesImported += SimulinkGraphMLImporter::importNodes(subgraph, design, nodeMap, edgeNodes, newSubsystem);
        }

    } else if(blockType == "Enabled Subsystem"){
        std::shared_ptr<EnabledSubSystem> newEnabledSubsystem = NodeFactory::createNode<EnabledSubSystem>(parent);
        newEnabledSubsystem->setId(Node::getIDFromGraphMLFullPath(fullNodeID));

        //Add node to design
        design.addNode(newEnabledSubsystem);
        //Add to map
        nodeMap[fullNodeID]=newEnabledSubsystem;

        //Traverse the children in the subgraph
        if(subgraph != nullptr)
        {
            nodesImported += SimulinkGraphMLImporter::importNodes(subgraph, design, nodeMap, edgeNodes, newEnabledSubsystem);
        }

    } else if(blockType == "Special Input Port"){
        std::shared_ptr<Node> newNode = NodeFactory::createNode<EnableInput>(parent);
        newNode->setId(Node::getIDFromGraphMLFullPath(fullNodeID));

        //Add node to design
        design.addNode(newNode);
        //Add to map
        nodeMap[fullNodeID]=newNode;

    } else if(blockType == "Special Output Port"){
        std::shared_ptr<Node> newNode = NodeFactory::createNode<EnableOutput>(parent);
        newNode->setId(Node::getIDFromGraphMLFullPath(fullNodeID));

        //Add node to design
        design.addNode(newNode);
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
        } else if(instanceName == "Output Master"){
            std::shared_ptr<Node> master = design.getOutputMaster();
            nodeMap[fullNodeID]=master;
            master->setId(Node::getIDFromGraphMLFullPath(fullNodeID));
        } else if(instanceName == "Visualization Master"){
            std::shared_ptr<Node> master = design.getVisMaster();
            nodeMap[fullNodeID]=master;
            master->setId(Node::getIDFromGraphMLFullPath(fullNodeID));
        } else if(instanceName == "Unconnected Master"){
            std::shared_ptr<Node> master = design.getUnconnectedMaster();
            nodeMap[fullNodeID]=master;
            master->setId(Node::getIDFromGraphMLFullPath(fullNodeID));
        } else if(instanceName == "Terminator Master"){
            std::shared_ptr<Node> master = design.getTerminatorMaster();
            nodeMap[fullNodeID]=master;
            master->setId(Node::getIDFromGraphMLFullPath(fullNodeID));
        } else {
            throw std::runtime_error("Unknown Master Type: " + instanceName);
        }

    } else if(blockType == "VectorFan"){
        //TODO: Implement
        throw std::runtime_error("VectorFan not yet implemented");

    } else if(blockType == "Expanded"){
        std::shared_ptr<Node> origNode = SimulinkGraphMLImporter::importStandardNode(fullNodeID, dataKeyValueMap, parent);
        //Do not add the orig node to the node list

        std::shared_ptr<ExpandedNode> expandedNode = NodeFactory::createNode<ExpandedNode>(parent, origNode);
        expandedNode->setId(Node::getIDFromGraphMLFullPath(fullNodeID));//

        //Iterate through the children
        //Traverse the children in the subgraph
        if(subgraph != nullptr)
        {
            nodesImported += SimulinkGraphMLImporter::importNodes(subgraph, design, nodeMap, edgeNodes, expandedNode);
        }
    } else if(blockType == "Standard"){
        std::shared_ptr<Node> newNode = SimulinkGraphMLImporter::importStandardNode(fullNodeID, dataKeyValueMap, parent);
        //Add new node to design and to name node map
        design.addNode(newNode);
        nodeMap[fullNodeID] = newNode;
    } else{
        throw std::runtime_error("Unknown Block Type");
    }

    return nodesImported;

}

void SimulinkGraphMLImporter::printGraphmlDOM(std::string filename)
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

        SimulinkGraphMLImporter::printXMLNodeAndChildren(node);


    }

    //Cleanup
    parser->release(); //Should release the objects created by the parser

    XMLPlatformUtils::Terminate();
}

void SimulinkGraphMLImporter::printXMLNodeAndChildren(const DOMNode *node, int tabs)
{
    for(int tab = 0; tab<tabs; tab++)
    {
        std::cout << "\t";
    }
    std::cout << "Node {" << std::endl;

    //==== Print Name and Value ====
    std::string nodeName = SimulinkGraphMLImporter::getTranscodedString(node->getNodeName());
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
        std::string nodeValue = SimulinkGraphMLImporter::getTranscodedString(nodeVal);
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
            std::string attrName = SimulinkGraphMLImporter::getTranscodedString(attribute->getNodeName());

            const XMLCh *attrVal = attribute->getNodeValue();

            std::string attrValueStr;

            if(attrVal != nullptr)
            {
                attrValueStr = SimulinkGraphMLImporter::getTranscodedString(attrVal);
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
            SimulinkGraphMLImporter::printXMLNodeAndChildren(child, tabs+2);
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

std::shared_ptr<Node> SimulinkGraphMLImporter::importStandardNode(std::string idStr, std::map<std::string, std::string> dataKeyValueMap,
                                                                  std::shared_ptr<SubSystem> parent) {

    int id = Node::getIDFromGraphMLFullPath(idStr);

    std::string blockFunction = dataKeyValueMap.at("block_function");

    std::shared_ptr<Node> newNode;

    if(blockFunction == "Sum"){
        newNode = Sum::createFromSimulinkGraphML(id, dataKeyValueMap, parent);
    }else if(blockFunction == "Product"){
        newNode = Product::createFromSimulinkGraphML(id, dataKeyValueMap, parent);
    }

    return newNode;
}
