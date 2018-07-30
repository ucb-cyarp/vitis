//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_ENABLEDSUBSYSTEM_H
#define VITIS_ENABLEDSUBSYSTEM_H

#include <memory>
#include <vector>
#include "SubSystem.h"
#include "EnablePort.h"
#include "EnableInput.h"
#include "EnableOutput.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Represents a subsystem whose execution can be enabled or disabled via an enable line
 *
 * The enable line is passed to each of the EnableInput/Output node.  The same source should exist for each EnableInput/Output
 */
class EnabledSubSystem : public virtual SubSystem {
friend class NodeFactory;

protected:
    std::vector<std::shared_ptr<EnableInput>> enabledInputs; ///< A vector of pointers to the @ref EnableInput nodes for this @ref EnabledSubSystem
    std::vector<std::shared_ptr<EnableOutput>> enabledOutputs; ///< A vector of pointers to the @ref EnableOutput nodes for this @ref EnabledSubSystem

    /**
     * @brief Default constructor
     */
    EnabledSubSystem();

    /**
     * @brief Construct a node with a given parent.
     * @param parent parent of the new node
     */
    EnabledSubSystem(std::shared_ptr<SubSystem> parent);

    /**
     * @brief Performs check of @ref Node in addition to checking the enable port
     */
    void validate() override;

    std::string labelStr() override ;

public:
    void addEnableInput(std::shared_ptr<EnableInput> &input);
    void addEnableOutput(std::shared_ptr<EnableOutput> &output);

    void removeEnableInput(std::shared_ptr<EnableInput> &input);
    void removeEnableOutput(std::shared_ptr<EnableOutput> &output);

    std::vector<std::shared_ptr<EnableInput>> getEnableInputs() const;
    std::vector<std::shared_ptr<EnableOutput>> getEnableOutputs() const;

    /**
     * @brief Gets the port which is the source of the enable input to the EnableNodes directly contained within this enabled subsystem
     *
     * @note EnableSubsystem::validate should generally be run before calling this function.  It ensures that all EnableNodes have the same enable source.
     *
     * This method first checks the first EnableInput node to get the enable source, if that does not exist it checks the
     * first EnableOutput.
     *
     * @return the source port for the enable lines to the EnableNodes
     */
    std::shared_ptr<Port> getEnableSrc();

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;
};

/*@}*/

#endif //VITIS_ENABLEDSUBSYSTEM_H
