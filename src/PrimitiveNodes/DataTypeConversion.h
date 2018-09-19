//
// Created by Christopher Yarp on 7/16/18.
//

#ifndef VITIS_DATATYPECONVERSION_H
#define VITIS_DATATYPECONVERSION_H

#include "PrimitiveNode.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/NumericValue.h"
#include "GraphCore/DataType.h"
#include "GraphMLTools/GraphMLDialect.h"

#include <map>
#include <string>
#include <vector>

/**
 * \addtogroup PrimitiveNodes Primitives
 */
/*@{*/

/**
 * @brief Represents a data type conversion occurring between 2 arcs
 */
class DataTypeConversion :public PrimitiveNode{
    friend NodeFactory;

public:
    enum class InheritType{
        SPECIFIED, ///< No inherited type, use the specified Datatype
        INHERIT_FROM_OUTPUT ///< Type inherited from output arc
    };

private:
    DataType tgtDataType; ///< The target datatype for the output, ignored if the @ref inheritType is not @ref InheritType::SPECIFIED
    InheritType inheritType; ///< The type of type inheritance this converter uses

    //==== Constructors ====
    /**
     * @brief Construct a datatype converter in "inherit" mode
     */
    DataTypeConversion();

    /**
     * @brief Construct a datatype converter in "inherit" mode, with a given parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit DataTypeConversion(std::shared_ptr<SubSystem> parent);

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
    DataTypeConversion(std::shared_ptr<SubSystem> parent, DataTypeConversion* orig);

public:
    static InheritType parseInheritType(std::string str);
    static std::string inheritTypeToString(InheritType type);

    //==== Getters/Setters ====
    DataType getTgtDataType() const;
    void setTgtDataType(const DataType dataType);
    InheritType getInheritType() const;
    void setInheritType(InheritType inheritType);

    //==== Factories ====
    /**
     * @brief Creates a sum node from a GraphML Description
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
    static std::shared_ptr<DataTypeConversion> createFromGraphML(int id, std::string name,
                                                       std::map<std::string, std::string> dataKeyValueMap,
                                                       std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    //==== Propagation/Validation ====
    /**
     * @brief Propagates the width and complex values from the arc if @ref inheritType is @ref InheritType::SPECIFIED as these are not included in the output (and should not be changed by a datatype conversion)
     */
    void propagateProperties() override;

    /**
     * @brief Checks that the specified datatype and the output arc datatype match if @ref inheritType is @ref InheritType::SPECIFIED.
     */
    void validate() override;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    /**
     * @brief Emits type conversion logic.
     *
     * If no type conversion is required (input and target types are the same), no type conversion logic is returned.
     */
    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag = false) override;

};

/*@}*/

#endif //VITIS_DATATYPECONVERSION_H
