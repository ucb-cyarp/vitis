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
 */
/*@{*/

class GraphMLExporter {
public:
    /**
     * @brief Exports a design to a GraphML Design
     *
     * @param filename The filename of the GraphML file to import
     * @param design Design to export
     */
    static void exportGraphML(std::string filename, Design &design);

};

/*@}*/

#endif //VITIS_GRAPHMLEXPORTER_H
