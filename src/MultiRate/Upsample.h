//
// Created by Christopher Yarp on 4/16/20.
//

#ifndef VITIS_UPSAMPLE_H
#define VITIS_UPSAMPLE_H

#include "RateChange.h"
#include "GraphMLTools/GraphMLDialect.h"

/**
 * \addtogroup MultiRate Multi-Rate Support Nodes
 * @{
*/

/**
 * @brief Represents an integer upsample rate change.
 *
 * This block takes in 1 sample and produced N samples. Unlike Repeat, this class Zero fills between lower rate
 * samples.  For instance, an upsample by 3 of the input stream 1, 2, 3, 4 would be 1, 0, 0, 2, 0, 0, 3, 0, 0, 4, 0, 0
 */
class Upsample : public RateChange {
    friend class NodeFactory;

protected:
    int upsampleRatio; ///<This is the upsample amount.  It is expressed in (# Samples Output)/(# Samples Input)
    std::vector<NumericValue> initCond; ///<This is the initial conditions for the upsample

    /**
     * @brief Constructs an Upsample node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    Upsample();

    /**
     * @brief Upsample node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit Upsample(std::shared_ptr<SubSystem> parent);

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
    Upsample(std::shared_ptr<SubSystem> parent, Upsample* orig);

    /**
     * @brief Copy parameters from another Upsample node
     *
     * This is used when specializing Upsample nodes
     *
     * @param orig the RateChange node to copy from
     */
    virtual void populateParametersExceptRateChangeNodes(std::shared_ptr<Upsample> orig);

    /**
     * @brief Populates parameters for this node from GraphML.  Factored out so that subclasses can use the same import method.
     * @param graphNode
     * @param include_block_node_type
     */
    void populateUpsampleParametersFromGraphML(int id, std::string name,
                                               std::map<std::string, std::string> dataKeyValueMap,
                                               GraphMLDialect dialect);

    /**
     * @brief Emits properties of the node.  Factored out so that subclasses can use the same function
     * @param doc
     * @param graphNode
     */
    void emitGraphMLProperties(xercesc::DOMDocument *doc, xercesc::DOMElement* thisNode) override;

public:
    //====Getters/Setters====
    int getUpsampleRatio() const;
    void setUpsampleRatio(int upsampleRatio);
    std::vector<NumericValue> getInitCond() const;
    void setInitCond(const std::vector<NumericValue> &initCond);

    //====Factories====
    /**
     * @brief Creates a Upsample node from a GraphML Description
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
    static std::shared_ptr<Upsample> createFromGraphML(int id, std::string name,
                                                    std::map<std::string, std::string> dataKeyValueMap,
                                                    std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    std::pair<int, int> getRateChangeRatio() override;

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type) override ;

    std::string typeNameStr() override;

    std::string labelStr() override ;

    void validate() override;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

    std::shared_ptr<RateChange> convertToRateChangeInputOutput(bool convertToInput,
                                                               std::vector<std::shared_ptr<Node>> &nodesToAdd,
                                                               std::vector<std::shared_ptr<Node>> &nodesToRemove,
                                                               std::vector<std::shared_ptr<Arc>> &arcsToAdd,
                                                               std::vector<std::shared_ptr<Arc>> &arcToRemove) override;
};

/*! @} */

#endif //VITIS_UPSAMPLE_H
