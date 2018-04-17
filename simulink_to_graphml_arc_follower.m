%Parent and parent port are important to create the GraphML arc entry.

% The Given Node and Port are important as this routien will trace each
% outgoing arc from the given port.  Each time a subsystem is encountered 
% (and the signal is not driving the enable line, this function recursively 
% calls itself on the input port of the subsystem.  If an subsystem is
% encountered and the port is driving the enable line, an entry is made in
% the enabled system driver maps and the function returns.
% If a standard (not a subsystem) node is encountered, an arc entry is made
% and the graphml helper is called on the node.

% Cases:
    % Encounters general block
    % Encounters normal port of susbsystem
    %    Check if subsystem is already included in graphML and include it if
    %    not already.  Add new name to the map.  Also add to stack in
    %    recursive call or call to helper
    % Encounteres enable input to subsystem
    %    Check if subsystem is already included in graphML and include it if
    %    not already.  Add new name to the map.  Also add to stack in
    %    recursive call or call to helper
    % Encounters output port
        %If not top level subsystem: recursive call to self on arcs from
        %output
        %If top level, connect to appropriate output node and return
    

function [] = simulink_to_graphml_arc_follower(system, node, parent, parent_port, node_list_in, node_count_in, edge_count_in, enabled_parent_stack_in, enabled_status_stack_in, hierarchy_nodeid_stack_in, hierarchy_node_path_stack_in, file, verbose)