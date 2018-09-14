//
// Created by Christopher Yarp on 8/22/18.
//

#ifndef VITIS_DISCRETEFIR_H
#define VITIS_DISCRETEFIR_H

#include "HighLevelNode.h"
#include "GraphMLTools/GraphMLDialect.h"
#include "GraphCore/NodeFactory.h"

/**
 * \addtogroup HighLevelNodes High Level Nodes
 *
 * @brief Expandable to primitives and may have multiple implementation possibilities
 *
 * A Convenience For Referring to a Common Structure
 */
/*@{*/

class DiscreteFIR : public HighLevelNode {
    friend NodeFactory;

public:
    /**
     * @brief Specifies the source for the coefficients of the FIR filter.
     */
    enum class CoefSource{
        FIXED, ///<The coefficients are fixed and specified in the discription of the FIR filter
        INPUT_PORT ///<The coefficients come from an input port
    };

    static CoefSource parseCoefSourceStr(std::string str);
    static std::string coefSourceToString(CoefSource coefSource);

private:
    CoefSource coefSource; ///<The source of the coefficients for this FIR filter
    std::vector<NumericValue> coefs; ///<The coefficients of the FIR filter (only used if coefficient source is FIXED)
    std::vector<NumericValue> initVals; ///<The initial values of the state elements in the FIR filter

    /**
     * @brief Constructs a DiscreteFIR filter with no coefficients or initVals.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    DiscreteFIR();

    /**
     * @brief  Constructs a DiscreteFIR filter with no coefficients or initVals with a specified parent
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit DiscreteFIR(std::shared_ptr<SubSystem> parent);

public:
    //==== Getters/Setters ====
    CoefSource getCoefSource() const;
    void setCoefSource(CoefSource coefSource);
    std::vector<NumericValue> getCoefs() const;
    void setCoefs(const std::vector<NumericValue> &coefs);
    std::vector<NumericValue> getInitVals() const;
    void setInitVals(const std::vector<NumericValue> &initVals);

    //==== Factories ====
    /**
     * @brief Creates a DiscreteFIR node from a GraphML Description
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
    static std::shared_ptr<DiscreteFIR> createFromGraphML(int id, std::string name,
                                                                std::map<std::string, std::string> dataKeyValueMap,
                                                                std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Expand ====
    /**
     * @brief Expands the CompareToConstant block into primitive blocks
     *
     * Validates before expansion to check assumptions are fulfilled.
     *
     * Expands into direct form FIR
     *
     */
    bool expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes, std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs) override ;

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    void validate() override ;

};

/*@}*/

#endif //VITIS_DISCRETEFIR_H