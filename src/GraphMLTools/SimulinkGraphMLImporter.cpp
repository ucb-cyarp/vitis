//
// Created by Christopher Yarp on 6/27/18.
//

#include "SimulinkGraphMLImporter.h"

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

    //It was suggested by Xerces that scoping be used to ensure proper destruction of Xerces objects before Terminate
    //function is called
    {

        //Lets traverse the graph and just grab names

        DOMNode *node = doc;

        //NOTE: It appears that each node has an accompanying TEXT node, even if there was not text in the node.
        //      When parsing, be cognisant that the data between the <></> nodes are in the child TEXT node for that
        //      node.  The data is in the value field of the text node.  However, attributes have name/value pairs
        //      in nodes returned by the attribute map.

        SimulinkGraphMLImporter::printXMLNodeAndChildren(node);
    }


    //Cleanup
    //doc->release(); //Release the document
    parser->release(); //Should release the objects created by the parser

    XMLPlatformUtils::Terminate();


    return nullptr;

}

void SimulinkGraphMLImporter::printXMLNodeAndChildren(const DOMNode *node, int tabs)
{
    //==== Print Name and Value ====
    char *nodeName = XMLString::transcode(node->getNodeName());
    for(int tab = 0; tab<tabs; tab++)
    {
        std::cout << "\t";
    }
    std::cout << "Node Name: " << nodeName << std::endl;
    XMLString::release(&nodeName);

    //==== Node Type ====
    DOMNode::NodeType nodeType = node->getNodeType();
    for(int tab = 0; tab<tabs; tab++)
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
        char *nodeValue = XMLString::transcode(nodeVal);
        for(int tab = 0; tab<tabs; tab++)
        {
            std::cout << "\t";
        }
        std::cout << "Node Value: " << nodeValue << std::endl;
        XMLString::release(&nodeValue);
    }

    //==== Print Attrs ====
    for(int tab = 0; tab<tabs; tab++)
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
            char *attrName = XMLString::transcode(attribute->getNodeName());

            const XMLCh *attrVal = attribute->getNodeValue();

            std::string attrValueStr;

            if(attrVal != nullptr)
            {
                char *attrValue = XMLString::transcode(attrVal);
                attrValueStr = attrValue;
                XMLString::release(&attrValue);
            }
            else
            {
                attrValueStr = "";
            }

            for(int tab = 0; tab<tabs+1; tab++)
            {
                std::cout << "\t";
            }
            std::cout << attrName << "=" << attrValueStr << std::endl;
        }
    }

    for(int tab = 0; tab<tabs; tab++)
    {
        std::cout << "\t";
    }
    std::cout << "]" << std::endl;


    //==== Print Children ====
    for(int tab = 0; tab<tabs; tab++)
    {
        std::cout << "\t";
    }
    std::cout << "Children=[" << std::endl;

    if(node->hasChildNodes())
    {
        for(DOMNode* child = node->getFirstChild(); child != nullptr; child = child->getNextSibling())
        {
            SimulinkGraphMLImporter::printXMLNodeAndChildren(child, tabs+1);
        }
    }

    for(int tab = 0; tab<tabs; tab++)
    {
        std::cout << "\t";
    }
    std::cout << "]" << std::endl;

    //Newline After
    std::cout << std::endl;
}