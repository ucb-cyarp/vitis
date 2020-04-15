# Optimization Passes

This is a list of the currently implemented optimization passes and where they are defined:

Name | Description | Implementation
---- | ----------- | ---------
Expand Enabled Subsystem Context | Extends the enabled subsystem context to include combinational logic at the boarders of the subsystem. | Design::expandEnabledSubsystemContexts
Discover Mux Contexts | Encapsulates the combinational logic at the inputs of a multiplexer into conditionally executed contexts.  Called from Design::discoverAndMarkContexts(). | Mux::discoverAndMarkMuxContextsAtLevel
Grouping Crossings | Discovers sets of partition crossing arcs which can be grouped together in a single FIFO.  Currently, this is restricted to combining arcs from the same output port which are used in multiple input ports in another partition. | Design::getGroupableCrossings
Node Pruning | Prunes unused nodes in the design so that the unused operation is not emitted, inflating the size of the program and possibly leading to performance degridation. | Design::prune
