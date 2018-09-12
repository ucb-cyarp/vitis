//
// Created by Christopher Yarp on 7/17/18.
//

#ifndef VITIS_COMPARE_H
#define VITIS_COMPARE_H

#include "PrimitiveNode.h"
#include "GraphCore/NodeFactory.h"
#include "GraphMLTools/GraphMLDialect.h"

#include <map>
#include <string>

/**
 * \addtogroup PrimitiveNodes Primitives
 */
/*@{*/

/**
 * @brief Represents a comparison (relational operator) block.
 *
 * Compares the signal at port 0 to the signal at port 1.  Port 0 is the left hand operator, Port 1 is the right:
 *
 * (Port 0) CompareOP (Port 1)
 *
 * ex. (Port 0) < (Port 1)
 */
class Compare : public PrimitiveNode {
    friend NodeFactory;
public:
    /**
     * @brief An enum representing the comparison operator used when comparing the signals
     */
    enum class CompareOp{
        LT, ///<Less Than (<)
        LEQ, ///<Less Than or Equal To (<=)
        GT, ///<Greater Than (>)
        GEQ, ///<Greater Than or Equal To (>=)
        EQ, ///<Equal To (==)
        NEQ ///<Not Equal To (!=)
    };

    /**
     * @brief Parse a string to a CompareOp
     * @param str string to parse
     * @return corresponding CompareOp
     */
    static CompareOp parseCompareOpString(std::string str);

    /**
     * @brief Get a string representation of the CompareOp
     * @param op CompareOp to get a string representation of
     * @return string representation of the CompareOp
     */
    static std::string compareOpToString(CompareOp op);

    /**
     * @brief Get a C style string representation of the CompareOp
     * @param op CompareOp to get a string representation of
     * @return C style string representation of the CompareOp
     */
    static std::string compareOpToCString(CompareOp op);

private:
    CompareOp compareOp;

    //==== Constructors ====
    /**
     * @brief Constructs an empty compare node with operator LT
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    Compare();

    /**
     * @brief Constructs an empty compare node with operator LT with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit Compare(std::shared_ptr<SubSystem> parent);

public:
    CompareOp getCompareOp() const;
    void setCompareOp(CompareOp compareOp);

    //====Factories====
    /**
     * @brief Creates a compare (relational operator) node from a GraphML Description
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
    static std::shared_ptr<Compare> createFromGraphML(int id, std::string name,
                                                  std::map<std::string, std::string> dataKeyValueMap,
                                                  std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    void validate() override ;

    /**
     * @brief Emits a C expression for the comparison
     */
    CExpr emitCExpr(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag = false) override;

};

/*@}*/


#endif //VITIS_COMPARE_H
