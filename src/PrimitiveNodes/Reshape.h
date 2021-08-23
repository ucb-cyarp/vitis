//
// Created by Christopher Yarp on 6/2/21.
//

#ifndef VITIS_RESHAPE_H
#define VITIS_RESHAPE_H

#include "PrimitiveNode.h"
#include "GraphCore/NodeFactory.h"

#include <vector>

/**
 * \addtogroup PrimitiveNodes Primitives
 * @{
*/

/**
 * @brief Represent the reshaping of vectors/arrays.  The number of input elements should match the number of output elements.
 *
 * The order of the elements stored in memory remains the same.
 *
 * A second input port can be used as a reference for the target dimensions
 */
class Reshape : public PrimitiveNode {
    friend NodeFactory;

public:
    enum class ReshapeMode{
        VEC_1D, ///<Transform to 1D Vector
        ROW_VEC, ///< Transform to 2D Row Vector
        COL_VEC, ///< Transform to 1D Col Vector
        MANUAL, ///< Transform to Dimensions Specified
        REF_INPUT ///< Transform to Dimensions of 2nd Reference Input
    };

    static ReshapeMode parseReshapeMode(std::string str);
    static std::string reshapeModeToStr(ReshapeMode mode);

private:
    ReshapeMode mode; ///<The reshape mode
    std::vector<int> targetDimensions; ///<The target dimensions.  Populated on import if Manual mode or populated based on port dimensions for other modes

    //==== Constructors ====
    /**
     * @brief Constructs an empty Reshape node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    Reshape();

    /**
     * @brief Constructs an empty Select node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit Reshape(std::shared_ptr<SubSystem> parent);

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
     * @param orig The original node from which a shallow copy is being made
     */
    Reshape(std::shared_ptr<SubSystem> parent, Reshape* orig);

public:
    //====Getters/Setters====
    ReshapeMode getMode() const;
    void setMode(ReshapeMode mode);
    std::vector<int> getTargetDimensions() const;
    void setTargetDimensions(const std::vector<int> &targetDimensions);

    //====Factories====
    /**
     * @brief Creates a Reshape node from a GraphML Description
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
    static std::shared_ptr<Reshape> createFromGraphML(int id, std::string name,
                                                     std::map<std::string, std::string> dataKeyValueMap,
                                                     std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    void propagateProperties() override;

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type) override ;

    std::string typeNameStr() override;

    std::string labelStr() override ;

    void validate() override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    /**
     * @brief Emits a C expression for the Reshape
     */
    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, SchedParams::SchedType schedType, int outputPortNum,
                    bool imag) override;
};

/*! @} */

#endif //VITIS_RESHAPE_H
