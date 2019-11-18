//
// Created by Christopher Yarp on 7/11/18.
//

#ifndef VITIS_GRAPHMLEXPORTER_H
#define VITIS_GRAPHMLEXPORTER_H

#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/PlatformUtils.hpp>

#include "GraphCore/Design.h"
#include <string>

/**
 * \addtogroup GraphMLTools GraphML Import/Export Tools
 * @{
*/

/**
 * @brief Exporter for exporting a DSP design to a GraphML file
 */
class GraphMLExporter {
public:
    /**
     * @brief Exports a design to a GraphML Design
     *
     * @param filename The filename of the GraphML file to import
     * @param design Design to export
     */
    static void exportGraphML(std::string filename, Design &design);

    /**
     * @brief Exports Port names to a GraphML (vitis) file
     * @param node the node whose ports are being exported
     * @param doc the XML document to export to
     * @param graphNode the XML graph node to export the ports into
     */
    static void exportPortNames(std::shared_ptr<Node> node, xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode);

    /**
     * @brief Adds port name parameters to the set of graphMLPaemeters
     * @param node the node from which to add its port name parameters
     * @param graphMLParameters the set of GraphML Parameters to add to
     */
    static void addPortNameProperties(std::shared_ptr<Node> node, std::set<GraphMLParameter> &graphMLParameters);

};

/*! @} */

#endif //VITIS_GRAPHMLEXPORTER_H
