//
// Created by Christopher Yarp on 7/25/18.
//

#ifndef VITIS_LUTDESIGNVALIDATOR_H
#define VITIS_LUTDESIGNVALIDATOR_H

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
 * @brief Class containing method to validate the lut_subsystem Simulink Design Stimulus (in test/stimulus/simulink/basic/lut_subsystem.graphml)
 */
class LUTDesignValidator {
public:
    /**
     * @brief Validate the imported design
     * @param design to validate
     */
    static void validate(Design &design);
};


#endif //VITIS_LUTDESIGNVALIDATOR_H
