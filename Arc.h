//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_ARC_H
#define VITIS_ARC_H

#include <memory>
#include "Port.h"
#include "Node.h"
#include "DataType.h"

/**
 * @brief Represents Arcs in the data flow graph of a DSP design
 */
class Arc {
private:
    std::shared_ptr<Port> srcPort; ///< Pointer to the source port this arc is connected to
    std::shared_ptr<Port> dstPort; ///< Pointer to the destination port this arc is connected to
    DataType dataType; ///< The datatype of the data passed via this arc
    double sampleTime; ///< The sample time of the data passed via this arc (in s)
    int delay; ///< The delay along this arc (in cycles)
    int slack; ///< The slack allong this arc (in cycles)

public:
    //==== Constructors ====
    /**
     * @brief Construct a blank arc
     *
     * Arc has no source or destination port pointers, the default data type, a delay of 0, and a slack of zero
     */
    Arc();

    /**
     * @brief Construct an Arc with the specified properties
     *
     * @note This constructor does not add the arc to the port objects of the referenced nodes.  To accomplish that,
     * use @ref Arc::connectNodes instead.
     *
     * @param srcPort
     * @param dstPort
     * @param dataType
     * @param sampleTime
     */
    Arc(std::shared_ptr<Port> srcPort, std::shared_ptr<Port> dstPort, DataType dataType, double sampleTime = -1);

    //==== Factories ====
    /**
     * @brief Connects two nodes with a newly created Arc
     *
     * This function adds the new arc to the specified ports of the source node and destination node
     *
     * @param src source node for Arc
     * @param srcPortNum source port number for Arc
     * @param dst destination node for Arc
     * @param dstPortNum destination port number for Arc
     * @param dataType data type for data flowing via Arc
     * @param sampleTime sample time for data flowing via Arc (in s)
     * @return shared pointer to the newly created arc
     */
    static std::shared_ptr<Arc> connectNodes(std::shared_ptr<Node> src, int srcPortNum, std::shared_ptr<Node> dst, int dstPortNum, DataType dataType, double sampleTime = -1);

    //==== Getters/Setters ====
    std::shared_ptr<Port> getSrcPort() const;
    void setSrcPort(const std::shared_ptr<Port> &srcPort);
    std::shared_ptr<Port> getDstPort() const;
    void setDstPort(const std::shared_ptr<Port> &dstPort);
    DataType getDataType() const;
    void setDataType(const DataType &dataType);
    double getSampleTime() const;
    void setSampleTime(double sampleTime);
    int getDelay() const;
    void setDelay(int delay);
    int getSlack() const;
    void setSlack(int slack);
};


#endif //VITIS_ARC_H
