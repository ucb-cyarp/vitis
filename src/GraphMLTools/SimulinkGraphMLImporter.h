//
// Created by Christopher Yarp on 6/27/18.
//

#ifndef VITIS_SIMULINKGRAPHMLIMPORTER_H
#define VITIS_SIMULINKGRAPHMLIMPORTER_H

#include <memory>
#include <string>

#include <xercesc/dom/DOM.hpp>

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
