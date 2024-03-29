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
 * @{
*/

/**
 * @brief GraphML Namespace String
 */
#define GRAPHML_NS "http://graphml.graphdrawing.org/xmlns"

/**
 * @brief Container for helper functions used when dealing with GraphML files (importing/exporting)
 */
namespace GraphMLHelper {
    /**
     * @brief Returns an XML string as a string c++ string
     * @param xmlStr XML string to transcode
     *
     * @note For the inverse transform, use @ref XMLTranscoder
     *
     * @return XML string transcoded into a standard c++ string
     */
    std::string getTranscodedString(const XMLCh *xmlStr);

    /**
     * @brief Set the XML attribute of the given DOMElement
     * @param node the XML DOMElement to set the attribute of
     * @param name the attribute name
     * @param val the attribute value
     */
    void setAttribute(xercesc::DOMElement* node, std::string name, std::string val);

    /**
     * @brief Adds a \<data\> entry to the given XML DOMElement node
     * @param doc the XML DOMDocument that the node resides in
     * @param node the XML DOMElement node to add the data entry to
     * @param key the data key
     * @param val the data value
     * @return a pointer to the new data node
     */
    xercesc::DOMElement* addDataNode(xercesc::DOMDocument *doc, xercesc::DOMElement* node, std::string key, std::string val);

    /**
     * @brief Create a new element in the given XML document.  This element is not added as the child of another element
     * @param doc XML document to create new element in
     * @param name name of new XML element
     * @return pointer to the new XML element
     */
    xercesc::DOMElement* createNode(xercesc::DOMDocument *doc, std::string name);

    /**
     * @brief Create a new element in the given XML document.  This element contains no attributes but contains a single text node.
     * This element is not added as the child of another element
     *
     * An example would be \<default\>some text\</default\>
     *
     * @param doc XML document to create new element in
     * @param name name of new XML element
     * @param txt text content of text node
     * @return pointer to the new XML element
     */
    xercesc::DOMElement* createEncapulatedTextNode(xercesc::DOMDocument *doc, std::string name, std::string txt);
};

/*! @} */

#endif //VITIS_GRAPHMLHELPER_H
