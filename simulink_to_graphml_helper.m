%The various stacks need to be passed in but do not need to be returned as
%the parent has a copy of the stack which it can pass to other nodes.

%However, the list of nodes and the current node/edge count need to be
%returned to the caller because these values may have changed durring the
%call.

function [graphml_node_str, ...
          graphml_arc_str, ...
          node_map_out, ...
          node_count_out, ...
          arc_count_out, ...
          enabled_input_ports_out, ...
          enabled_input_ports_system_out, ...
          enabled_output_ports_out, ...
          enabled_output_ports_system_out, ...
          enabled_system_driver_node_map_out, ...
          enabled_system_driver_port_map_out] = ...
          ...
          simulink_to_graphml_helper(node, ...
          node_map_in, ...
          node_count_in, ...
          arc_count_in, ...
          enabled_status_stack_in, ...
          hierarchy_nodeid_stack_in, ...
          hierarchy_node_path_stack_in, ...
          ... % For collecting Special input and output ports which will need to be connected to enable drivers at the end (when all will have been discovered).  Cannot rely on them having been discovered until the entire graph has been traversed (regardless of whether DFS or BFS traversal is used)
          enabled_input_ports_in, ...
          enabled_input_ports_system_in, ...
          enabled_output_ports_in, ...
          enabled_output_ports_system_in, ...
          ... %for recording the drivers for enabled subsystems as they are encountered.
          enabled_system_driver_node_map_in, ...
          enabled_system_driver_port_map_in, ...
          verbose)
%simulink_to_graphml_helper Converts a simulink system to a GraphML file.
%   Detailed explanation goes here

%Args:
% node                         = The name of the node.  The full path to 
%                                the node is {system}/{node}.  system can 
%                                be generatedby concatenating entries in
%                                hierarchy_node_path_stack_in
% node_map_in                  = The map of nodes that have already been 
%                                traversed (and a GraphML has already been 
%                                created for).  It maps to the full node
%                                name in GraphML.  Arcs to these nodes are 
%                                allowed but no additional traversal of the
%                                node should be preformed.
% node_count_in                = Number of nodes which existed before the 
%                                call to this function.  Used for 
%                                autoincrementing IDs of new nodes
% arc_count_in                 = Number of arcs which existed before the 
%                                call to this function.  Used for 
%                                autoincrementing IDs of new arcs
%                                lines for the subsystem in the hierarchy
% enabled_status_stack_in      = A stack indecating if a given subsystem in
%                                the hierarchy is enabled or not
% hierarchy_nodeid_stack_in    = A stack of the hierarchy in GraphML
%                                nodeids.  Used to generate GraphML
%                                hierarchical node prefix
% hierarchy_node_path_stack_in = A stack of the hierarchy in Simulink node
%                                names. Used to generate simulink system 
%                                path
% verbose                      = Indicates if debug text should be written

%Returns:
% graphml_node_str             = GraphML XML for creating nodes which were
%                                traversed in the call.  Includes nodes
%                                traversed in recursive calls
% graphml_arc_str              = GraphML XML for creating arcs which were
%                                traversed in the call.  Includes arc
%                                traversed in recursive calls
% node_map_out                 = Map of traversed nodes after the call to
%                                this function.  Includes nodes traversed
%                                in recursive calls.  Maps to GraphML name
% node_count_out               = The new number of nodes after the call to
%                                this function.  Includes nodes traversed
%                                in recursive calls
% arc_count_out                = The new number of arcs after the call to
%                                this function.  Includes arcs traversed
%                                in recursive calls

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
% 


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
%Copy inputs to outputs in preparation for return or modification before
%return
graphml_node_str = {};
graphml_arc_str = {};
node_map_out = node_map_in;
node_count_out = node_count_in;
arc_count_out = arc_count_in;
enabled_input_ports_out = enable_input_ports_in;
enabled_input_ports_system_out = enabled_input_ports_system_in;
enabled_output_ports_out = enabled_output_ports_in;
enabled_output_ports_system_out = enabled_output_ports_system_in;
enabled_system_driver_node_map_out = enabled_system_driver_node_map_in;
enabled_system_driver_port_map_out = enabled_system_driver_port_map_in;

%Get simulink paths
system_path_simulink = get_system_path(hierarchy_node_path_stack_in, '/');
node_path_simulink = [system_path_simulink, '/', node];

%Check if node has already been traversed
node_handle = get_param(node_path_simulink, 'Handle');

if ~isKey(node_map_out, node_handle)
    %This node is not in the node map and has therefore not yet been
    %traversed.  Traverse it now
    
    %Get a new node number
    node_id_number = node_count_out + 1;
    node_count_out = node_count_out + 1; % Update the count
    
    %Get the node path
    node_ml_name = ['n' num2str(node_id_number)];
    graph_ml_system_path = get_system_path(hierarchy_nodeid_stack_in, '::');
    graph_ml_node_path = [graph_ml_system_path, '::', node_ml_name];
    
    %Add it to the node map
    node_list_out(node_handle) = graph_ml_node_path;
    
    %Create the node entry and add it to the list
    %======TODO======
    
    %Traverse:
    %Get a list of ports for the given node
    ports = get_param(node_path_simulink, 'PortHandles');
    
    %Call arc follower on ports
    %======TODO======
    
    
end



        
end

