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

    int blockSize; ///<The size of the block (in samples) processed in each call to the function. NOTE: This is used in emit only and is set during the emit process.  Think of this more like a callback
    std::string indVarName; ///<When the blockSize > 1, this is the variable used for indexing into the block. NOTE: This is used in emit only and is set during the emit process.  Think of this more like a callback

    std::map<std::shared_ptr<Port>, std::shared_ptr<ClockDomain>> ioClockDomains; ///<This links a particular Master Port to a clock domain.  If nullptr, the port is in the base clock domain.  If not in the map, the port is assumed to be in the base clock domain (nullptr)

public:
    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::set<GraphMLParameter> graphMLParameters() override;

    std::string typeNameStr() override;

    std::string labelStr() override ;

    bool canExpand() override ;

    //==== Getters/Setters ====
    int getBlockSize() const;
    void setBlockSize(int blockSize);
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
};

/*! @} */

#endif //VITIS_MASTERNODE_H
