//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_ENABLEDSUBSYSTEM_H
#define VITIS_ENABLEDSUBSYSTEM_H

#include <memory>
#include <vector>
#include "SubSystem.h"
#include "Port.h"
#include "EnableInput.h"
#include "EnableOutput.h"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Represents a subsystem whose execution can be enabled or disabled via an enable port
 */
class EnabledSubSystem : public SubSystem {
friend class NodeFactory;

protected:
    Port enablePort; ///< The enable port of the subsystem
    std::vector<std::shared_ptr<EnableInput>> enabledInputs; ///< A vector of pointers to the @ref EnableInput nodes for this @ref EnabledSubSystem
    std::vector<std::shared_ptr<EnableOutput>> enabledOutputs; ///< A vector of pointers to the @ref EnableOutput nodes for this @ref EnabledSubSystem

    void init() override;

    /**
     * @brief Default constructor
     */
    EnabledSubSystem();

    /**
     * @brief Construct a node with a given parent.
     * @param parent parent of the new node
     */
    EnabledSubSystem(std::shared_ptr<SubSystem> parent);

public:

};

/*@}*/

#endif //VITIS_ENABLEDSUBSYSTEM_H
