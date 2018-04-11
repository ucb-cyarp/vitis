function simulink_to_graphml_helper( system, verbose, parent_enabled, parent_enable_src)
%simulink_to_graphml_helper Converts a simulink system to a GraphML file.
%   Detailed explanation goes here

% Conducts a DFS traversal of the simulink graph, transposing to a GraphML
% file as it goes. (Need to traverse the entire tree so may as well use
% DFS rather than BFS).

% Start: Virtual nodes "inputs", "outputs", "unconnected", "terminated" and 
% "visualization" are created.  Input ports are made into nodes and 
% connected to the "inputs" node.

% DFS Start: DFS traversial starts from "inputs node"

% DFS End:
    % If any output port of the top level system is reached, a node is
    % created and and connected to the "outputs" node
    
    % If any block is reached which has an output port which is unconnected
    % , an arc from that node is drawn to the "unconnected" node
    
    % If any terminator node is reached, an arc is drawn from the parent
    % node to the "terminated" node.  The terminator node is not drawn.
    
    % If any visualization node is reached, the parent node is connected to
    % the "visualization" node.
    
    % If a node is reached that has no output arcs (and is not covered
    % above, BFS terminates for the node and returns

% Because DFS may encounter a node more than once (although should not
% traverse an edge more than once) care must be taken to make sure nodes
% are not duplicated.  The node handle, which should be unique in the
% simulink model, is used as the node ID.  A list of node IDs of created
% nodes is kept and is checked before adding a node.  The list is passed to
% each recursive call and is returned back.

% Special Cases

% SubSystem
    % Ports for subsystems are removed (unless the subsystem is enabled and
    % they are replaced with special nodes - see below).
    
    % A node is created in GraphML to represent the subsystem.  Nodes
    % within the subsystem use the name format subsystem::internal_node to
    % denote the hierarchical node.
    
    % Nested SubSystems
        % Becasuse subsystems can be nested, a stack is maintained to track
        % where in the hierarchy the traversal is currently.  Each time a
        % subsystem is entered, its name is pushed to the stack.  Each time
        % an output node of a system is reached, the name is popped from
        % the stack.
    

% Eanbled Subsystem
    % Modify Output Port To be Nodes With Memory and Input from Enable Line
        % Determines whether output is previous value or new value
        % OUTPUT PORT IS A SPECIAL NODE IN THIS CASE
        
    % Modify Input Ports to be 'Gated' with Enable Line
        % Determines if data is passed to dependent operations in this
        % "sample cycle" or if nothing is passed.  Dependant blocks will
        % stall waiting for dependant information.  Modified output ports
        % break this stalling for dependent operations outside of enabled
        % subsystems
        % INPUT PORT IS A SPECIAL NODE IN THIS CASE
        
    % NOTE: When generating code, the enable signal (true or false) is sent
    % to the output ports in each "sample cycle".  Even if the enabled code
    % is not called by the SW, the output ports still know to produce a
    % value.  This is one reason why the output ports are special nodes
    % with special semantics.
        
    % Nested Enabled SubSystems
        % The same semantics shown above apply to nested enabled subsystems
        % The difference is that the "enable" line which needs to be
        % connected to the input/output ports.  Because there may be many
        % levels of enabled subsystems, a stack is used to track the
        % hierarchy of enabled subsystems.  Each time an enabled subsystem
        % is entered, the enable line is pushed to the stack.  Each time
        % the output port of an enabled subsystem is reached, the enable
        % line is popped from the stack.
        
        % NOTE: To determine if the output belongs to an enabled subsystem,
        % a stack indecating if a subsystem is enabled or not is also kept.

% NOTE: Multiple driver nets are not allowed in simulink and are not
% allowed in the GraphML translation.  However, net fanout is allowed.  The
% net fanout is described as completely seperate arcs in the GraphML
% translation.

% NOTE: Bidirectional or tristate arcs are not allowed.  The Simulink data
% flow graph is assumed to be a directional (not nesssisarily acyclic)
% graph.
        
% ==Begin==

        
end

