//
// Created by Christopher Yarp on 8/31/20.
//

#ifndef VITIS_PARTITIONCROSSING_H
#define VITIS_PARTITIONCROSSING_H

#include "GraphCore/Arc.h"

/**
 * \addtogroup Estimators Estimators
 * @{
*/

/**
 * @brief This represents a partition crossing in an estimation graph.
 */
class PartitionCrossing : public Arc {
private:
    int initStateCountBlocks;
    int bytesPerBlock;

    //==== Constructors ====
    /**
     * @brief Construct a blank PartitionCrossing
     *
     * Arc has no source or destination port pointers, the default data type, a delay of 0, and a slack of zero
     */
    PartitionCrossing();

    /**
     * @brief Construct an PartitionCrossing with the specified properties
     *
     * @note This constructor does not add the arc to the port objects of the referenced nodes.  To accomplish that,
     * use @ref Arc::connectNodes instead.
     *
     * @param srcPort
     * @param dstPort
     * @param dataType
     * @param sampleTime
     */
    PartitionCrossing(std::shared_ptr<OutputPort> srcPort, std::shared_ptr<InputPort> dstPort, DataType dataType, double sampleTime = -1);

public:
    int getInitStateCountBlocks() const;
    void setInitStateCountBlocks(int initStateCountBlocks);

    int getBytesPerSample() const;

    void setBytesPerSample(int bytesPerSample);

    int getBytesPerBlock() const;

    void setBytesPerBlock(int bytesPerBlock);

    double getBytesPerBaseRateSample() const;

    void setBytesPerBaseRateSample(double bytesPerBaseRateSample);

    /**
     * @brief Factory function to create a new blank PartitionCrossing.
     *
     * Factory is used instead of a constructor since shared_from_this is used within Arc and this requires a shared_ptr to exist
     *
     * Arc has no source or destination port pointers, the default data type, a delay of 0, and a slack of zero
     *
     * @return a pointer to a new arc
     */
    static std::shared_ptr<PartitionCrossing> createArc();

    /**
     * @brief Connects two nodes with a newly created Arc
     *
     * This function adds the new arc to the specified ports of the source node and destination node
     *
     * @param srcPort source port Arc
     * @param dstPort destination port for Arc
     * @param dataType data type for data flowing via Arc
     * @param sampleTime sample time for data flowing via Arc (in s)
     * @return shared pointer to the newly created arc
     */
    static std::shared_ptr<PartitionCrossing>
    connectNodes(std::shared_ptr<OutputPort> srcPort, std::shared_ptr<InputPort> dstPort,
                 DataType dataType, double sampleTime = -1);

    /**
     * @brief Emits the current arc as GraphML
     *
     * Include Data node entries for parameters
     *
     * @param doc the XML document containing graphNode
     * @param graphNode the node representing the \<graph\> XMLNode that this node object is contained within
     *
     * @return pointer to this arc's associated DOMNode
     */
    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode) override;

    /**
     * @brief Get a human readable description of the arc
     * @return human readable description of arc
     */
    std::string labelStr() override;
};

/*! @} */

#endif //VITIS_PARTITIONCROSSING_H
