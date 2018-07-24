//
// Created by Christopher Yarp on 7/23/18.
//

#ifndef VITIS_SIMULINKMULTIPORTSWITCH_H
#define VITIS_SIMULINKMULTIPORTSWITCH_H

#include "MediumLevelNode.h"
#include "GraphCore/NodeFactory.h"
#include "GraphMLTools/GraphMLDialect.h"

/**
 * \addtogroup MediumLevelNodes Medium Level Nodes
 */
/*@{*/

/**
 * @brief Represents a Simulink Multi-port Switch
 *
 * This node is expanded to the standard Mux node.  To correct for Simulink's index starting from 1 if one ordering is specified
 */
class SimulinkMultiPortSwitch : public MediumLevelNode {
    friend NodeFactory;

public:
    /**
     * @brief Specifies how the index is specified
     */
    enum class IndexType{
        ZERO_BASED, ///<Indexes start from 0 (default for Vitis/C++)
        ONE_BASED, ///<Indexes start from 1 (default for Simulink)
        CUSTOM ///<Custom index mapping, possible for multiple indexes to map to one port
    };

    static IndexType parseIndexTypeStr(std::string str);
    static std::string indexTypeToString(IndexType indexType);

private:
    IndexType indexType; ///<The indexing scheme the Multi-port switch uses to select the port forwarded to the output

    //==== Constructors ====
    /**
     * @brief Constructs a SimulinkMultiPortSwitch node with ONE_BASED indexing
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    SimulinkMultiPortSwitch();

    /**
     * @brief Constructs a SimulinkMultiPortSwitch node with ONE_BASED indexing and a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit SimulinkMultiPortSwitch(std::shared_ptr<SubSystem> parent);

public:
    //==== Getters/Setters ====
    IndexType getIndexType() const;
    void setIndexType(IndexType indexType);

    //==== Factories ====
    /**
     * @brief Creates a SimulinkMultiPortSwitch node from a GraphML Description
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
    static std::shared_ptr<SimulinkMultiPortSwitch> createFromGraphML(int id, std::string name,
                                                   std::map<std::string, std::string> dataKeyValueMap,
                                                   std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Expand ====
    /**
     * @brief Expands the SimulinkMultiPortSwitch block into a mux block and possibly a decrement.
     *
     * Validates before expansion to check assumptions are fulfilled.
     *
     * Port 0 of the Simulink Block is the select port, the following ports are the data ports that are selected from.
     * When expanded:
     *   - port 0 is assigned to the select port
     *   - port 1 is assigned to port 0
     *   - port 2 is assigned to port 1
     *   - etc.
     *
     * Expansion inserts a decrement (subtract constant 1) if the IndexType is ONE_BASED
     * CUSTOM IndexType requires the inclusion of a LUT in the expansion and is currently not supported.
     *
     * The type of the constant (-1) used for the decrement is the same as the input.  The output of the subtract block is the same as the select input
     */
    bool expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes, std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs) override ;

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    //Check of ONE_BASED that number of bits >1 if integer
    void validate() override ;

};

/*@}*/

#endif //VITIS_SIMULINKMULTIPORTSWITCH_H
