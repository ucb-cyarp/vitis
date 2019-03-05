//
// Created by Christopher Yarp on 2019-02-28.
//

#ifndef VITIS_DIGITALMODULATOR_H
#define VITIS_DIGITALMODULATOR_H

#include "MediumLevelNode.h"
#include "GraphCore/NodeFactory.h"
#include "MasterNodes/MasterUnconnected.h"

/**
 * \addtogroup MediumLevelNodes Medium Level Nodes
 */
/*@{*/

/**
 * @brief Represents a Digital Modulator
 *
 * Is capable of modulating BPSK, QPSK/4QAM, 16QAM
 *
 * Is capable of fixed distance between points (x & y dimensions) or avg power normalization
 *
 * Power Normalization uses the equations derived in "Digital Communication" 3rd ed. by John R. Barry, Edward A. Lee,
 * and David G. Messerschmitt, 1998 (pg 150).
 *
 * When normalizing QAM to a pwr of 1, the distance between points (in the x and y axis) are computed using
 * \f$\sqrt{\frac{3}{2\left(M-1\right)}}\f$.
 *
 * - BPSK is a special case with a normalization factor of 1
 * - QPSK/4QAM is scaled by \f$\sqrt{\frac{1}{2}}\f$
 * - 16QAM is scaled by \f$\sqrt{\frac{1}{10}}\f$
 */
class DigitalModulator : public MediumLevelNode{
friend NodeFactory;

private:
    int bitsPerSymbol; ///<The bits per symbol: BPSK=1, QPSK=2, 16QAM=4
    double rotation; ///<How the constellation is rotated

    double normalization; ///<For fixed distance between points is normalization between points, specifies the distance between points.  For avg power normalization, sets the pwr to normalize 2 (1 is common)

    bool grayCoded; ///<If true, the constellation is gray coded.  If false, it is binary coded
    bool avgPwrNormalize; ///<If true, the constellation is power normalized.  If false, the constellation uses fixed distances between points

    //Rotation is currently not supported

    //==== Constructors ====
    /**
     * @brief Constructs a Digital Modulator node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    DigitalModulator();

    /**
     * @brief Constructs a Digital Modulator node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit DigitalModulator(std::shared_ptr<SubSystem> parent);

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
    DigitalModulator(std::shared_ptr<SubSystem> parent, DigitalModulator* orig);

public:
    int getBitsPerSymbol() const;
    void setBitsPerSymbol(int bitsPerSymbol);

    double getRotation() const;
    void setRotation(double rotation);

    double getNormalization() const;
    void setNormalization(double normalization);

    bool isGrayCoded() const;
    void setGrayCoded(bool grayCoded);

    bool isAvgPwrNormalize() const;
    void setAvgPwrNormalize(bool avgPwrNormalize);

    //==== Factories ====
    /**
     * @brief Creates a DigitalModulator node from a GraphML Description
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
    static std::shared_ptr<DigitalModulator> createFromGraphML(int id, std::string name,
                                                   std::map<std::string, std::string> dataKeyValueMap,
                                                   std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Expand ====
    /**
     * @brief Expands the Modulator block into a LUT
     *
     * Validates before expansion to check assumptions are fulfilled.
     *
     * The type of the constant is determined in the following manner:
     *   - If output type is a floating point type, the constant takes on the same type as the output
     *   - If the output is an integer type, the constant takes the smallest integer type which accommodates the constant.
     *   - If the output is a fixed point type, the constant takes on a fixed point type with the integer portion is the smallest that supports the given constant
     *     the number of fractional bits are the max(fractional bits in output, fractional bits in 1st input)
     *
     *   Complexity and width are taken from the gain NumericValue
     */
    std::shared_ptr<ExpandedNode> expand(std::vector<std::shared_ptr<Node>> &new_nodes, std::vector<std::shared_ptr<Node>> &deleted_nodes,
                                             std::vector<std::shared_ptr<Arc>> &new_arcs, std::vector<std::shared_ptr<Arc>> &deleted_arcs,
                                             std::shared_ptr<MasterUnconnected> &unconnected_master) override ;

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    void validate() override ;

    std::shared_ptr<Node> shallowClone(std::shared_ptr<SubSystem> parent) override;

};

/*@}*/

#endif //VITIS_DIGITALMODULATOR_H
