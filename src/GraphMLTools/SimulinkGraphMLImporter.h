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
#include "GraphCore/SubSystem.h"

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
     * @brief Prints the various DOM nodes of a grsaphml file;
     * @param filename The filename of the GraphML file to import
     */
    static void printGraphmlDOM(std::string filename);

private:
    /**
     * @brief Imports nodes from an XML DOM tree
     * @param node The root of the XML DOM tree from the perspective of the import
     * @param design The design object which is modified to include the imported nodes
     * @param nodeMap A map of nodes names to node object pointers which is populated during the import
     * @param edgeNodes A vector of edge node DOM nodes which are found while traversing the XML and importing nodes
     * @return The number of nodes imported
     */
    static int importNodes(xercesc::DOMNode *node, Design &design, std::map<std::string, std::shared_ptr<Node>> &nodeMap, std::vector<xercesc::DOMNode*> &edgeNodes);

    /**
     * @brief Imports nodes from an XML DOM tree
     * @param node The root of the XML DOM tree from the perspective of the import
     * @param design The design object which is modified to include the imported nodes
     * @param nodeMap A map of nodes names to node object pointers which is populated during the import
     * @param edgeNodes A vector of edge node DOM nodes which are found while traversing the XML and importing nodes
     * @param parent The parent Node object for the current position in the DOM
     * @return The number of nodes imported
     */
    static int importNodes(xercesc::DOMNode *node, Design &design, std::map<std::string, std::shared_ptr<Node>> &nodeMap, std::vector<xercesc::DOMNode*> &edgeNodes, std::shared_ptr<SubSystem> parent);


    /**
     * @brief Imports the specified DSP node from an XML DOM node
     * @param node The DSP node's associated XML DOM node
     * @param design The design object which is modified to include the imported nodes
     * @param nodeMap A map of nodes names to node object pointers which is populated during the import
     * @param edgeNodes A vector of edge node DOM nodes which are found while traversing the XML and importing nodes
     * @param parent The parent Node object for the current position in the DOM
     * @return The number of nodes imported
     */
    static int importNode(xercesc::DOMNode *node, Design &design, std::map<std::string, std::shared_ptr<Node>> &nodeMap, std::vector<xercesc::DOMNode*> &edgeNodes, std::shared_ptr<SubSystem> parent);

    /**
     * @brief Imports a Standard GraphML block
     *
     * @note This method does not add the new node to either the design or the name/node map.
     * This is because this method is used when importing both standard nodes and expanded nodes.
     * The orig node of the expanded node should not be added to the node list
     *
     * @param id The id of the node
     * @param dataKeyValueMap The map of key/value pairs for node parameters
     * @param parent The parent Node object for the current position in the DOM
     * @return A pointer to the newly created Standard node
     */
    static std::shared_ptr<Node> importStandardNode(std::string id, std::map<std::string, std::string> dataKeyValueMap, std::shared_ptr<SubSystem> parent);

    /**
     * @brief Import an array of DSP Arcs (from DOM nodes) into the design
     *
     * The edgeNodes list and nodeMap map is populated during a call to @ref SimulinkGraphMLImporter::importNodes
     *
     * @param edgeNodes An array of edge XML DOM nodes
     * @param design The Design to add these edges (arcs) to
     * @param nodeMap A map of node names to Node object pointers
     * @return number of edges imported
     */
    static int importEdges(std::vector<xercesc::DOMNode*> &edgeNodes, Design &design, std::map<std::string, std::shared_ptr<Node>> &nodeMap);

    /**
     * @brief Get the text value for a given node (in the text element under this node)
     * @param node The node to get the text value of
     * @return the text value of the node if there is a single text node child of the given node.  "" if the node has no children, more than 1 child, or the child is not a text node
     */
    static std::string getTextValueOfNode(xercesc::DOMNode *node);

    /**
     * @brief For a GraphML XML Node element, get a map of data key/value pairs and a subgraph XML node if one exists.
     *
     * @note Throws an exception if more than one subgraph node exists for the given node
     *
     * @param node The GraphML Node XML Element
     * @param dataMap A key/value map for data nodes under the given GraphML node
     * @return A pointer to a subgraph node if one exists
     */
    static xercesc::DOMNode* graphMLDataMap(xercesc::DOMNode *node, std::map<std::string, std::string> &dataMap);

    /**
     * @brief For a GraphML XML Node element, get a map of data key/value pairs (and attribute pairs of the given node) and a subgraph XML node if one exists.
     *
     * @note Throws an exception if more than one subgraph node exists for the given node
     *
     * @param node The GraphML Node XML Element
     * @param attributeMap A key/value map for attributes of the given GraphML node
     * @param dataMap A key/value map for data nodes under the given GraphML node
     * @return A pointer to a subgraph node if one exists
     */
    static xercesc::DOMNode* graphMLDataAttributeMap(xercesc::DOMNode *node, std::map<std::string, std::string> &attributeMap, std::map<std::string, std::string> &dataMap);

    /**
     * @brief Prints an XML node.  It prints its name, value, attribute names, and attribute values.  It also prints child nodes.
     * @param tabs the number of tabs for this level of the
     * @param node The XML node to print
     */
    static void printXMLNodeAndChildren(const xercesc::DOMNode *node, int tabs=0);
};

/*@}*/

#endif //VITIS_SIMULINKGRAPHMLIMPORTER_H
