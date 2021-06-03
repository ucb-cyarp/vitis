//
// Created by Christopher Yarp on 8/2/20.
//

#ifndef VITIS_SELECT_H
#define VITIS_SELECT_H

#include "PrimitiveNode.h"
#include "GraphCore/NodeFactory.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 * @{
*/

/**
 * @brief Represent the selecting (slicing) of vectors/arrays.
 * The first port contains the vector to select from, the second port dictates what to select
 */
class Select : public PrimitiveNode{
    friend NodeFactory;

public:
    enum class SelectMode{
        OFFSET_LENGTH, ///<The second input port gives the offset which is the beginning of the selected segment and the length is set statically in outputLen.  The selection is a contiguous segment of the origional array
        INDEX_VECTOR ///<The second input port is a vector of indexes to select
    };

    static SelectMode parseSelectModeStr(std::string str);
    static std::string selectModeToStr(SelectMode mode);

private:
    std::vector<SelectMode> modes;///<The mode for determining how the subsequent input port(s) controls the selection
    std::vector<int> outputLens; ///<When mode is OFFSET_LENGTH, this is the length of the selection for the given dimension.  Defined as a property so that it is guaranteed to be statically set (allowing the output dimensions to be statically determined).  The size of this vector should match that of the modes vector.  Any value is allowed for dimensions which use INDEX_VECTOR select mode

    //==== Constructors ====
    /**
     * @brief Constructs an empty Select node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    Select();

    /**
     * @brief Constructs an empty Select node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit Select(std::shared_ptr<SubSystem> parent);

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
    Select(std::shared_ptr<SubSystem> parent, Select* orig);

public:
    //====Getters/Setters====
    std::vector<SelectMode> getModes() const;
    void setModes(const std::vector<SelectMode> &modes);
    std::vector<int> getOutputLens() const;
    void setOutputLens(const std::vector<int> &outputLens);

    //====Factories====
    /**
     * @brief Creates a Concatenate node from a GraphML Description
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
    static std::shared_ptr<Select> createFromGraphML(int id, std::string name,
                                                          std::map<std::string, std::string> dataKeyValueMap,
                                                          std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type) override ;

    std::string typeNameStr() override;

    std::string labelStr() override ;

    void validate() override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    /**
     * @brief Emits a C expression for the Concatenate
     */
    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                    bool imag) override;

};

/*! @} */

#endif //VITIS_SELECT_H
