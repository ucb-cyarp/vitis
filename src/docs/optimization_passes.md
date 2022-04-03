# Optimization Passes

This is a list of the currently implemented optimization passes and where they are defined.  *This does not represent all optimizations provided by the Lamianr flow as it does not include specialized implementations of DSP blocks or FIFOs or vector implementations of operators*.

Name | Description | Implementation
---- | ----------- | ---------
Blocking/Sub-Blocking | Blocking for amortizing the cost of FIFO transactions and sub-blocking operators within the design for improved intra-core performance | `DomainPasses::blockAndSubBlockDesign`
Expand Enabled Subsystem Context | Extends the enabled subsystem context to include combinational logic at the boarders of the subsystem. | `ContextPasses::expandEnabledSubsystemContexts`
Discover Mux Contexts | Encapsulates the combinational logic at the inputs of a multiplexer into conditionally executed contexts.  Called from ContextPasses::discoverAndMarkContexts(). | `Mux::discoverAndMarkMuxContextsAtLevel`
Grouping Crossings | Discovers sets of partition crossing arcs which can be grouped together in a single FIFO.  Currently, this is restricted to combining arcs from the same output port which are used in multiple input ports in another partition. | `Design::getGroupableCrossings`
FIFO Merging | Merges together FIFOs between the same pair of partitions if possible.  FIFOs become multi-ported with the FIFO contents becomeing a C structure with each port being an element in the structure.  FIFOs to be merged must have the same number of initial conditions.  Initial condition reshaping is performed with excess delays being removed from the FIFO and placed in delays.  FIFOs eligable for merging are currently limited to ones which have the same source and destination contexts.  Merging helps amortize fixed FIFO costs by reducing the number of FIFOs required in the design. | `MultiThreadPasses::mergeFIFOs`
Node Pruning | Prunes unused nodes in the design so that the unused operation is not emitted, inflating the size of the program and possibly leading to performance degradation. | `DesignPasses::prune`
