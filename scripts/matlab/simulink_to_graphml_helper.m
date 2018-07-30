%The various stacks need to be passed in but do not need to be returned as
%the parent has a copy of the stack which it can pass to other nodes.

%However, the list of nodes and the current node/edge count need to be
%returned to the caller because these values may have changed durring the
%call.

%simulink_node = handle to simulink node to traverse.
%system_ir_node = the IR node representing the parent system in the
%hierarchy
function [block_ir_node, new_nodes, new_arcs, new_special_nodes] = simulink_to_graphml_helper(simulink_node, system_ir_node, output_master_node, unconnected_master_node, terminator_master_node, vis_master_node, node_handle_ir_map)
%simulink_to_graphml_helper Converts a simulink system to a GraphML file.
%   Detailed explanation goes here

%Side Effects:
% Checks if this node has already been traversed (if so, returns)
% Creates IR node for this node
% Creates IR nodes (if they do not already exist) for each node
% connected to an outgoing arc from the current node (via recursive calls 
% to simulink_to_graphml_arc_follower which calls simulink_to_graphml_helper).
% Creates IR arcs for each each outgoing arc from the node (via call
% to simulink_to_graphml_arc_follower)
% ====
% When following an arc, it may pass through one or more subsystems /
% enabled subsystems.  Because it potentially has to traverse several
% layers, and potentially insert several enabled input/output nodes, a
% seperate helper function (simulink_to_graphml_arc_follower) is used to
% accomplish this task.
% This function calls simulink_to_graphml_arc_follower for each output port
% of the given node.
% simulink_to_graphml_arc_follower makes recursive calls to itself 
% each time a subsystem is encountered and calls to 
% simulink_to_graphml_helper whenever a node (not a subsystem) is 
% encountered (which has not yet already been traversed)

%Returns:
%    IR Node representing Simulink block, regardless of whether it
%    existed before the call to this function or not.

%Detailed Description:
% Conducts a DFS traversal of the simulink graph, transposing to a GraphML
% file as it goes.
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
    % above, DFS terminates for the node and returns

% Because DFS may encounter a node more than once (although should not
% traverse an edge more than once) care must be taken to make sure nodes
% are not duplicated and are not re-evaluated.
% The node handle, which should be unique in the simulink model. 
% A map of simulink handles to IR nodes is kept and is checked before
% adding a node.  The map handle is passed to each recursive call. 
% Because the simulink handles are doubles, they really should only be used
%in matlab.  Node IDs within GraphML will be autoincremented numbers.


% Special Cases

% SubSystem
    % Ports for subsystems are not included (unless the subsystem is 
    % enabled and they are replaced with special nodes - see below).
    
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
        % connected to the Special input/output ports.  Because Special
        % Input/Output ports only occur on the boundaries of an enabled
        % susbsystem and are considered direct children of that subsystem
        % in the IR hierarchy, each Special Input/Output node only need a
        % reference to its parent which contains a reference to the enable 
        % line driver.  Since the driver may not be known until the graph
        % has been completly traversed, the arcs to the enabled lines of
        % the Special Input/Output nodes are created in a pass after the
        % graph traversal using the references stored in the enabled
        % subsystem IR node.

% NOTE: Multiple driver nets are not allowed in simulink and are not
% allowed in the GraphML translation.  However, net fanout is allowed.  The
% net fanout is described as completely seperate arcs in the GraphML
% translation.

% NOTE: Bidirectional or tristate arcs are not allowed.  The Simulink data
% flow graph is assumed to be a directional (not nesssisarily acyclic)
% graph.

% A useful resource for ObjectParameters is https://www.mathworks.com/help/simulink/slref/common-block-parameters.html


%Base cases:
    % Node had already been traversed
    %     Return with inputs passed directly to outputs as appropriate
    % Node is encountered with no output ports
    %     Add node as usual, no recursive calls will be made
    % Terminator node is encountered
    % Visualization node is encountered
        
% ==Begin==

%Init Outputs
new_nodes = [];
new_arcs = [];
new_special_nodes = [];

%Make sure this is the node handle
node_simulink_handle = get_param(simulink_node, 'Handle');

[block_ir_node, node_created] = GraphNode.createNodeIfNotAlready(node_simulink_handle, 'Standard', node_handle_ir_map, system_ir_node);

if node_created
    %This node is not in the map and has therefore never been traversed.
    %Add this new node to the list
    new_nodes = [new_nodes, block_ir_node];
    
    %====Traverse Node=====
    %Get the ouput ports for this node
    port_handles = get_param(node_simulink_handle, 'PortHandles');
    output_port_handles = port_handles.Outport;
    
    %Call the arc follower on each output port
    for i = 1:length(output_port_handles)
        out_port_handle = output_port_handles(i);
        out_port_num = get_param(out_port_handle, 'PortNumber');
        
        %For this initial call to the arc follower, the simulink node/port
        %(the node/port we are tracing from) and the driver node/port are
        %the same (driver node is actuallt the IR representation).  
        %This is because no subsystems could have been traversed yet.
        %The system node (the parent hierarchical node) is not changed
        %because no subsystems have been traversed.
        [new_nodes_recur, new_arcs_recur, new_special_nodes_recur] = simulink_to_graphml_arc_follower(simulink_node, out_port_num, block_ir_node, out_port_num, system_ir_node, output_master_node, unconnected_master_node, terminator_master_node, vis_master_node, node_handle_ir_map);
        
        %Add the new nodes and arcs from arc tracer calls to the list
        new_nodes = [new_nodes, new_nodes_recur];
        new_arcs = [new_arcs, new_arcs_recur];
        new_special_nodes = [new_special_nodes, new_special_nodes_recur];
    end
    
    
end

%If the node already exists it was already traversed

%Return the IR node in either case

end

