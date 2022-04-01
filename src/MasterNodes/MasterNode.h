//
// Created by Christopher Yarp on 7/11/18.
//

#ifndef VITIS_MASTERNODE_H
#define VITIS_MASTERNODE_H

#include "GraphCore/Node.h"
#include "MultiRate/ClockDomain.h"

/**
 * \addtogroup MasterNodes Master Nodes
 *
 * @brief A set of classes which represent the different boundaries of the design
 * @{
*/

/**
 * @brief Represents a master node in the design
 *
 * Master nodes are artificial nodes which represent different boundaries of the design including inputs and outputs
 */
class MasterNode : public Node {
    friend class NodeFactory;

protected:
    MasterNode();
    MasterNode(std::shared_ptr<SubSystem> parent, MasterNode* orig);

    //TODO: Change this to be a map of ports to indExprs
    std::string indVarName; ///<When the blockSize > 1, this is the variable used for indexing into the block. NOTE: This is used in emit only and is set during the emit process.  Think of this more like a callback

    std::map<std::shared_ptr<Port>, std::shared_ptr<ClockDomain>> ioClockDomains; ///<This links a particular Master Port to a clock domain.  If nullptr, the port is in the base clock domain.  If not in the map, the port is assumed to be in the base clock domain (nullptr)
    std::map<std::shared_ptr<Port>, int> blockSizes; ///<The size of the block (in samples) processed in each call to the function
    std::map<std::shared_ptr<Port>, DataType> portOrigDataType; ///<The original port datatype before blocking
    std::set<std::shared_ptr<Port>> clockDomainHandledByBlockingBoundary; //Indicates that the port operating in a clock domain is having its indexing logic handled by BlockingNodes.  This is mainly used for MasterInputs going to clock domains operating in vector mode

    //TODO: When adding indexing expression keep https://github.com/ucb-cyarp/vitis/issues/101 in mind

public:
    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::set<GraphMLParameter> graphMLParameters() override;

    std::string typeNameStr() override;

    std::string labelStr() override ;

    bool canExpand() override ;

    //==== Getters/Setters ====
    const std::map<std::shared_ptr<Port>, int> &getBlockSizes() const;
    void setBlockSizes(const std::map<std::shared_ptr<Port>, int> &blockSizes);

    int getPortBlockSize(std::shared_ptr<InputPort> port);
    int getPortBlockSize(std::shared_ptr<OutputPort> port);
    void setPortBlockSize(std::shared_ptr<InputPort> port, int blockSize);
    void setPortBlockSize(std::shared_ptr<OutputPort> port, int blockSize);

    const std::string &getIndVarName() const;
    void setIndVarName(const std::string &indVarName);
    std::map<std::shared_ptr<Port>, std::shared_ptr<ClockDomain>> getIoClockDomains() const;
    void setIoClockDomains(const std::map<std::shared_ptr<Port>, std::shared_ptr<ClockDomain>> &ioClockDomains);
    void resetIoClockDomains();

    void setPortClkDomain(std::shared_ptr<InputPort> port, std::shared_ptr<ClockDomain> clkDomain);
    void setPortClkDomain(std::shared_ptr<OutputPort> port, std::shared_ptr<ClockDomain> clkDomain);
    //If no entry is in the ClockDomain map, it is assumed that the port is in the base clock domain and nullptr is returned
    std::shared_ptr<ClockDomain> getPortClkDomain(std::shared_ptr<InputPort> port);
    std::shared_ptr<ClockDomain> getPortClkDomain(std::shared_ptr<OutputPort> port);

    void setPortOrigDataType(std::shared_ptr<InputPort> port, DataType dataType);
    void setPortOrigDataType(std::shared_ptr<OutputPort> port, DataType dataType);
    //If no entry is in the ClockDomain map, it is assumed that the port is in the base clock domain and nullptr is returned
    DataType getPortOrigDataType(std::shared_ptr<InputPort> port);
    DataType getPortOrigDataType(std::shared_ptr<OutputPort> port);

    void setPortClockDomainLogicHandledByBlockingBoundary(std::shared_ptr<InputPort> port);
    void setPortClockDomainLogicHandledByBlockingBoundary(std::shared_ptr<OutputPort> port);
    bool isPortClockDomainLogicHandledByBlockingBoundary(std::shared_ptr<InputPort> port);
    bool isPortClockDomainLogicHandledByBlockingBoundary(std::shared_ptr<OutputPort> port);

    /**
     * @brief Sets the port original datatypes based on what they are currently connected to.
     *
     * @warning This should be done before blocking
     */
    void setPortOriginalDataTypesBasedOnCurrentTypes();

    /**
     * @brief Returns the set of clock domains that ports of this MasterNode are connected to.
     *
     * nullptr signifies the base rate
     *
     * @return
     */
    std::set<std::shared_ptr<ClockDomain>> getClockDomains();
};

/*! @} */

#endif //VITIS_MASTERNODE_H
