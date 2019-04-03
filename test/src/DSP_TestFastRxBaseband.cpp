//
// Created by Christopher Yarp on 2019-01-18.
//

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

#include "GraphTestHelper.h"
#include "SimpleDesignValidator.h"

//Test to check that EnabledSubsys can be imported (without crashing).  No other check other than crashing is checked
TEST(DSP_TestFastRxBaseband, SimulinkImport_CrashOnly) {
//==== Import File ====
std::string inputFile = "./stimulus/simulink/dsp/fast_rx_baseband.graphml";
std::unique_ptr<Design> design = GraphMLImporter::importGraphML(inputFile, GraphMLDialect::SIMULINK_EXPORT);
}

//Test to check that EnabledSubsys can be exported (without crashing).  No other check other than crashing is checked
TEST(DSP_TestFastRxBaseband, SimulinkImportExport_CrashOnly) {
//==== Cleanup Any Residual Results ====
std::string vitisFile = "./fast_rx_baseband.graphml";
remove(vitisFile.c_str());

//==== Import Simulink File ====
std::string simulinkFile = "./stimulus/simulink/dsp/fast_rx_baseband.graphml";
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
TEST(DSP_TestFastRxBaseband, SimulinkGenCTopologicalContext_CrashOnly) {
std::string outputDir = ".";
std::string designName = "fast_rx_baseband";

std::string mainHFileLoc = outputDir + "/" + designName + ".h";
std::string mainCFileLoc = outputDir + "/" + designName + ".c";

std::string benchmarkHFileLoc = outputDir + "/" + designName + "_benchmark_kernel.h";
std::string benchmarkCFileLoc = outputDir + "/" + designName + "_benchmark_kernel.cpp";
std::string benchmarkDriverCFileLoc = outputDir + "/" + designName + "_benchmark_driver.cpp";
std::string benchmarkMakefileLoc = outputDir + "/Makefile_" + designName + "_const";
std::string benchmarkMakefileNoPCMLoc = outputDir + "/Makefile_noPCM_" + designName + "_const";

std::string memBenchmarkHFileLoc = outputDir + "/" + designName + "_benchmark_kernel_mem.h";
std::string memBenchmarkCFileLoc = outputDir + "/" + designName + "_benchmark_kernel_mem.cpp";
std::string memBenchmarkDriverCFileLoc = outputDir + "/" + designName + "_benchmark_driver_mem.cpp";
std::string memBenchmarkMakefileLoc = outputDir + "/Makefile_" + designName + "_mem";
std::string memBenchmarkMakefileNoPCMLoc = outputDir + "/Makefile_noPCM_" + designName + "_mem";


//Cleanup Prev Run
remove(mainHFileLoc.c_str());
remove(mainCFileLoc.c_str());

remove(benchmarkHFileLoc.c_str());
remove(benchmarkCFileLoc.c_str());
remove(benchmarkDriverCFileLoc.c_str());
remove(benchmarkMakefileLoc.c_str());
remove(benchmarkMakefileNoPCMLoc.c_str());

remove(memBenchmarkHFileLoc.c_str());
remove(memBenchmarkCFileLoc.c_str());
remove(memBenchmarkDriverCFileLoc.c_str());
remove(memBenchmarkMakefileLoc.c_str());
remove(memBenchmarkMakefileNoPCMLoc.c_str());

//==== Import Simulink File ====
std::string simulinkFile = "./stimulus/simulink/dsp/fast_rx_baseband.graphml";
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

//Print Scheduler
SchedParams::SchedType sched = SchedParams::SchedType::TOPOLOGICAL_CONTEXT;
TopologicalSortParameters topoParams(TopologicalSortParameters::Heuristic::BFS, 0);

std::cout << "SCHED: " << SchedParams::schedTypeToString(sched) << std::endl;
std::cout << "SCHED_HEUR: " << TopologicalSortParameters::heuristicToString(topoParams.getHeuristic()) << std::endl;
std::cout << "SCHED_RAND_SEED: " << topoParams.getRandSeed() << std::endl;

//Emit C
std::cout << "Emitting C File: " << mainHFileLoc << std::endl;
std::cout << "Emitting C File: " << mainCFileLoc << std::endl;

{
SCOPED_TRACE("Emitting C");
design->generateSingleThreadedC(outputDir, designName, sched, topoParams, false, false);
}

std::cout << "Emitting CPP File: " << benchmarkHFileLoc << std::endl;
std::cout << "Emitting CPP File: " << benchmarkCFileLoc << std::endl;
std::cout << "Emitting CPP File: " << benchmarkDriverCFileLoc << std::endl;
std::cout << "Emitting Makefile: " << benchmarkMakefileLoc << std::endl;
std::cout << "Emitting Makefile: " << benchmarkMakefileNoPCMLoc << std::endl;

std::cout << "Emitting CPP File: " << memBenchmarkHFileLoc << std::endl;
std::cout << "Emitting CPP File: " << memBenchmarkCFileLoc << std::endl;
std::cout << "Emitting CPP File: " << memBenchmarkDriverCFileLoc << std::endl;
std::cout << "Emitting Makefile: " << memBenchmarkMakefileLoc << std::endl;
std::cout << "Emitting Makefile: " << memBenchmarkMakefileNoPCMLoc << std::endl;

{
SCOPED_TRACE("Emitting Benchmarks");
design->emitSingleThreadedCBenchmarkingDrivers(outputDir, designName, designName);
}

//Cleanup and Erase Exported Files
remove(mainHFileLoc.c_str());
remove(mainCFileLoc.c_str());

remove(benchmarkHFileLoc.c_str());
remove(benchmarkCFileLoc.c_str());
remove(benchmarkDriverCFileLoc.c_str());
remove(benchmarkMakefileLoc.c_str());
remove(benchmarkMakefileNoPCMLoc.c_str());

remove(memBenchmarkHFileLoc.c_str());
remove(memBenchmarkCFileLoc.c_str());
remove(memBenchmarkDriverCFileLoc.c_str());
remove(memBenchmarkMakefileLoc.c_str());
remove(memBenchmarkMakefileNoPCMLoc.c_str());
}