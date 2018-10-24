//
// Created by Christopher Yarp on 10/22/18.
//

#ifndef VITIS_CONTEXTFAMILYCONTAINER_H
#define VITIS_CONTEXTFAMILYCONTAINER_H

#include "SubSystem.h"
#include "ContextContainer.cpp"

/**
 * \addtogroup GraphCore Graph Core
 */
/*@{*/

/**
 * @brief Container for context containers that should be scheduled with some order
 *
 * This is not considered a standard part of the design hierarchy but is used while scheduling
 */
class ContextFamilyContainer {
private:
    std::vector<std::shared_ptr<ContextContainer>> containerOrder; ///<An ordered list of context containers which should be scheduled in order (context containers should also be children)

};

/*@}*/

#endif //VITIS_CONTEXTFAMILYCONTAINER_H
