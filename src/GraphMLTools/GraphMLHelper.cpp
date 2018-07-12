//
// Created by Christopher Yarp on 7/11/18.
//

#include "GraphMLHelper.h"

using namespace xercesc;

std::string GraphMLHelper::getTranscodedString(const XMLCh *xmlStr)
{
    char *transcoded = XMLString::transcode(xmlStr);
    std::string transcodedStr = transcoded;
    XMLString::release(&transcoded); //Need to release transcode object

    return transcodedStr;
}

XMLCh *GraphMLHelper::getTranscodedString(const std::string str) {
    return XMLString::transcode(str.c_str());
}

void GraphMLHelper::setAttribute(xercesc::DOMElement *node, std::string name, std::string val) {
    node->setAttribute(xercesc::XMLString::transcode(name.c_str()), xercesc::XMLString::transcode(val.c_str()));
}

xercesc::DOMElement *GraphMLHelper::addDataNode(xercesc::DOMDocument *doc, xercesc::DOMElement *node, std::string key, std::string val) {
    DOMElement* dataNode = doc->createElement(XMLString::transcode("data"));
    setAttribute(dataNode, "key", key);

    DOMText* dataValue = doc->createTextNode(getTranscodedString(val));
    dataNode->appendChild(dataValue);

    node->appendChild(dataNode);

    return dataNode;
}

xercesc::DOMElement *GraphMLHelper::createNode(xercesc::DOMDocument *doc, std::string name) {
    return doc->createElement(getTranscodedString(name));
}
