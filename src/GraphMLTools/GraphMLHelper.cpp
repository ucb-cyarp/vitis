//
// Created by Christopher Yarp on 7/11/18.
//

#include "GraphMLHelper.h"

#include "XMLTranscoder.h"

using namespace xercesc;

std::string GraphMLHelper::getTranscodedString(const XMLCh *xmlStr)
{
    char *transcoded = XMLString::transcode(xmlStr);
    std::string transcodedStr = transcoded;
    XMLString::release(&transcoded); //Need to release transcode object

    return transcodedStr;
}

void GraphMLHelper::setAttribute(xercesc::DOMElement *node, std::string name, std::string val) {
    node->setAttribute(TranscodeToXMLCh(name.c_str()), TranscodeToXMLCh(val.c_str()));
}

xercesc::DOMElement *GraphMLHelper::addDataNode(xercesc::DOMDocument *doc, xercesc::DOMElement *node, std::string key, std::string val) {
    DOMElement* dataNode = doc->createElementNS(TranscodeToXMLCh(GRAPHML_NS), TranscodeToXMLCh("data"));
    setAttribute(dataNode, "key", key);

    DOMText* dataValue = doc->createTextNode(TranscodeToXMLCh(val));
    dataNode->appendChild(dataValue);

    node->appendChild(dataNode);

    return dataNode;
}

xercesc::DOMElement *GraphMLHelper::createNode(xercesc::DOMDocument *doc, std::string name) {
    return doc->createElementNS(TranscodeToXMLCh(GRAPHML_NS), TranscodeToXMLCh(name));
}

xercesc::DOMElement *
GraphMLHelper::createEncapulatedTextNode(xercesc::DOMDocument *doc, std::string name, std::string txt) {
    DOMElement* node = doc->createElementNS(TranscodeToXMLCh(GRAPHML_NS), TranscodeToXMLCh(name));

    DOMText* textNode = doc->createTextNode(TranscodeToXMLCh(txt));
    node->appendChild(textNode);

    return node;
}
