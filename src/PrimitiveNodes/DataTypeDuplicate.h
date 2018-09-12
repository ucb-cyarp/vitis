//
// Created by Christopher Yarp on 9/11/18.
//

#ifndef VITIS_DATATYPEDUPLICATE_H
#define VITIS_DATATYPEDUPLICATE_H

#include "PrimitiveNode.h"
#include "GraphMLTools/GraphMLDialect.h"
#include "GraphCore/NodeFactory.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 */
/*@{*/

/**
 * @brief Represents a DataTypeDuplicateBlock
 *
 * This block is effectivly a sink and is not synthesized.  It enforces a constraint that all input arcs coming into it
 * have the same DataType.
 *
 * There are no outputs and therefore the C emit function should not be called.
 *
 * This node can be removed in graph pruning
 */
class DataTypeDuplicate : public PrimitiveNode{
    friend NodeFactory;
private:
    //==== Constructors ====
    /**
     * @brief Constructs an empty DataType Duplicate Block node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    DataTypeDuplicate();

    /**
     * @brief Constructs an empty DataType Duplicate node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit DataTypeDuplicate(std::shared_ptr<SubSystem> parent);

public:
    //====Factories====
    /**
     * @brief Creates a DataType Duplicate node from a GraphML Description
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
    static std::shared_ptr<DataTypeDuplicate> createFromGraphML(int id, std::string name,
                                                                std::map<std::string, std::string> dataKeyValueMap,
                                                                std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    void validate() override ;

    /**
     * @brief Emits a C expression for the Complex to Real/Imag
     */
    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag = false) override;
};

/*@}*/

#endif //VITIS_DATATYPEDUPLICATE_H
