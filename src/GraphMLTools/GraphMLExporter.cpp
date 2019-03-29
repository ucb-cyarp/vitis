//
// Created by Christopher Yarp on 7/11/18.
//

#include "GraphMLExporter.h"

#include <iostream>

#include "GraphMLHelper.h"
#include "XMLTranscoder.h"

#include "GraphCore/Node.h"
#include "GraphCore/InputPort.h"
#include "GraphCore/OutputPort.h"
#include "General/ErrorHelpers.h"

using namespace xercesc;

void GraphMLExporter::exportGraphML(std::string filename, Design &design) {
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
        throw std::runtime_error(ErrorHelpers::genErrorStr("XML Parser Env could not initialize.  Could not import GraphML File."));
    }

    //From https://xerces.apache.org/xerces-c/program-dom-3.html and DOMCount.cpp
    //Get the DOM Implementation
    XMLCh LS_literal[] = {chLatin_L, chLatin_S, chNull};
    DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(LS_literal);

    //Using comments from CreateDOMDocument.cpp as a guide
    //Create document with GraphML namespace
    DOMDocument* doc = impl->createDocument(TranscodeToXMLCh(GRAPHML_NS), TranscodeToXMLCh("graphml"), nullptr);

    //Get Root Element for Document
    DOMElement *root = doc->getDocumentElement();
    //Add additional namespace entries to root element
    GraphMLHelper::setAttribute(root, "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    GraphMLHelper::setAttribute(root, "xsi:schemaLocation", "http://graphml.graphdrawing.org/xmlns http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd");

    //Emit the design;
    design.emitGraphML(doc, root);

    //---Serialize it---

    //DOMPrint.cpp xerces example informed the serializer setup below
    DOMLSSerializer *serializer = impl->createLSSerializer();
    DOMConfiguration* serializerConfig = serializer->getDomConfig();

    //+++Enable format-pretty-print+++
    if (serializerConfig->canSetParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true)) {
        serializerConfig->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true);
    }
    if (serializerConfig->canSetParameter(XMLUni::fgDOMWRTDiscardDefaultContent, false)) {
        serializerConfig->setParameter(XMLUni::fgDOMWRTDiscardDefaultContent, false);
    }
    if (serializerConfig->canSetParameter(XMLUni::fgDOMNamespaceDeclarations, false)) {
        serializerConfig->setParameter(XMLUni::fgDOMNamespaceDeclarations, false);
    }

    bool writeSuccess = serializer->writeToURI(doc, TranscodeToXMLCh(filename));

    if(!writeSuccess){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Encountered an error when exporting to XML"));
    }

    //Release
    doc->release();
    serializer->release();

    XMLPlatformUtils::Terminate();
}

void GraphMLExporter::exportPortNames(std::shared_ptr<Node> node, xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode) {
    std::vector<std::shared_ptr<InputPort>> inputPorts = node->getInputPorts();
    std::vector<std::shared_ptr<OutputPort>> outputPorts = node->getOutputPorts();

    //Emit port names
    unsigned long numInputPorts = inputPorts.size();
    unsigned long numOutputPorts = outputPorts.size();
    GraphMLHelper::addDataNode(doc, graphNode, "named_input_ports", GeneralHelper::to_string(numInputPorts));
    GraphMLHelper::addDataNode(doc, graphNode, "named_output_ports", GeneralHelper::to_string(numOutputPorts));

    for(unsigned long i = 0; i<numInputPorts; i++){
        GraphMLHelper::addDataNode(doc, graphNode, "input_port_name_" + GeneralHelper::to_string(i), inputPorts[i]->getName());
    }

    for(unsigned long i = 0; i<numOutputPorts; i++){
        GraphMLHelper::addDataNode(doc, graphNode, "output_port_name_" + GeneralHelper::to_string(i), outputPorts[i]->getName());
    }
}

void GraphMLExporter::addPortNameProperties(std::shared_ptr<Node> node, std::set<GraphMLParameter> &graphMLParameters) {
    std::vector<std::shared_ptr<InputPort>> inputPorts = node->getInputPorts();
    std::vector<std::shared_ptr<OutputPort>> outputPorts = node->getOutputPorts();

    graphMLParameters.insert(GraphMLParameter("named_input_ports", "int", true));
    graphMLParameters.insert(GraphMLParameter("named_output_ports", "int", true));

    unsigned long numInputPorts = inputPorts.size();
    unsigned long numOutputPorts = outputPorts.size();

    for(unsigned long i = 0; i<numInputPorts; i++){
        graphMLParameters.insert(GraphMLParameter("input_port_name_" + GeneralHelper::to_string(i) , "string", true));
    }

    for(unsigned long i = 0; i<numOutputPorts; i++){
        graphMLParameters.insert(GraphMLParameter("output_port_name_" + GeneralHelper::to_string(i) , "string", true));
    }
}
