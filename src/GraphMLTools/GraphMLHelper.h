//
// Created by Christopher Yarp on 7/11/18.
//

#ifndef VITIS_GRAPHMLHELPER_H
#define VITIS_GRAPHMLHELPER_H

#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/PlatformUtils.hpp>

#include <string>

/**
 * \addtogroup GraphMLTools GraphML Import/Export Tools
 */
/*@{*/

class GraphMLHelper {
public:

    /**
     * @brief Returns an XML string as a string c++ string
     * @param xmlStr XML string to transcode
     * @return XML string transcoded into a standard c++ string
     */
    static std::string getTranscodedString(const XMLCh *xmlStr);

    /**
     * @brief Returns an XML Unicode string from an std::string
     *
     * @note: Does not free the memory allocated by XMLString::transcode.  Needs to be freed later in program.
     *
     * @param str string to transcode
     * @return XML unicode string
     */
    static XMLCh* getTranscodedString(const std::string str);


    /**
     * @brief Set the XML attribute of the given DOMElement
     * @param node the XML DOMElement to set the attribute of
     * @param name the attribute name
     * @param val the attribute value
     */
    static void setAttribute(xercesc::DOMElement* node, std::string name, std::string val);

    /**
     * @brief Adds a \<data\> entry to the given XML DOMElement node
     * @param doc the XML DOMDocument that the node resides in
     * @param node the XML DOMElement node to add the data entry to
     * @param key the data key
     * @param val the data value
     * @return a pointer to the new data node
     */
    static xercesc::DOMElement* addDataNode(xercesc::DOMDocument *doc, xercesc::DOMElement* node, std::string key, std::string val);

    /**
     * @brief Create a new element in the given XML document
     * @param doc XML document to create new element in
     * @param name name of new XML element
     * @return pointer to the new XML element
     */
    static xercesc::DOMElement* createNode(xercesc::DOMDocument *doc, std::string name);
};

/*@}*/

#endif //VITIS_GRAPHMLHELPER_H
