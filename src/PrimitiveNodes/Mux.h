//
// Created by Christopher Yarp on 7/16/18.
//

#ifndef VITIS_MUX_H
#define VITIS_MUX_H

#include "PrimitiveNode.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/SelectPort.h"
#include "GraphCore/NumericValue.h"
#include "GraphMLTools/GraphMLDialect.h"

#include <map>

/**
 * \addtogroup PrimitiveNodes Primitives
 */
/*@{*/

/**
 * @brief Represents a mux block.
 *
 * The selector port is a special port in this block
 * Ports 0-(n-1) are the input lines which are selected from.  The use of a seperate selector port allows the user to use
 * the standard @ref getInputPort method without re-mapping indexes of ports.
 *
 * The selector port's value directly maps to the input port index which is passed through.
 *
 * This switch object can act as a multiport switch
 *
 * @note This mux assumes the selector port input is the index of the input to forward.  If a switch with a threshold is desired,
 * use @ref ThresholdSwitch.
 *
 * @note The Simulink Multiport Switch maps to this excpet that the index is off by 1.  To prevent confusion, a seperate @SimulinkMultiPortSwitch
 * class is provided.  The @ref SimulinkMultiportSwitch expands to a mux and a -1 on the select line.
 *
 */
class Mux : public PrimitiveNode{
    friend NodeFactory;

private:
    std::unique_ptr<SelectPort> selectorPort;

    //==== Constructors ====
    /**
     * @brief Constructs a mux without a parent specified.  Creates an empty selector port.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    Mux();

    /**
     * @brief Constructs a mux with a given parent.  Creates an empty selector port.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit Mux(std::shared_ptr<SubSystem> parent);

public:
    //==== Getters/Setters ====
    /**
     * @brief Get an aliased shared pointer to the selector port.
     *
     * The pointer is aliased with this node as the stored pointer.
     *
     * @return aliased shared pointer to the selector port
     */
    std::shared_ptr<SelectPort> getSelectorPort() const;

    /**
    * @brief Add a select arc to the given node, updating referenced objects in the process
    *
    * Adds an select arc to this node.  Removes the arc from the orig port if not NULL.
    * Updates the given arc so that the destination port is set to the specified input
    * port of this node.  If the port object for the given number is not yet created, it
    * will be created during the call.
    * @param arc The arc to add
    */
    void addSelectArcUpdatePrevUpdateArc(std::shared_ptr<Arc> arc);

    //==== Factories ====
    /**
     * @brief Creates a mux node from a GraphML Description
     *
     * @note Supports VITIS dialect only for import
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
    static std::shared_ptr<Mux> createFromGraphML(int id, std::string name,
                                                  std::map<std::string, std::string> dataKeyValueMap,
                                                  std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    /**
     * @brief Performs check of @ref Node in addition to checking the select port
     */
    void validate() override;

    std::string labelStr() override ;

    //TODO: When syntehsizing C/C++ check that boolean is handled (false = 0, true = 1)

};

/*@}*/

#endif //VITIS_MUX_H
