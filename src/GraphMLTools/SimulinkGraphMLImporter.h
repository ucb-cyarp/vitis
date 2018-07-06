//
// Created by Christopher Yarp on 6/27/18.
//

#ifndef VITIS_SIMULINKGRAPHMLIMPORTER_H
#define VITIS_SIMULINKGRAPHMLIMPORTER_H

#include <memory>
#include <string>
#include <map>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/PlatformUtils.hpp>

#include "GraphCore/Design.h"

/**
 * \addtogroup GraphMLTools GraphML Import/Export Tools
 */
/*@{*/

/**
 * @brief Contains logic for importing a GraphML description of a Simulink Design
 *
 * Does not utilize the Boost Graph Library (BGL) because the library does not directly support the concept of nested
 * graphs which are used extensively in this project.  While BGL does provide a read_graphml function, it ignores the
 * nested graphs (according to https://www.boost.org/doc/libs/1_67_0/libs/graph/doc/read_graphml.html).
 *
 * However, since GraphML files are XML files, XML parsers are leveraged
 */
class SimulinkGraphMLImporter {
public:
    /**
     * @brief Imports a GraphML file which contains an exported Simulink design.
     *
     * @note The Simulink GraphML file contains some Simulink specific information which will not be retained if the
     * graph is re-exported to GraphML.
     *
     * @note The port numbers will be changed from starting at 1 to starting at 0 to better match C++
     *
     * @param filename The filename of the GraphML file to import
     * @return A pointer to a new Design object which contains an internal representation of the design
     */
    static std::unique_ptr<Design> importSimulinkGraphML(std::string filename);

    /**
     * @brief Imports nodes from an XML DOM tree
     * @param node The root of the XML DOM tree from the perspective of the import
     * @param design The design object which is modified to include the imported nodes
     * @param nodeMap A map of nodes names to node object pointers which is populated durring the import
     * @return The number of nodes imported
     */
    int importNodes(xercesc::DOMNode *node, Design &design, std::map<std::string, std::shared_ptr<Node>> &nodeMap);

    /**
     * @brief Get the text value for a given node (in the text element under this node)
     * @param node The node to get the text value of
     * @return the text value of the node if there is a single text node child of the given node.  "" if the node has no children, more than 1 child, or the child is not a text node
     */
    static std::string getTextValueOfNode(xercesc::DOMNode *node);

    /**
     * @brief Returns an XML string as a string c++ string
     * @param xmlStr XML string to transcode
     * @return XML string transcoded into a standard c++ string
     */
    static std::string getTranscodedString(const XMLCh *xmlStr);

    /**
     * @brief Prints the various DOM nodes of a graphml file;
     * @param filename The filename of the GraphML file to import
     */
    static void printGraphmlDOM(std::string filename);


    /**
     * @brief Prints an XML node.  It prints its name, value, attribute names, and attribute values.  It also prints child nodes.
     * @param node The XML node to print
     */
    static void printXMLNodeAndChildren(const xercesc::DOMNode *node, int tabs=0);
};

/*@}*/

#endif //VITIS_SIMULINKGRAPHMLIMPORTER_H
