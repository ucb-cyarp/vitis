//
// Created by Christopher Yarp on 7/24/18.
//

#ifndef VITIS_VECTORFAN_H
#define VITIS_VECTORFAN_H

#include "GraphCore/Node.h"
#include "GraphCore/SubSystem.h"
#include "GraphMLTools/GraphMLDialect.h"

/**
 * \addtogroup BusNodes Bus Nodes
 *
 * @brief Nodes which act on buses and convert to/from buses and standard arcs
 */
/*@{*/

/**
 * @brief An abstract class for VectorFan objects
 *
 * Provides a factory function which parses Simulink VectorFan objects
 */
class VectorFan : public Node {

protected:
    /**
     * @brief Constructs an empty VectorFan node
     */
    VectorFan();

    /**
     * @brief Constructs an empty VectorFan node with a given parent.  This node is not added to the children list of the parent.
     *
     * @param parent parent node
     */
    explicit VectorFan(std::shared_ptr<SubSystem> parent);

public:

    //====Factories====
    /**
     * @brief @brief Creates a VectorFan subclass node from a GraphML Description
     *
     * This factory exists to parse SimulinkExport VectorFan objects and to produce either a VectorFanIn or VectorFanOut object.
     *
     * @note This function does not add the node to the design or to the nodeID/pointer map
     *
     * @param id the ID number of the node
     * @param name the human readable name of a node
     * @param dataKeyValueMap A map of property keys and values extracted from the data nodes in the GraphML
     * @param parent The parent of this node in the hierarchy
     * @param dialect The dialect of the GraphML file being imported
     * @return a pointer to the new delay node
     */
    static std::shared_ptr<VectorFan> createFromGraphML(int id, std::string name,
                                                      std::map<std::string, std::string> dataKeyValueMap,
                                                      std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //====Methods====
    bool canExpand() override;

};

/*@}*/

#endif //VITIS_VECTORFAN_H
