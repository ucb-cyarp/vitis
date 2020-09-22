# Current Steps for Generating Designs

## Multithreaded

Entry Point: Design::emitMultiThreadedC

1. Set Partition of I/O Masters:

   Set to IO_PARTITION_NUM
   
2. Propogate Partition from Subsystems:

   Propagate the partition number from subsystems to their child nodes.  This is because, on import, only the subsystems
   contain the partition numbers.  However, each node needs to have its parititiion explicitally set.  This is done in
   Design::propagatePartitionsFromSubsystemsToChildren()
   
3. Pruning (**Optimization Pass**):
   
   The graph is pruned of unused paths and nodes.  The visualization node is currently unimplemented and is included in 
   list of nodes to prune.

4. Unconnected Node and Terminator Disconnect:
   
   Disconnect any remaining nodes from the unconnected and terminator MasterNodes.  Since this occurs after the prune 
   stage, all of the nodes in the design are used.  However, some of their output ports may not be used.  Disconnecting 
   the arcs avoids unnecessary inter-partition FIFO inference for 
   
5. EnabledSubSystem Contect Expansion (**Optimization Pass**):

   Expand enabled subsystems contexts to include combinational nodes at the subsystem boundaries.
   provided by Design::expandEnabledSubsystemContexts
   
6. Discover and Mark Contexts:
   Provided by Design::discoverAndMarkContexts.  This discovers nodes within contexts (ie. 
   within enabled subsystems or within mux mutually exclusive paths).  Part of this pass can be viewed as an 
   **optimization pass** because the mutually exclusive contexts for Muxes are made as large as possible (operation 
   conducted by Mux::discoverAndMarkMuxContextsAtLevel.  Because contexts can be nested, 
   Design::discoverAndMarkContexts calls EnabledSubSystem::discoverAndMarkContexts to perform context discovery in nested
   contexts
   
7. Encapsulate Contexts:
   
   This stage encapsulates the nodes within a context within special container classes called ContextContainers.  Each
   Because there can be multiple logically connected contexts (ex. different inputs of a Mux), ContextContainers are 
   grouped within ContextFamilyContainers.  These containers are used by both the scheduler and emitter to handle 
   contexts.
   
   Nodes within ContextFamilyContainers are scheduled together to avoid unnecessary duplicate conditionals
   (ie. it tries to keep all the operators in a context within a single if/else block).  In the scheduler, this manifits
   as ContextFamilyContainers being treated like BlackBoxes.  All of the arcs in and out of the context are elevated to
   the ContextFamilyContainer from the perspective of the scheduler.  The node is scheduled as a single entitity.
   Since the nodes inside of the Context need to be scheduled themselves, the scheduler is recursively run on the
   contents of the ContextFamilyContainer.
   
   Note that it is not strictly speaking a requirement that the entire ContextFamilyContainer be scheduled together,
   it just reduces the number of condition checks emitted.  This could be relaxed in later versions of the tool.
   
   **Warning: UpsampleClockDomains currently rely on all of their nodes being scheduled together (ie not being split up).
   This currently is provided by the hierarchical implementation of the scheduler.  However, if this were to be changed
   later, a method for having vector intermediates would be required.**
   
   It is important to note that, during encapsulation, a ContextFamilyContainer (and associated ContextContainers) is 
   created for each partition.  This allows the ContextFamilyContainer to be correctly scheduled for each partition.
   It also facilitates automatic FIFO insertion for the context drivers.  During encapsulation, order constraint arcs 
   are created from the context driver to each context family container.  These arcs are also passed to the ContextRoot
   to be used during C code emit so that the correct source is used in each partition.
      
8. Create ContextVariableUpdate Nodes:
   
   Creates ContextVariableUpdate nodes which are used to update the context variable(s) of context root nodes.  The most
   concrete example is that the output of a Mux must be updated by one of the mutually exclusive input contexts in each
   itteration.  This is done after EnabledSubSystem expansion so that any movement of the EnableOutput and EnableInput
   nodes is considered when placing the ContextVariableUpdate nodes.  This is handled by 
   Design::createContextVariableUpdateNodes
   
9. Discover Partition Crossings:

   Discovers the set of arcs that cross partition boundaries.  Also, identify what crossings can be grouped together.
   Currently, only arcs from the same output port to nodes in a single partitions can be grouped.  This is accomplished
   by Design::getGroupableCrossings.
   
10. Partition/Thread Crossing FIFO Insertion:

    Partition/thread crossing FIFOs are spliced into each of the partition crossing arg groups discovered earlier.  This
    is currently accomplished by MultiThreadTransformHelpers::insertPartitionCrossingFIFOs
    
    After creation, the length and block size of each FIFO are set.  Information about the FIFOs is also reported.
    
11. FIFO Delay Ingestion:

    Delays that are next to FIFOs are absorbed to become initial conditions in the FIFO.  This is 
    important when there is a loop between partitions.  This initial stat allows the design to execute without
    deadlocking (assuming enough state was ingested).  Note that delay ingestion is also limited by the length of the 
    FIFO.  This pass is performed by MultiThreadTransformHelpers::absorbAdjacentDelaysIntoFIFOs
    
12. Create StateUpdate Nodes

    State update nodes are created for each node which contains state.  They signify when the state can be updated.
    From a circuits perspective, this is the registering of a flip-flop on a clock edge.  However, in software, there is
    some additional freedom.  The state update can actualy occur any time the current state is no longer in use.  
    Ordering constraint arcs are often used to force these nodes to be scheduled after all nodes depdendnt on the state
    element have been scheduled.  This is accomplished by Design::createStateUpdateNodes
    
13. Intra-Partition Scheduling

    Each partition is scheduled (separately) using a topological sort algorithm.  By scheduling, I am referring to 
    determining the order in which operations in each partition are emitted.  Several different heuristics are available
    for the topological sort algorithm.  Because topological sort algorithms are natural to implement destructively, 
    they are performed on a copy of the design which is taken right before scheduling.  The scheduling is performed by 
    Design::scheduleTopologicalStort.  The scheduler currently works hierarchically with nodes within contexts being 
    scheduled together.
    
    **Warning: UpsampleClockDomains currently rely on all of their nodes being scheduled together (ie not being split up).
    This currently is provided by the hierarchical implementation of the scheduler.  However, if this were to be changed
    later, a method for having vector intermediates would be required.**
    
    After scheduling, a check is make to ensure that no node is scheduled before its arguments are.  This is accomplished
    by Design::verifyTopologicalOrder
    
    At this point, a GraphML file can be exported with the scheduling information.  This can be helpful when debugging
    or analyzing scheduling decisions.
    
14. Workload Reporting:

    A report of how many operations exist within each partition and how much communication is required between partitions
    is exported to the console.
    
15. Emit

    Finally, the C code can be emitted.  This occurs in several stages:
    
    a. Emit Type Header File: This is mostly used to define how boolean types are handled
    
    b. Emit Platform Parameters: This emits some platform specific constants such as memory alignment.
    
    c. Emit NUMA Allocation Helpers: Emits helper functions for allocating within specified NUMA domains
    
    d. Emit each partition:  This emits each partition according to the schedule computed earlier.  It uses each node's
       emitC function to generate the relevant C code.  It also keeps track of when contexts are entered/exited and emits
       the appropriate conditional code.  A thread function is also created for each partition which repeatedly calls
       the emitted partition's compute function.  The compute function also contains an inner loop which loops over
       the samples in a block.  A reset function is also emitted for each partition.  The partition emit is handled by
       MultiThreadEmitterHelpers::emitPartitionThreadC.  The actual emitting of operators (Nodes) within the partition 
       is handled within this function by MultiThreadEmitterHelpers::emitSelectOpsSchedStateUpdateContext
       
    e. Emit Telemetry Helpers (if requested): Emits helper files for telemetry recording if requested.  Also emits the
       telemetry config JSON file to be used by VitisTelemetryDash
       
    f. Emit I/O Drivers: Emits different I/O drivers, each of which includes a kernel file, a driver file, and a makefile.
       Different drivers include Constant, Linux Pipe, Socket Pipe, and POSIX Shared Memory