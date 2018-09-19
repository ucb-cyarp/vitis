//
// Created by Christopher Yarp on 9/13/18.
//

#ifndef VITIS_UNSUPPORTEDSINK_H
#define VITIS_UNSUPPORTEDSINK_H

#include <vector>
#include <memory>
#include <map>
#include <string>

#include "PrimitiveNode.h"
#include "GraphMLTools/GraphMLDialect.h"
#include "GraphCore/NodeFactory.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 */
/*@{*/

/**
 * @brief Represents a Unsupported Sink Block.  This block may represent a sink in Simulink/Matlab that is
 * not critical for the correct function of the design.  Some examples of this include  datatype constraint
 * blocks.  The DataTypeDuplicate block was implemented because it is commonly used and provides a good sanity check.
 * However, more complex sink logic is outside the current scope of the project.  Note that if the sink block is used to
 * impose datatype restrictions in simulink, they types will be resolved durring the simulink export process.
 *
 * Since sinks that are not outputs are not synthesized the fact that this block's emit logic is un-implemted should
 * not cause a problem.  A warning will be generated durring validation.
 *
 * @note For GraphML emit, the type of the node is restored to the origional unsupported type.  The parameters are saved
 * and re-emitted
 */
class UnsupportedSink : public PrimitiveNode{
    friend NodeFactory;
private:
    std::string nodeType; ///<Stores the original type of node which was unsupported
    std::map<std::string, std::string> dataKeyValueMap; ///<Stores the parameters of the unsupported node

    //==== Constructors ====
    /**
     * @brief Constructs an empty UnsupportedSink
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    UnsupportedSink();

    /**
     * @brief Constructs an empty UnsupportedSink with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit UnsupportedSink(std::shared_ptr<SubSystem> parent);

    /**
     * @brief Constructs a new node with a shallow copy of parameters from the original node.  Ports are not copied and neither is the parent reference.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @note If copying a graph, the parent should be one of the copies and not from the original graph.
     *
     * @warning Because pointer (this) is passed to ports, nodes must be allocated on the heap and not moved.  All interaction should be via pointers.
     *
     * @param parent parent node
     * @param orig The origional node from which a shallow copy is being made
     */
    UnsupportedSink(std::shared_ptr<SubSystem> parent, std::shared_ptr<UnsupportedSink> orig);

public:
    //====Getters/Setters====
    std::string getNodeType() const;
    void setNodeType(const std::string &nodeType);
    std::map<std::string, std::string> getDataKeyValueMap() const;
    void setDataKeyValueMap(const std::map<std::string, std::string> &dataKeyValueMap);

    //====Factories====
    /**
     * @brief Creates a UnsupportedSink node from a GraphML Description
     *
     * @note This function does not add the node to the design or to the nodeID/pointer map
     *
     * @param id the ID number of the node
     * @param name the human readable name of a node
     * @param type the type of the node
     * @param dataKeyValueMap A map of property keys and values extracted from the data nodes in the GraphML
     * @param parent The parent of this node in the hierarchy
     * @return a pointer to the new delay node
     */
    static std::shared_ptr<UnsupportedSink> createFromGraphML(int id, std::string name, std::string type,
                                                              std::map<std::string, std::string> dataKeyValueMap,
                                                              std::shared_ptr<SubSystem> parent);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    void validate() override ;

    /**
     * @brief Does nothing since the sink node is unsupported.  Should never be called for bottom up emit but may
     * be called for scheduled emit.
     */
    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag = false) override;

};

/*@}*/


#endif //VITIS_UNSUPPORTEDSINK_H
