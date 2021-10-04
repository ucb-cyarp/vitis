//
// Created by Christopher Yarp on 9/3/21.
//

#include "MultiThreadGenerator.h"

#include "MasterNodes/MasterInput.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterUnconnected.h"
#include "Passes/MultiThreadPasses.h"
#include "Passes/DesignPasses.h"
#include "Passes/DomainPasses.h"
#include "Passes/ContextPasses.h"
#include "MultiRate/MultiRateHelpers.h"
#include "MultiThread/LocklessInPlaceThreadCrossingFIFO.h"
#include "MultiThread/LocklessThreadCrossingFIFO.h"
#include "Estimators/CommunicationEstimator.h"
#include "GraphMLTools/GraphMLExporter.h"
#include "Scheduling/IntraPartitionScheduling.h"
#include "Estimators/ComputationEstimator.h"
#include "MultiThread/ConstIOThread.h"
#include "MultiThread/StreamIOThread.h"
#include "MultiThread/ThreadCrossingFIFO.h"
#include "PrimitiveNodes/TappedDelay.h"

#include <iostream>

void MultiThreadGenerator::emitMultiThreadedC(Design &design, std::string path, std::string fileName, std::string designName,
                                SchedParams::SchedType schedType, TopologicalSortParameters schedParams,
                                ThreadCrossingFIFOParameters::ThreadCrossingFIFOType fifoType, bool emitGraphMLSched,
                                bool printSched, int fifoLength, unsigned long blockSize, unsigned long subBlockSize,
                                bool propagatePartitionsFromSubsystems, std::vector<int> partitionMap, bool threadDebugPrint,
                                int ioFifoSize, bool printTelem, std::string telemDumpPrefix,
                                EmitterHelpers::TelemetryLevel telemLevel, int telemCheckBlockFreq, double telemReportPeriodSec,
                                unsigned long memAlignment, bool useSCHEDFIFO,
                                PartitionParams::FIFOIndexCachingBehavior fifoIndexCachingBehavior,
                                MultiThreadEmit::ComputeIODoubleBufferType fifoDoubleBuffer,
                                std::string pipeNameSuffix) {

    if(!telemDumpPrefix.empty()){
        telemDumpPrefix = fileName+"_"+telemDumpPrefix;
    }

    //Change the I/O masters to be in partition -2
    //This is a special partition.  Even if they I/O exists on the same node as a process thread, want to handle it seperatly
    design.getInputMaster()->setPartitionNum(IO_PARTITION_NUM);
    design.getOutputMaster()->setPartitionNum(IO_PARTITION_NUM);
    design.getVisMaster()->setPartitionNum(IO_PARTITION_NUM);

    //==== Propagate Properties from Subsystems ====
    if(propagatePartitionsFromSubsystems){
        MultiThreadPasses::propagatePartitionsFromSubsystemsToChildren(design);
    }

    //==== Prune ====
    DesignPasses::prune(design, true);

    //After pruning, disconnect the terminator master and the unconnected master
    //The remaining nodes should be used for other things since they were not pruned
    //However, they may have ports disconnected which were connected to the unconnected or terminator masters
    //This removal is used to avoid uneeccisary communication to the unconnected and terminator masters (FIFOs
    //would potentially be inserted where they are not needed).
    //Note what nodes and ports are disconnected

    DesignPasses::pruneUnconnectedArcs(design, true);

    //=== Handle Clock Domains ===
    //Should do this after all pruning has taken place (and all node disconnects have taken place)
    //Should do after disconnects take place so that no arcs are going to the terminator master as the clock domain
    //discovery and check logic does not distinguish between the actual output master and the terminator master
    //since they are both MasterOutputs
    //TODO: re-evaluate this
    std::vector<std::shared_ptr<ClockDomain>> clockDomains = DomainPasses::findClockDomains(design);
    //TODO Optimize this so a second pass is not needed
    //After pruning, re-discover clock domain parameters since rate change nodes and links to MasterIO ports may have been removed
    //Before doing that, reset the ClockDomain links for master nodes as ports may have become disconnected
    DomainPasses::resetMasterNodeClockDomainLinks(design);
    MultiRateHelpers::rediscoverClockDomainParameters(clockDomains);

    //ClockDomain Rate Change Specialization.  Convert ClockDomains to UpsampleClockDomains or DownsampleClockDomains
    clockDomains = DomainPasses::specializeClockDomains(design, clockDomains);
    design.assignNodeIDs();
    design.assignArcIDs();
    //This also converts RateChangeNodes to Input or Output implementations.
    //This is done before other operations since these operations replace the objects because the class is changed to a subclass

    //AfterSpecialization, create support nodes for clockdomains, particularly DownsampleClockDomains
    DomainPasses::createClockDomainSupportNodes(design, clockDomains, false, false); //Have not done context discovery & marking yet so do not request context inclusion
    design.assignNodeIDs();
    design.assignArcIDs();

    //Check the ClockDomain rates are appropriate
    MultiRateHelpers::validateClockDomainRates(clockDomains);

    //==== Proceed with Context Discovery ====
    //This is an optimization pass to widen the enabled subsystem context
    ContextPasses::expandEnabledSubsystemContexts(design);
    //Quick check to make sure vis node has no outgoing arcs (required for createEnabledOutputsForEnabledSubsysVisualization).
    //Is ok for them to exist (specifically order constraint arcs) after state update node and context encapsulation
    if(design.getVisMaster()->outDegree() > 0){
        throw std::runtime_error(ErrorHelpers::genErrorStr("Visualization Master is Not Expected to Have an Out Degree > 0, has " + GeneralHelper::to_string(design.getVisMaster()->outDegree())));
    }

    // TODO: Re-enable vis node later?
    //    createEnabledOutputsForEnabledSubsysVisualization();
    design.assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
    design.assignArcIDs();

    //Cleanup empty branches of the hierarchy after nodes moved under enabled subsystem contexts
    //cleanupEmptyHierarchy("it was empty or all underlying nodes were moved");

    //==== Partitioning ====
    //TODO: Partition Here
    //      Currently done manually

    //==== Set Partitions of Subsystems That Are Currently Unassigned ====
    //This can happen towards the top level of the design when subsystems are used for organization.  It is possible all nodes are assigned a partition
    //but some of these upper level
    //With blocking now being accomplished by blocking domains, these subsystems need to be assigned a partition
    DesignPasses::assignPartitionsToUnassignedSubsystems(design, true, true);

    //==== Context Encapsulation Preparation (Organizing Contexts and Dealing with Contexts that Cross Partitions) ====
    //Assign EnableNodes to partitions do before encapsulation and Context/StateUpdate node creation, but after EnabledSubsystem expansion so that EnableNodes are moved/created/deleted as appropriate ahead of time
    ContextPasses::placeEnableNodesInPartitions(design);

    //TODO: If FIFO Insertion Moved to Before this Point:
    //      Modify to not stop at FIFOs with no initial state when finding Mux contexts.  FIFOs with initial state are are treated as containing delays.  FIFOs without initial state treated like wires.
    ContextPasses::discoverAndMarkContexts(design);

    //Replicate ContextRoot Drivers (if should be replicated) for each partition
    //Do this after ContextDiscovery and Marking but before encapsulation
    ContextPasses::replicateContextRootDriversIfRequested(design);
    design.assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
    design.assignArcIDs();

    if(emitGraphMLSched) {
        //Export GraphML (for debugging)
        std::string graphMLAfterBlockingFileName = fileName + "_preBlocking.graphml";
        std::cout << "Emitting GraphML Blocking File: " << path << "/" << graphMLAfterBlockingFileName << std::endl;
        GraphMLExporter::exportGraphML(path + "/" + graphMLAfterBlockingFileName, design);
    }

    //TODO: Insert blocking and sub-blocking logic here?  Done before context, encpsulation, and FIFO insertion and may effectivly replace determining if a partition has a single clock domain
    //      Done after context discovery because that is when mux contexts are identified but encapsulation has not yet occured
    //      Because it is done after context discovery, sub-blocking needs to update the context stack for nodes
    //
    //      When handling clock domains under the blocking domain, if they are set to process in a vector fashion, the counter logic should be supporessed
    //      This should remove the need to check if a partition only operates at a certain clock rate.
    //      For FIFOs in the clock domain, they should be passing Vectors of the expected size
    DomainPasses::blockAndSubBlockDesign(design, blockSize, subBlockSize);

    if(emitGraphMLSched) {
        //Export GraphML (for debugging)
        std::string graphMLAfterBlockingFileName = fileName + "_afterBlocking.graphml";
        std::cout << "Emitting GraphML Blocking File: " << path << "/" << graphMLAfterBlockingFileName << std::endl;
        GraphMLExporter::exportGraphML(path + "/" + graphMLAfterBlockingFileName, design);
    }

    //TODO: Need to implement validation post pruning. Also need to fix validation when depends on arcs that could be disconnected (black box)
//    design.validateNodes(); //Does not validate delays which had their specialization deferred

    //Order constraining zero input nodes in enabled subsystems is not nessisary as rewireArcsToContexts can wire the enable
    //line as a depedency for the enable context to be emitted.  This is currently done in the scheduleTopoloicalSort method called below
    //TODO: re-introduce orderConstrainZeroInputNodes if the entire enable context is not scheduled hierarchically
    //TODO: Modify to check for ContextRootDrivers per partition
    //orderConstrainZeroInputNodes(); //Do this after the contexts being marked since this constraint should not have an impact on contexts

    //==== Encapsulate Contexts ====
    //TODO: fix encapsuleate to duplicate driver arc for each ContextFamilyContainer in other partitions
    //to the given partition and add an out
    ContextPasses::encapsulateContexts(design); //TODO: Modify to only insert FIFOs for contextDecisionDrivers when replication was not requested.  If replication was requr
    design.assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
    design.assignArcIDs();

    //Cleanup empty branches of the hierarchy after nodes moved under enabled subsystem contexts
    //cleanupEmptyHierarchy("all underlying nodes were moved durring context encapsulation");

    //==== Create Context Variable Update Nodes ====

    //TODO: investigate moving
    //*** Creating context variable update nodes can be done after partitioning since it should inherit the partition
    //of its context master (in the case of Mux).  Note that includeContext should be set to false
    ContextPasses::createContextVariableUpdateNodes(design, true); //Create after expanding the subcontext so that any movement of EnableInput and EnableOutput nodes (is placed in the same partition as the ContextRoot)
    design.assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
    design.assignArcIDs();

    //==== Insert and Configure FIFOs (but do not ingest Delays) ====
    //** Done before state variable updates created
    //Also should be before state update nodes created since FIFOs can absorb delay nodes (which should not create state update nodes)
    //Insert FIFOs into the crossings.  Note, the FIFO's partition should reside in the upstream partition of the arc(s) it is being placed in the middle of.
    //They should also reside inside of the source context family container
    //
    //This should also be done after encapsulation as the driver arcs of contexts are duplicated for each ContextFamilyContainer

    //Discover arcs crossing partitions and what partition crossings can be grouped together
    std::map<std::pair<int, int>, std::vector<std::vector<std::shared_ptr<Arc>>>> partitionCrossings = design.getGroupableCrossings();

    std::map<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> fifoMap;

    //Insert the partition crossing FIFOs
    {
        std::vector<std::shared_ptr<Node>> new_nodes;
        std::vector<std::shared_ptr<Node>> deleted_nodes;
        std::vector<std::shared_ptr<Arc>> new_arcs;
        std::vector<std::shared_ptr<Arc>> deleted_arcs;

        switch (fifoType) {
            case ThreadCrossingFIFOParameters::ThreadCrossingFIFOType::LOCKLESS_X86:
                //Note that FIFOs are placed in the src partition and context (unless the src is an EnableOutput or RateChangeOutput in which case they are placed one level up).
                fifoMap = MultiThreadPasses::insertPartitionCrossingFIFOs<LocklessThreadCrossingFIFO>(partitionCrossings, new_nodes, deleted_nodes, new_arcs, deleted_arcs);
                break;
            case ThreadCrossingFIFOParameters::ThreadCrossingFIFOType::LOCKLESS_INPLACE_X86:
                fifoMap = MultiThreadPasses::insertPartitionCrossingFIFOs<LocklessInPlaceThreadCrossingFIFO>(partitionCrossings, new_nodes, deleted_nodes, new_arcs, deleted_arcs);
                break;
            default:
                throw std::runtime_error(
                        ErrorHelpers::genErrorStr("Unsupported Thread Crossing FIFO Type for Multithreaded Emit"));
        }
        design.addRemoveNodesAndArcs(new_nodes, deleted_nodes, new_arcs, deleted_arcs);
    }

    design.assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
    design.assignArcIDs();

    std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifoVec;
    std::map<int, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> inputFIFOs;
    std::map<int, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> outputFIFOs;
    for(auto it = fifoMap.begin(); it != fifoMap.end(); it++){
        int srcPartition = it->first.first;
        int dstPartition = it->first.second;
        fifoVec.insert(fifoVec.end(), it->second.begin(), it->second.end());

        std::vector<std::shared_ptr<ThreadCrossingFIFO>>& inputFIFOsRef = inputFIFOs[dstPartition];
        inputFIFOsRef.insert(inputFIFOsRef.end(), it->second.begin(), it->second.end());

        std::vector<std::shared_ptr<ThreadCrossingFIFO>>& outputFIFOsRef = outputFIFOs[srcPartition];
        outputFIFOsRef.insert(outputFIFOsRef.end(), it->second.begin(), it->second.end());
    }

    //Set FIFO length and block size here (do before delay ingest)
    for(int i = 0; i<fifoVec.size(); i++){
        fifoVec[i]->setFifoLength(fifoLength);
    }

    //Since FIFOs are placed in the src partition and context (unless the src is an EnableOutput or RateChange output),
    //FIFOs connected to the input master should have their rate set based on the port rate of the input master.
    //FIFOs connected to the output master should have their rate properly set based on the ClockDomain they are in, especially
    //since any FIFO connected to a RateChange output will be placed in the next context up.
    MultiRateHelpers::setFIFOClockDomainsAndBlockingParams(fifoVec, blockSize);

    //==== Retime ====
    //TODO: Retime Here
    //      Not currently implemented

    //==== FIFO Delay Ingest ====
    //Should be before state update nodes created since FIFOs can absorb delay nodes (which should not create state update nodes)
    //Currently, only nodes that are soley connected to FIFOs are absorbed.  It is possible to absorb other nodes but delay matching
    //and/or multiple FIFOs cascaded (to allow an intermediary tap to be viewed) would potentially be required.
    //TODO: only adjacent delays for now, extend

    {
        std::vector<std::shared_ptr<Node>> new_nodes;
        std::vector<std::shared_ptr<Node>> deleted_nodes;
        std::vector<std::shared_ptr<Arc>> new_arcs;
        std::vector<std::shared_ptr<Arc>> deleted_arcs;

        MultiThreadPasses::absorbAdjacentDelaysIntoFIFOs(fifoMap, new_nodes, deleted_nodes, new_arcs, deleted_arcs, blockSize>1);
        design.addRemoveNodesAndArcs(new_nodes, deleted_nodes, new_arcs, deleted_arcs);
    }

    design.assignNodeIDs();
    design.assignArcIDs();

    //==== Merge FIFOs ====
    //Clock domain was already set for each FIFO above and will be used in the FIFO merge
    //      Add FIFO merge here (so long as on demand / lazy eval FIFOs in conditional execution regions are not considered)
    //      All ports need the same number of initial conditions.  May need to resize initial conditions
    //      Doing this after delay absorption because it currently is only implemented for a single set of input ports
    //      Note that there was a comment I left in the delay absorption code stating that I expected FIFO bundling /
    //      merging to occur after delay absorption
    //      *
    //      Involves moving arcs to ports, moving block size, moving initial conditions, and moving clock domain
    //      Remove other FIFOs.  Place new FIFO outside contexts.  Note: if on-demand / lazy eval fifo implemented later
    //      the merge should not happen between FIFOs in different contexts and new FIFO should be placed inside context.
    //      To make this easier to work with with, insert the FIFO into the context of all the inputs if they are all the same
    //      otherwise, put it outside the contexts


    //TODO: There is currently a problem when ignoring contexts due to scheduling nodes connected to the FIFOs
    {
        std::vector<std::shared_ptr<Node>> new_nodes;
        std::vector<std::shared_ptr<Node>> deleted_nodes;
        std::vector<std::shared_ptr<Arc>> new_arcs;
        std::vector<std::shared_ptr<Arc>> deleted_arcs;
        std::vector<std::shared_ptr<Node>> add_to_top_lvl;

        MultiThreadPasses::mergeFIFOs(fifoMap, new_nodes, deleted_nodes, new_arcs, deleted_arcs, add_to_top_lvl, false, true, blockSize>1);
        design.addRemoveNodesAndArcs(new_nodes, deleted_nodes, new_arcs, deleted_arcs);
        for(auto topLvlNode : add_to_top_lvl){
            design.addTopLevelNode(topLvlNode);
        }
    }
    design.assignNodeIDs();
    design.assignArcIDs();

    //Specialize Deferred delays
    DomainPasses::specializeDeferredDelays(design);
    design.assignNodeIDs();
    design.assignArcIDs();

    //Validate the now specialized delays in design
    {
        std::vector<std::shared_ptr<Node>> nodes = design.getNodes();
        for(const std::shared_ptr<Node> &node : nodes) {
            if (GeneralHelper::isType<Node, Delay>(node) != nullptr &&
                GeneralHelper::isType<Node, TappedDelay>(node) == nullptr) {
                node->validate();
            }
        }
    }

    //Validate the now merged FIFOs
    {
        for(const std::pair<std::pair<int, int>, std::vector<std::shared_ptr<ThreadCrossingFIFO>>> &partCrossingFIFOs : fifoMap) {
            std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifos = partCrossingFIFOs.second;
            for(const std::shared_ptr<ThreadCrossingFIFO> &fifo : fifos){
                fifo->validate();
            }
        }
    }

    //==== Report FIFOs ====
    std::cout << std::endl;
    std::cout << "========== FIFO Report ==========" << std::endl;
    for(auto it = fifoMap.begin(); it != fifoMap.end(); it++){
        std::vector<std::shared_ptr<ThreadCrossingFIFO>> fifoVec = it->second;
        for(int i = 0; i<fifoVec.size(); i++) {
            std::vector<std::shared_ptr<ClockDomain>> clockDomains = fifoVec[i]->getClockDomains();
            std::string clkDomainDescr = "[";
            for(int i = 0; i<clockDomains.size(); i++){
                if(i>0){
                    clkDomainDescr += ", ";
                }
                std::pair<int, int> rate = std::pair<int, int>(1, 1);
                if(clockDomains[i] != nullptr){ //Clock domain can be null which signifies the base case
                    rate = clockDomains[i]->getRateRelativeToBase();
                }
                clkDomainDescr += GeneralHelper::to_string(rate.first) + "/" + GeneralHelper::to_string(rate.second);
            }
            clkDomainDescr += "]";

            std::cout << "FIFO: " << fifoVec[i]->getName() << ", Length (Blocks): " << fifoVec[i]->getFifoLength() << ", Length (Items): " << (fifoVec[i]->getFifoLength()*fifoVec[i]->getTotalBlockSizeAllPorts()) << ", Initial Conditions (Blocks): " << fifoVec[i]->getInitConditionsCreateIfNot(0).size()/(fifoVec[i]->getOutputPort(0)->getDataType().numberOfElements()/fifoVec[i]->getSubBlockSizeCreateIfNot(0))/fifoVec[i]->getBlockSizeCreateIfNot(0) << ", Clock Domain(s): " << clkDomainDescr << std::endl;
        }
    }
    std::cout << std::endl;

    //==== Report Communication (and check for communication deadlocks) ====
    //Report Communication before StateUpdate nodes inserted to avoid false positive errors when detecting arcs to state update nodes
    //in the input partition of a ThreadCrossingFIFO
    std::string graphMLCommunicationInitCondFileName = "";
    std::string graphMLCommunicationInitCondSummaryFileName = "";

    if(emitGraphMLSched) {
        //Export GraphML (for debugging)
        graphMLCommunicationInitCondFileName = fileName + "_communicationInitCondGraph.graphml";
        graphMLCommunicationInitCondSummaryFileName = fileName + "_communicationInitCondGraphSummary.graphml";
        std::cout << "Emitting GraphML Communication Initial Conditions File: " << path << "/" << graphMLCommunicationInitCondFileName << std::endl;
        Design communicationGraph = CommunicationEstimator::createCommunicationGraph(design, false, false);
        GraphMLExporter::exportGraphML(path + "/" + graphMLCommunicationInitCondFileName, communicationGraph);
        std::cout << "Emitting GraphML Communication Initial Conditions Summary File: " << path << "/" << graphMLCommunicationInitCondSummaryFileName << std::endl << std::endl;
        Design communicationGraphSummary = CommunicationEstimator::createCommunicationGraph(design, true, false);
        GraphMLExporter::exportGraphML(path + "/" + graphMLCommunicationInitCondSummaryFileName, communicationGraphSummary);
    }

    //Check for deadlock among partitions
    CommunicationEstimator::checkForDeadlock(design, designName, path);

    //==== FIFO Merge Cleanup ====
    //Need to rebuild fifoVec, inputFIFOs, and outputFIFOs, since merging may have occurred
    fifoVec.clear();
    inputFIFOs.clear();
    outputFIFOs.clear();
    for(auto it = fifoMap.begin(); it != fifoMap.end(); it++){
        int srcPartition = it->first.first;
        int dstPartition = it->first.second;
        fifoVec.insert(fifoVec.end(), it->second.begin(), it->second.end());

        std::vector<std::shared_ptr<ThreadCrossingFIFO>>& inputFIFOsRef = inputFIFOs[dstPartition];
        inputFIFOsRef.insert(inputFIFOsRef.end(), it->second.begin(), it->second.end());

        std::vector<std::shared_ptr<ThreadCrossingFIFO>>& outputFIFOsRef = outputFIFOs[srcPartition];
        outputFIFOsRef.insert(outputFIFOsRef.end(), it->second.begin(), it->second.end());
    }

    //==== Create State Update Nodes ====
    //Create state update nodes after delay absorption to avoid creating dependencies that would inhibit delay absorption
    DesignPasses::createStateUpdateNodes(design, true); //Done after EnabledSubsystem Contexts are expanded to avoid issues with deleting and re-wiring EnableOutputs
    design.assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
    design.assignArcIDs();

    //==== Check All Nodes have an ID ====
    {
        std::vector<std::shared_ptr<Node>> nodes = design.getNodes();
        if (!MultiThreadEmit::checkNoNodesInIO(nodes)) {
            throw std::runtime_error(
                    ErrorHelpers::genErrorStr("Nodes exist in the I/O partition of the design which are not FIFOs"));
        }
    }

    //==== Schedule Operations within Partitions ===
    IntraPartitionScheduling::scheduleTopologicalStort(design, schedParams, false, true, designName, path, printSched, true); //Pruned before inserting state update nodes

    //Verify the schedule
    design.verifyTopologicalOrder(false, schedType);

    //==== Report Schedule and Partition Information ====
    //Emit the schedule (as a graph)
    std::string graphMLSchedFileName = "";
    if(emitGraphMLSched) {
        //Export GraphML (for debugging)
        graphMLSchedFileName = fileName + "_scheduleGraph.graphml";
        std::cout << "Emitting GraphML Schedule File: " << path << "/" << graphMLSchedFileName << std::endl;
        GraphMLExporter::exportGraphML(path + "/" + graphMLSchedFileName, design);
    }

    //Analyze Partitions
    std::map<int, std::vector<std::shared_ptr<Node>>> partitions = design.findPartitions();

    std::map<int, std::map<EstimatorCommon::NodeOperation, int>> counts;
    std::map<std::type_index, std::string> names;

    for(auto it = partitions.begin(); it != partitions.end(); it++){
        std::pair<std::map<EstimatorCommon::NodeOperation, int>, std::map<std::type_index, std::string>> partCount = ComputationEstimator::reportComputeInstances(it->second, {design.getVisMaster()});
        counts[it->first] = partCount.first;

        for(auto nameIt = partCount.second.begin(); nameIt != partCount.second.end(); nameIt++){
            if(names.find(nameIt->first) == names.end()){
                names[nameIt->first] = nameIt->second;
            }
        }
    }

    //Get the ClockDomainRates in each partition (to report)
    // std::map<int, std::set<std::pair<int, int>>> partitionClockDomainRates = findPartitionClockDomainRates();

    std::cout << "Partition Node Count Report:" << std::endl;
    ComputationEstimator::printComputeInstanceTable(counts, names);
    std::cout << std::endl;

    std::map<std::pair<int, int>, EstimatorCommon::InterThreadCommunicationWorkload> commWorkloads = CommunicationEstimator::reportCommunicationWorkload(fifoMap);
    std::cout << "Partition Communication Report:" << std::endl;
    CommunicationEstimator::printComputeInstanceTable(commWorkloads);
    std::cout << std::endl;

    //==== Emit Design ====

    //Emit Types
    EmitterHelpers::stringEmitTypeHeader(path);

    //Emit FIFO header (get struct descriptions from FIFOs)
    std::string fifoHeaderName = MultiThreadEmit::emitFIFOStructHeader(path, fileName, fifoVec);

    //Emit FIFO Support File
    std::set<ThreadCrossingFIFOParameters::CopyMode> copyModesUsed = MultiThreadEmit::findFIFOCopyModesUsed(fifoVec);
    std::string fifoSupportHeaderName = MultiThreadEmit::emitFIFOSupportFile(path, fileName, copyModesUsed);

    //Emit other support files
    MultiThreadEmit::writePlatformParameters(path, VITIS_PLATFORM_PARAMS_NAME, memAlignment);
    MultiThreadEmit::writeNUMAAllocHelperFiles(path, VITIS_NUMA_ALLOC_HELPERS);

    //====Emit PAPI Helpers====
    std::vector<std::string> otherCFiles;
    std::string papiHelperHFile = "";

    if(EmitterHelpers::usesPAPI(telemLevel) && (printTelem || !telemDumpPrefix.empty())){
        papiHelperHFile = EmitterHelpers::emitPAPIHelper(path, "vitis");
        std::string papiHelperCFile = "vitis_papi_helpers.c";
        otherCFiles.push_back(papiHelperCFile);
    }

    //==== Emit Partitions ====

    //Emit partition functions (computation function and driver function)
    for(auto partitionBeingEmitted = partitions.begin(); partitionBeingEmitted != partitions.end(); partitionBeingEmitted++){
        //Emit each partition (except -2, handle specially)
        if(partitionBeingEmitted->first != IO_PARTITION_NUM) {
            //Note that all I/O into and out of the partitions threads is via thread crossing FIFOs from the IO_PARTITION thread

            //TODO: FIFOs: Discover clock domains and set index expr before calling emitter

            //Find the clock domains in this partition.  This is used to create count variables for clock domains not operating in vector mode.
            std::set<std::shared_ptr<ClockDomain>> clockDomainsInPartition = MultiRateHelpers::findClockDomainsOfNodes(partitionBeingEmitted->second);

            MultiThreadEmit::emitPartitionThreadC(partitionBeingEmitted->first, partitionBeingEmitted->second,
                                                  inputFIFOs[partitionBeingEmitted->first], outputFIFOs[partitionBeingEmitted->first],
                                                  clockDomainsInPartition,
                                                  path, fileName, designName, schedType, design.getOutputMaster(), blockSize, fifoHeaderName,
                                                  fifoSupportHeaderName,
                                                  threadDebugPrint, printTelem,
                                                  telemLevel, telemCheckBlockFreq, telemReportPeriodSec,
                                                  telemDumpPrefix, false, papiHelperHFile,
                                                  fifoIndexCachingBehavior, fifoDoubleBuffer);
        }
    }

    EmitterHelpers::emitParametersHeader(path, fileName, blockSize);

    //====Emit Helpers====
    if(printTelem || !telemDumpPrefix.empty()){
        EmitterHelpers::emitTelemetryHelper(path, fileName);
        std::string telemetryCFile = fileName + "_telemetry_helpers.c";
        otherCFiles.push_back(telemetryCFile);
    }

    if(!telemDumpPrefix.empty()){
        std::map<int, int> partitionToCPU;
        for(auto it = partitions.begin(); it!=partitions.end(); it++) {
            if (partitionMap.empty()) {
                //No thread affinities are set, set the CPUs to -1
                partitionToCPU[it->first] = -1;
            } else {
                //Use the partition map
                if (it->first == IO_PARTITION_NUM) {
                    partitionToCPU[it->first] = partitionMap[0]; //Is always the first element and the array is not empty
                } else {
                    if (it->first < 0 || it->first >= partitionMap.size() - 1) {
                        throw std::runtime_error(ErrorHelpers::genErrorStr("The partition map does not contain an entry for partition " + GeneralHelper::to_string(it->first)));
                    }
                    partitionToCPU[it->first] = partitionMap[it->first + 1];
                }

            }
        }

        MultiThreadEmit::writeTelemConfigJSONFile(path, telemDumpPrefix, designName, partitionToCPU, IO_PARTITION_NUM, graphMLSchedFileName);
    }

    //====Emit I/O Divers====
    std::set<int> partitionSet;
    for(auto it = partitions.begin(); it != partitions.end(); it++){
        partitionSet.insert(it->first);
    }

    //TODO: Modify Drivers to look at MasterIO
    std::pair<std::vector<Variable>, std::vector<std::pair<int, int>>> inputVarRatePair = design.getCInputVariables();
    std::vector<Variable> inputVars = inputVarRatePair.first;

    //++++Emit Const I/O Driver++++
    ConstIOThread::emitConstIOThreadC(inputFIFOs[IO_PARTITION_NUM], outputFIFOs[IO_PARTITION_NUM], path, fileName, designName, blockSize, fifoHeaderName, fifoSupportHeaderName, threadDebugPrint, fifoIndexCachingBehavior);
    std::string constIOSuffix = "io_const";

    //Emit the startup function (aka the benchmark kernel)
    MultiThreadEmit::emitMultiThreadedBenchmarkKernel(fifoMap, inputFIFOs, outputFIFOs, partitionSet, path, fileName, designName, fifoHeaderName, fifoSupportHeaderName, constIOSuffix, partitionMap, papiHelperHFile, useSCHEDFIFO);

    //Emit the benchmark driver
    MultiThreadEmit::emitMultiThreadedMain(path, fileName, designName, constIOSuffix, inputVars);

    //Emit the benchmark makefile
    MultiThreadEmit::emitMultiThreadedMakefileMain(path, fileName, designName, partitionSet,
                                                   constIOSuffix, false, otherCFiles,
                                                   !papiHelperHFile.empty(), !partitionMap.empty());

    //++++Emit Linux Pipe I/O Driver++++
    StreamIOThread::emitFileStreamHelpers(path, fileName);
    std::vector<std::string> otherCFilesFileStream = otherCFiles;
    std::string filestreamHelpersCFile = fileName + "_filestream_helpers.c";
    otherCFilesFileStream.push_back(filestreamHelpersCFile);

    std::string pipeIOSuffix = "io_linux_pipe";
    StreamIOThread::emitStreamIOThreadC(design.getInputMaster(), design.getOutputMaster(), inputFIFOs[IO_PARTITION_NUM],
                                        outputFIFOs[IO_PARTITION_NUM], path, fileName, designName,
                                        StreamIOThread::StreamType::PIPE, blockSize, fifoHeaderName, fifoSupportHeaderName, 0, threadDebugPrint, printTelem,
                                        telemLevel, telemCheckBlockFreq, telemReportPeriodSec, telemDumpPrefix, false,
                                        fifoIndexCachingBehavior, pipeNameSuffix);

    //Emit the startup function (aka the benchmark kernel)
    MultiThreadEmit::emitMultiThreadedBenchmarkKernel(fifoMap, inputFIFOs, outputFIFOs, partitionSet, path, fileName, designName, fifoHeaderName, fifoSupportHeaderName, pipeIOSuffix, partitionMap, papiHelperHFile, useSCHEDFIFO);

    //Emit the benchmark driver
    MultiThreadEmit::emitMultiThreadedMain(path, fileName, designName, pipeIOSuffix, inputVars);

    //Emit the client handlers
    StreamIOThread::emitSocketClientLib(design.getInputMaster(), design.getOutputMaster(), path, fileName, fifoHeaderName, designName);

    //Emit the benchmark makefile
    MultiThreadEmit::emitMultiThreadedMakefileMain(path, fileName, designName, partitionSet,
                                                   pipeIOSuffix, false, otherCFilesFileStream,
                                                   !papiHelperHFile.empty(), !partitionMap.empty());

    //++++Emit Socket Pipe I/O Driver++++
    std::string socketIOSuffix = "io_network_socket";
    StreamIOThread::emitStreamIOThreadC(design.getInputMaster(), design.getOutputMaster(), inputFIFOs[IO_PARTITION_NUM],
                                        outputFIFOs[IO_PARTITION_NUM], path, fileName, designName,
                                        StreamIOThread::StreamType::SOCKET, blockSize,
                                        fifoHeaderName, fifoSupportHeaderName, 0, threadDebugPrint, printTelem,
                                        telemLevel, telemCheckBlockFreq, telemReportPeriodSec, telemDumpPrefix, false,
                                        fifoIndexCachingBehavior, pipeNameSuffix);

    //Emit the startup function (aka the benchmark kernel)
    MultiThreadEmit::emitMultiThreadedBenchmarkKernel(fifoMap, inputFIFOs, outputFIFOs, partitionSet, path, fileName, designName, fifoHeaderName, fifoSupportHeaderName, socketIOSuffix, partitionMap, papiHelperHFile, useSCHEDFIFO);

    //Emit the benchmark driver
    MultiThreadEmit::emitMultiThreadedMain(path, fileName, designName, socketIOSuffix, inputVars);

    //Emit the benchmark makefile
    MultiThreadEmit::emitMultiThreadedMakefileMain(path, fileName, designName, partitionSet,
                                                   socketIOSuffix, false, otherCFilesFileStream,
                                                   !papiHelperHFile.empty(), !partitionMap.empty());

    //++++Emit POSIX Shared Memory FIFO Driver++++
    std::string sharedMemoryFIFOSuffix = "io_posix_shared_mem";
    std::string sharedMemoryFIFOCFileName = "BerkeleySharedMemoryFIFO.c";
    StreamIOThread::emitStreamIOThreadC(design.getInputMaster(), design.getOutputMaster(), inputFIFOs[IO_PARTITION_NUM],
                                        outputFIFOs[IO_PARTITION_NUM], path, fileName, designName,
                                        StreamIOThread::StreamType::POSIX_SHARED_MEM, blockSize,
                                        fifoHeaderName, fifoSupportHeaderName, ioFifoSize, threadDebugPrint, printTelem,
                                        telemLevel, telemCheckBlockFreq, telemReportPeriodSec, telemDumpPrefix, false,
                                        fifoIndexCachingBehavior, pipeNameSuffix);

    //Emit the startup function (aka the benchmark kernel)
    MultiThreadEmit::emitMultiThreadedBenchmarkKernel(fifoMap, inputFIFOs, outputFIFOs, partitionSet, path, fileName, designName, fifoHeaderName, fifoSupportHeaderName, sharedMemoryFIFOSuffix, partitionMap, papiHelperHFile, useSCHEDFIFO);

    //Emit the benchmark driver
    MultiThreadEmit::emitMultiThreadedMain(path, fileName, designName, sharedMemoryFIFOSuffix, inputVars);

    //Emit the benchmark makefile
    std::vector<std::string> otherCFilesSharedMem = otherCFiles;
    otherCFilesSharedMem.push_back(sharedMemoryFIFOCFileName);
    MultiThreadEmit::emitMultiThreadedMakefileMain(path, fileName, designName, partitionSet,
                                                   sharedMemoryFIFOSuffix, true, otherCFilesSharedMem,
                                                   !papiHelperHFile.empty(), !partitionMap.empty());
}