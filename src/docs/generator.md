# Current Steps for Generating Designs

## GraphML Import
Entry Point: `GraphMLImporter::importGraphML`
1. Parse GraphML file as XML File using Xerces-C (DOM Mode)
2. Create Laminar `design` object
3. Import Nodes
    1. Traverse the DOM Representation of the File.  Recursivly Traverse DOM nodes that are containers such as the document itself, the graphml object, and the graph nodes (nested graphs indicate sub-systems).  Attributes of graphml nodes (which are encoded as child nodes in the DOM) are parsed and converted to string->string maps.
    2. The type name of each node is used to constuct an object of that type via the `NodeFactory::createNode()` factory fuction.  The factory is used due to a querk of the smart pointer system with a smart point being accessable from the object only after creation.  Nodes nested within a subsystem have their parent node reference properly set to preserve the design hierarchy.  Pararameters of the instance are typically populated by a `createFromGraphML()` function present in most nodes.
    3. Arc/Edge nodes in the DOM which are encountered when traversing the XML DOM are placed in a vector to be processed later.
4.  Import Arcs/Edges
    1. Creates an Arc object with the provided attributes such as datatype.  Is connected to ports at the source and destination nodes.
5. Propogate Node Properties: Sometimes nodes cannot completly propagate their properties until arcs have been connected.  This is most common if some node parameter is based on the number of datatype of arcs connected to it.  This is accomplished by calling the `Node::propagateProperties()` function of each node.
6. Discover Clock Domain Properties and Hierarchy
    1. Find all ClockDomain subsystems in the design
    2. Discover RateChange nodes associated with each ClockDomain.  Use rate change note information to set the rate change ratio of the clock domain.
    3. Determine I/O to/from each ClockDomain and set the ClockDomain associated with these I/Os in the Input and Output master nodes.
7. Validate Each Node

## Multithreaded Target Generation
Entry Point: `MultiThreadGenerator::emitMultiThreadedC`

1. Set Partition of I/O Masters:

   Set to IO_PARTITION_NUM
   
2. Propagate Partition from Subsystems:

   Propagate the partition number from subsystems to their child nodes.  This is because, on import, only the subsystems
   contain the partition numbers.  However, each node needs to have its partition explicitly set.  This is done in
   `MultiThreadPasses::propagatePartitionsFromSubsystemsToChildren()`

3. Propagate Sub-blocking Size from Subsystems:

   Propagates the sub-blocking parameter from subsystems to their child node.  This is because, on import, only the subsystems contain the sub-blocking numbers passed via annotations.  However, each node needs the sub-blocking size set.  This is accomplished via `DomainPasses::propagateSubBlockingFromSubsystemsToChildren()`.

   Alternativly, the sub-blocking size is set globally via a CLI parameter.  This will override any sub-blocking design annotations.
   
4. Pruning (**Optimization Pass**):
   
   The graph is pruned of unused paths and nodes.  The visualization node is currently unimplemented and is included in 
   list of nodes to prune.
   Provided by `DesignPasses::prune()`

5. Unconnected Node and Terminator Disconnect:
   
   Disconnect any remaining nodes from the unconnected and terminator MasterNodes.  Since this occurs after the prune 
   stage, all the nodes in the design are used.  However, some of their output ports may not be used.  Disconnecting 
   the arcs avoids unnecessary inter-partition FIFO inference.

6. Clock Domain Handling
    1. Find Clock Domains in design.  Done by searching nodes in design and finding ClockDomain subsystems.
    2. Re-discover ClockDomain parameters (such as RateChange nodes assocated with each clock domain, rate change associated with each ClockDomain and I/O operating within each ClockDomain) that were initially discovered durring graph import.  Re-discovery is required since nodes/arcs may have been pruned earlier.
    3. Specialize ClockDomains to UpsampleClockDomains or DownsampleClockDomains.  This also converts RateChange nodes to specilized Input or Output nodes.  This operation replaces ClockDomain and RateChange nodes with their specialized equivalents rather then changing a prameter in the existing node.   *Note: Only DownsampleClockDomains are currently supported.*
    4. Create ClockDomain Support Nodes.  For example, downsample domains not operating in vector mode require a counter to determine when they should execute.
    5. Validate ClockDomain Rates
   
7. EnabledSubSystem Context Expansion (**Optimization Pass**):

   Expand enabled subsystems contexts to include combinational nodes at the subsystem boundaries.
   Provided by ContextPasses::expandEnabledSubsystemContexts

8. Assign Partitions and Sub-Blocking Size to Unassigned Subsystems
   
   It can sometimes happen, especially near the top of the hierarchy, that subsystems may not be assigned a partition or sub-blocking size.  This would not be an issue except when blocking domains are later formed.  The partition and sub-blocking is pulled from one of the nodes within the subsystem.

9. Place EnableNodes (EnableInput and EnableOutput) in Partitions:

   Places EnableNodes in partitions if they are not already.  Done after EnabledSubsystem expansion when new enable nodes may have been created.
   
10. Discover and Mark Contexts:

    Provided by `ContextPasses::discoverAndMarkContexts`.  This discovers nodes within contexts (ie. 
    within enabled subsystems or within mux mutually exclusive paths).  Part of this pass can be viewed as an 
    **optimization pass** because the mutually exclusive contexts for Muxes are made as large as possible (operation 
    conducted by `Mux::discoverAndMarkMuxContextsAtLevel`.  Because contexts can be nested,
    `ContextPasses::discoverAndMarkContexts` calls `EnabledSubSystem::discoverAndMarkContexts` to perform context discovery 
    in nested contexts

11. Replicate ContextRoot Drivers (If Requesteed):

    Performed by `ContextPasses::replicateContextRootDriversIfRequested` and primarily used for ClockDomains.  Since clock domains not operating in vector mode can be conditionally executed by a counter, having a clock domain spread across multiple partitions would result in communication from the counter to the clock domain in different partitions.  Since the computation required for the counter is inexpensive, it makes more sense to replicate the counter in each partition.  Currently, the ContextRoot driver must be a single node with no inputs.

12. Blocking & Sub-Blocking (**Optimization Pass**)

    1. Set Master Blocks' Origional Datatypes:

       Performed by `DomainPasses::setMasterBlockOrigDataTypes`, sets the origional datatype property of each input/output into and out of the system.  This should be done before blocking as the dimensions of the I/O arcs will be expanded.

    2. Block and Sub-block Design:

       Creates the Blocking and Sub-Blocking domains within the design.  The entire design is wrapped in a blocking domain which replaces the earlier Laminar special cased logic for handling blocked FIFO transactions.  Sub-blocking occurs by finding strongly connected components within the design (after delays which are greater than the sub-blocking length are disconnected).  Nodes within a strongly connected compoent must be scheduled together.  Currently, nodes within contexts (ex. Muxes or EnabledSubsystems) are contained within sub-blocking domains together.  One exception is ClockDomains operating in vector mode, where the block size outside of the clock domain and inside the clock domain are both integers and are related by the decimation ratio.  Nodes within these clock domains can be sub-blocked using the strongly connected component analysis.  Some nodes, such as TappedDelays, and FIR Filters, have specialized blocking implementations.  Their specialized implementation is used instead of a sub-blocking domain.

       *Note: Blocking/SubBlocking domains are treated as special contexts for the rest of the compiler flow.*

       *Note: There is currently a restriction that all nodes within a single partition share the same base sub-blocking factor.*
   
    3. Set Master Block Sizes (Based on Clock Domains):

       Sets the number of samples per block on I/O arcs based on the provided base blocking size and the relative rate of the clock domain this I/O communicates to/from.
   
13. Encapsulate Contexts:
   
    This stage encapsulates the nodes within a context within special container classes called ContextContainers.  Each
    Because there can be multiple logically connected contexts (ex. different inputs of a Mux), ContextContainers are 
    grouped within ContextFamilyContainers.  These containers are used by both the scheduler and emitter to handle 
    contexts.
   
    Nodes within ContextFamilyContainers are scheduled together to avoid unnecessary duplicate conditionals
    (ie. it tries to keep all the operators in a context within a single if/else block).  In the scheduler, this 
    manifests as ContextFamilyContainers being treated like BlackBoxes.  All of the arcs in and out of the context are 
    elevated to the ContextFamilyContainer from the perspective of the scheduler.  The node is scheduled as a single 
    entity. Since the nodes inside of the Context need to be scheduled themselves, the scheduler is recursively run on the
    contents of the ContextFamilyContainer.
   
    Note that it is not strictly speaking a requirement that the entire ContextFamilyContainer be scheduled together,
    it just reduces the number of condition checks emitted.  This could be relaxed in later versions of the tool.
   
    It is important to note that, during encapsulation, a ContextFamilyContainer (and associated ContextContainers) is 
    created for each partition.  This allows the ContextFamilyContainer to be correctly scheduled for each partition.
    It also facilitates automatic FIFO insertion for the context drivers.  During encapsulation, order constraint arcs 
    are created from the context driver to each context family container.  These arcs are also passed to the ContextRoot
    to be used during C code emit so that the correct source is used in each partition.
      
14. Create ContextVariableUpdate Nodes:
   
    Creates ContextVariableUpdate nodes which are used to update the context variable(s) of context root nodes.  The most
    concrete example is that the output of a Mux must be updated by one of the mutually exclusive input contexts in each
    itteration.  This is done after EnabledSubSystem expansion so that any movement of the EnableOutput and EnableInput
    nodes is considered when placing the ContextVariableUpdate nodes.  This is handled by
   ContextPasses::createContextVariableUpdateNodes
   
15. Discover Partition Crossings:

    Discovers the set of arcs that cross partition boundaries.  Also, identify what crossings can be grouped together.
    Currently, only arcs from the same output port to nodes in a single partitions can be grouped.  This is accomplished
    by `Design::getGroupableCrossings`.
   
16. Partition/Thread Crossing FIFO Insertion:

    Partition/thread crossing FIFOs are spliced into each of the partition crossing arg groups discovered earlier.  This
    is currently accomplished by `MultiThreadPasses::insertPartitionCrossingFIFOs`
    
    After creation, the length and block size of each FIFO are set (via `MultiRateHelpers::setFIFOClockDomainsAndBlockingParams`).

17. FIFO Delay Ingestion:

    Delays that are next to FIFOs are absorbed to become initial conditions in the FIFO.  This is 
    important when there is a loop between partitions.  This initial stat allows the design to execute without
    deadlocking (assuming enough state was ingested).  Note that delay ingestion is also limited by the length of the 
    FIFO.  This pass is performed by `MultiThreadPasses::absorbAdjacentDelaysIntoFIFOs`
    
18. FIFO Merging (**Optimization Pass**)

    Identifies FIFOs going between pairs of partitions that can be merged together into a single multi-ported FIFO, reducing the number of FIFOs in the system.  FIFO elements become structures with each port becoming an element.  Merging is currently restricted such that all FIFOs to be merged must have the same source and destination contexts.  FIFOs to be merged must have the same number of initial conditions.  Initial condition re-shaping is performed so that each FIFO to be merged has the same number of initial conditions, with excess initial conditions being shifted into delay nodes.

19. Deferred Delay Speecialization (part of Sub-Blocking)

    This performs specialization on delay nodes that would have occured durring sub-blocking if it did not interfere with FIFO delay absorption.  Because delay absorption changes the delay values and initial conditions, delays are left in their standard form after the sub-blocking pass.  After FIFO delay absorption occures, the specialization of delays can now occure with the delay values and datatypes properly set. 

20. Validate Deffered Delays and Merged FIFOs

21. FIFO and Communication Reporting

22. Inter-Partition Deadlock Check

    Checks that no partition participates in a cycle with other partitions in which there is no initial state.  The check is performed by ensuring that a hypothetical topological ordering of partitions exists (with each partition treated as a black box).

23. Create StateUpdate Nodes

    State update nodes are created for each node which contains state.  They signify when the state can be updated.
    From a circuits perspective, this is the registering of a flip-flop on a clock edge.  However, in software, there is
    some additional freedom.  The state update can actually occur any time the current state is no longer in use.  
    Ordering constraint arcs are often used to force these nodes to be scheduled after all nodes dependent on the state
    element have been scheduled.  This is accomplished by DesignPasses::createStateUpdateNodes
    
24. Intra-Partition Scheduling

    Each partition is scheduled (separately) using a topological sort algorithm.  By scheduling, I am referring to 
    determining the order in which operations in each partition are emitted.  Several different heuristics are available
    for the topological sort algorithm.  Because topological sort algorithms are natural to implement destructively, 
    they are performed on a copy of the design which is taken right before scheduling.  The scheduling is performed by
    IntraPartitionScheduling::scheduleTopologicalStort.  The scheduler currently works hierarchically with nodes within 
    contexts being scheduled together.
    
    **Warning: UpsampleClockDomains currently rely on all of their nodes being scheduled together (ie not being split up).
    This currently is provided by the hierarchical implementation of the scheduler.  However, if this were to be changed
    later, a method for having vector intermediates would be required.**
    
    After scheduling, a check is make to ensure that no node is scheduled before its arguments are.  This is accomplished
    by Design::verifyTopologicalOrder
    
    At this point, a GraphML file can be exported with the scheduling information.  This can be helpful when debugging
    or analyzing scheduling decisions.
    
25. Workload Reporting:

    A report of how many operations exist within each partition and how much communication is required between partitions
    is exported to the console.
    
26. Emit

    Finally, the C code can be emitted.  This occurs in several stages:
    
    1. Emit Type Header File: This is mostly used to define how boolean types are handled
    
    2. Emit Platform Parameters: This emits some platform specific constants such as memory alignment.
    
    3. Emit NUMA Allocation Helpers: Emits helper functions for allocating within specified NUMA domains
    
    4. Emit each partition:
    
       This emits each partition according to the schedule computed earlier.  It uses each node's
       emitC function to generate the relevant C code.  It also keeps track of when contexts are entered/exited and emits
       the appropriate conditional code.  A thread function is also created for each partition which repeatedly calls
       the emitted partition's compute function.  A reset function is also emitted for each partition.  The partition emit is handled by
       `MultiThreadEmitterHelpers::emitPartitionThreadC`.  The actual emitting of operators (Nodes) within the partition 
       is handled within this function by `MultiThreadEmit::emitSelectOpsSchedStateUpdateContext`
       
    5. Emit Telemetry Helpers (if requested): Emits helper files for telemetry recording if requested.  Also emits the
       telemetry config JSON file to be used by VitisTelemetryDash
       
    6. Emit I/O Drivers: Emits different I/O drivers, each of which includes a kernel file, a driver file, and a makefile.
       Different drivers include Constant, Linux Pipe, Socket Pipe, and POSIX Shared Memory