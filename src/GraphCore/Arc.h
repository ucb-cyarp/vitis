//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_ARC_H
#define VITIS_ARC_H

#include <memory>
//#include "Port.h"
//#include "Node.h"
#include "DataType.h"

//Forward Decls (Breaking Circular Dep)
class Port;
class Node;
//class DataType;

//This Class

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Represents Arcs in the data flow graph of a DSP design
 */
class Arc : public std::enable_shared_from_this<Arc>{
private:
    std::shared_ptr<Port> srcPort; ///< Pointer to the source port this arc is connected to
    std::shared_ptr<Port> dstPort; ///< Pointer to the destination port this arc is connected to
    DataType dataType; ///< The data type of the data passed via this arc
    double sampleTime; ///< The sample time of the data passed via this arc (in s)
    int delay; ///< The delay along this arc (in cycles)
    int slack; ///< The slack along this arc (in cycles)

protected:
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

public:

    //==== Factories ====
    /**
     * @brief Factory function to create a new blank arc.
     *
     * Factory is used instead of a constructor since shared_from_this is used within Arc and this requires a shared_ptr to exist
     *
     * Arc has no source or destination port pointers, the default data type, a delay of 0, and a slack of zero
     *
     * @return a pointer to a new arc
     */
    static std::shared_ptr<Arc> createArc();

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
    static std::shared_ptr<Arc>
    connectNodes(std::shared_ptr<Node> src, int srcPortNum, std::shared_ptr<Node> dst, int dstPortNum,
                 DataType dataType, double sampleTime = -1);

    //==== Getters/Setters (With added functionality) ====

    /**
     * @brief Sets a new src port for this arc, updating the new port and the previous port in the process
     *
     * Removes the arc from the orig port if not NULL.
     * Updates the given arc so that the src port is the specified new port
     * Adds the arc to the new port.
     *
     * @note A std::shared_ptr must exist before this function is called because it relies on shared_from_this
     *
     * @param srcPort the new srcPort
     */
    void setSrcPortUpdateNewUpdatePrev(std::shared_ptr<Port> srcPort);

    /**
     * @brief Sets a new dst port for this arc, updating the new port and the previous port in the process
     *
     * Removes the arc from the orig port if not NULL.
     * Updates the given arc so that the src port is the specified new port
     * Adds the arc to the new port.
     *
     * @note A std::shared_ptr must exist before this function is called because it relies on shared_from_this
     *
     * @param dstPort the new dstPort
     */
    void setDstPortUpdateNewUpdatePrev(std::shared_ptr<Port> dstPort);

    //==== Getters/Setters ====
    /**
     * @brief Gets a pointer to the source port of the arc
     * @return pointer to the src port of the arc
     */
    std::shared_ptr<Port> getSrcPort() const;

    /**
     * @brief Sets the srcPort pointer for the arc.  Does not remove the arc from any previously connected port or add it to the arc list of the new port.
     *
     * @note To automatically remove the arc from the old port and add it to the new one, use @ref Arc::setSrcPortUpdateNewUpdatePrev
     *
     * @param srcPort the new srcPort for the arc.
     */
    void setSrcPort(const std::shared_ptr<Port> &srcPort);

    /**
     * @brief Gets a pointer to the source port of the arc
     * @return pointer to the src port of the arc
     */
    std::shared_ptr<Port> getDstPort() const;

    /**
     * @brief Sets the dstPort pointer for the arc.  Does not remove the arc from any previously connected port or add it to the arc list of the new port.
     *
     * @note To automatically remove the arc from the old port and add it to the new one, use @ref Arc::setDstPortUpdateNewUpdatePrev
     *
     * @param dstPort the new dstPort for the arc.
     */
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

/*@}*/

#endif //VITIS_ARC_H
