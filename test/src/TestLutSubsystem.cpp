//
// Created by Christopher Yarp on 7/25/18.
//

#include <memory>
#include <vector>
#include <set>
#include "gtest/gtest.h"
#include "GraphMLTools/GraphMLImporter.h"
#include "GraphMLTools/GraphMLDialect.h"
#include "GraphMLTools/GraphMLExporter.h"
#include "GraphCore/Design.h"
#include "GraphCore/Port.h"
#include "MasterNodes/MasterInput.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterUnconnected.h"
#include "PrimitiveNodes/Sum.h"
#include "PrimitiveNodes/Product.h"
#include "PrimitiveNodes/Delay.h"

#include "GraphTestHelper.h"
#include "LUTDesignValidator.h"

TEST(SimulinkImport, LUTSubSystem) {
    //==== Import File ====
    std::string inputFile = "./stimulus/simulink/basic/lut_subsystem.graphml";
    std::unique_ptr<Design> design = GraphMLImporter::importGraphML(inputFile, GraphMLDialect::SIMULINK_EXPORT);

    {
        SCOPED_TRACE("");
        LUTDesignValidator::validate(*design);
    }
}

TEST(ImportExport, LUTSubSystemSimulinkImportExportReImport) {
    //==== Cleanup Any Residual Results ====
    std::string vitisFile = "./lut_subsystem_vitis_export.graphml";
    remove(vitisFile.c_str());

    //==== Import Simulink File ====
    std::string simulinkFile = "./stimulus/simulink/basic/lut_subsystem.graphml";
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

    //==== Validate Re-Imported File ====
    {
        SCOPED_TRACE("Verify Re-Imported Nested Design (from Vitis File)");
        LUTDesignValidator::validate(*reimportedDesign);
    }

    //Cleanup and Erase Exported File
    remove(vitisFile.c_str());
}