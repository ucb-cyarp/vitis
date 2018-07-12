//
// Created by Christopher Yarp on 7/11/18.
//

#include "GraphMLExporter.h"

#include <iostream>

#include "GraphMLHelper.h"
#include "XMLTranscoder.h"

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
        throw std::runtime_error("XML Parser Env could not initialize.  Could not import GraphML File.");
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
        throw std::runtime_error("Encountered an error when exporting to XML");
    }

    //Release
    doc->release();
    serializer->release();

    XMLPlatformUtils::Terminate();
}
