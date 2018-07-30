//
// Created by Christopher Yarp on 7/26/18.
//

#ifndef VITIS_ENABLEDSUBSYSTEMDESIGNVALIDATOR_H
#define VITIS_ENABLEDSUBSYSTEMDESIGNVALIDATOR_H

#include "GraphCore/Design.h"

#include <memory>
#include <vector>
#include <set>

#include "gtest/gtest.h"
#include "GraphMLTools/GraphMLImporter.h"
#include "GraphCore/Design.h"
#include "GraphCore/Port.h"
#include "MasterNodes/MasterInput.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterUnconnected.h"
#include "PrimitiveNodes/LUT.h"

#include "GraphTestHelper.h"

/**
 * @brief Class containing method to validate the nested_enabled_subsystem Simulink Design Stimulus (in test/stimulus/simulink/basic/enabled.graphml)
 */
class EnabledSubsystemDesignValidator {
public:
    /**
     * @brief Validate the imported design
     * @param design to validate
     */
    static void validate(Design &design);
};


#endif //VITIS_ENABLEDSUBSYSTEMDESIGNVALIDATOR_H
