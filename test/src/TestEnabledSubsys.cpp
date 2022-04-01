//
// Created by Christopher Yarp on 2019-01-16.
//

#include <memory>
#include <vector>
#include <set>
#include "gtest/gtest.h"
#include "GraphMLTools/GraphMLImporter.h"
#include "GraphMLTools/GraphMLDialect.h"
#include "GraphCore/Design.h"
#include "GraphCore/Port.h"
#include "MasterNodes/MasterInput.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterUnconnected.h"
#include "GraphMLTools/GraphMLExporter.h"
#include "GraphCore/SchedParams.h"
#include "General/TopologicalSortParameters.h"
#include "General/FileIOHelpers.h"
#include "DSPTestHelper.h"

#include "GraphTestHelper.h"
#include "SimpleDesignValidator.h"

//Test to check that EnabledSubsys can be imported (without crashing).  No other check other than crashing is checked
TEST(TestEnableSubsys, SimulinkImport_CrashOnly) {
    //==== Import File ====
    std::string inputFile = "./stimulus/simulink/basic/enabled_float.graphml";
    std::unique_ptr<Design> design = GraphMLImporter::importGraphML(inputFile, GraphMLDialect::SIMULINK_EXPORT);
}

//Test to check that EnabledSubsys can be exported (without crashing).  No other check other than crashing is checked
TEST(TestEnableSubsys, SimulinkImportExport_CrashOnly) {
    //==== Cleanup Any Residual Results ====
    std::string vitisFile = "./enabled_float.graphml";
    remove(vitisFile.c_str());

    //==== Import Simulink File ====
    std::string simulinkFile = "./stimulus/simulink/basic/enabled_float.graphml";
    std::unique_ptr<Design> design;
    {
        SCOPED_TRACE("Importing Simulink File");
        design = GraphMLImporter::importGraphML(simulinkFile, GraphMLDialect::SIMULINK_EXPORT);
    }

    //NOTE: Import Verified Separately in TEST(SimulinkImport, SimpleDesign)

    //Assign node and arc IDs if they were not assigned already
    design->assignNodeIDs();
    design->assignArcIDs();

    //==== Export Vitis File ====
    {
        SCOPED_TRACE("Exporting Vitis File");
        GraphMLExporter::exportGraphML(vitisFile, *design);
    }

    //==== Re-Import Vitis File ====
    std::unique_ptr<Design> reimportedDesign = GraphMLImporter::importGraphML(vitisFile, GraphMLDialect::VITIS);

    //Cleanup and Erase Exported File
    remove(vitisFile.c_str());
}

//Test to check that EnabledSubsys can be emitted to C (without crashing).  No other check other than crashing is checked
TEST(TestEnableSubsys, SimulinkGenCTopologicalContext_CrashOnly) {
    std::string outputDir = "./enabled_floatOut";
    std::string designName = "enabled_float";

    //Cleanup Prev Run
    FileIOHelpers::deleteDirectoryRecursive(outputDir, true);

    //==== Import Simulink File ====
    std::string simulinkFile = "./stimulus/simulink/basic/enabled_float.graphml";
    std::unique_ptr<Design> design;
    {
        SCOPED_TRACE("Importing Simulink File");
        design = GraphMLImporter::importGraphML(simulinkFile, GraphMLDialect::SIMULINK_EXPORT);
    }

    //Expand the design to primitives
    {
        SCOPED_TRACE("Expanding to Primitives");
        design->expandToPrimitive();
    }

    //Assign node and arc IDs (needed for expanded nodes)
    design->assignNodeIDs();
    design->assignArcIDs();

    //Validate after expansion
    design->validateNodes();

    {
        SCOPED_TRACE("Emitting C");
        DSPTestHelper::runMultithreadGenForSinglePartitionDefaultSettings(*design, outputDir, designName);
    }

    //Cleanup Run
    FileIOHelpers::deleteDirectoryRecursive(outputDir, true);
}