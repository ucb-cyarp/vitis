%Parent and parent port are important to create the GraphML arc entry.

% The Given Node and Port are important as this routien will trace each
% outgoing arc from the given port.  Each time a subsystem is encountered 
% (and the signal is not driving the enable line, this function recursively 
% calls itself on the input port of the subsystem.  If an subsystem is
% encountered and the port is driving the enable line, an arc is made and
% the enable driver in the IR node corresponding to the subsystem is 
% populated and the function returns. If a standard (not a subsystem) node
% is encountered, an arc entry is made and the graphml helper is called on 
% the node.

% Cases:
    % Encounters general block
    %    Calls simulink to graphml helper
    % Encounters normal port of susbsystem
    %    Check if subsystem already has an IR node created for it and
    %    create one if it does not.  Add entries to the approproate node
    %    maps.  Add the node to the hierarchy stack.
    %    *If susbsystem is enabled
    %        Create Special Input Port IR node, set the parent to the node 
    %        representing the enabled subsystem, add it to the appropriate maps
    %        and create an arc connecting the signal driver to it.  Assign
    %        the handle of the simulink port node it is representing to
    %        this IR node.  Add the subsystem to the stack and recursivly 
    %        call the arc follower on the output of the simulink port but 
    %        give the new Special Input port IR node as the driver.
    %    *If subsystem is not enabled
    %        Add the subsystem to the stack and recursivly 
    %        call the arc follower on the output of the simulink port.  Do
    %        not change the driver node or port
    % Encounteres enable input to subsystem
    %    Check if subsystem already has an IR node created for it and
    %    create one if it does not.  Add entries to the approproate node
    %    maps.  Populate the "enable driver" entries of the subsystem's
    %    corresonding IR node to the node and port driving this line
    % Encounters output port
    %    If not top level subsystem:
    %        If output port of enabled susbsystem:
    %            If output port reached, subsystem IR node must already
    %            exist.  Create Special Output Port IR node, add it to the
    %            appropriate maps, and creat an arc connecting the signal
    %            driver to it.  Assign the handle of the simulink port node
    %            it is representing to this IR node.  Pop the subsystem off
    %            of the stack and recursively call the arc follower on the
    %            output of the associated subsystem port but give the new
    %            Special Output port IR node as the driver
    %        If ouput port of an non-enabled subsystem:
    %            Pop the subsystem off of the stack and recursively call 
    %            the arc follower on the output of the associated subsystem
    %            port but do not change the driver    
    %    If top level, connect to appropriate output node and return
    
% Note, Special Input/Output ports have a reference to the enabled
% subsystem which controls their behavior.  Once the graph traversal is
% complete and the drivers of the enable ports are discovered, a pass 
% creates arcs from the enabled subsystem drivers to each Special
% Input/Output node.
    
% Pass in GraphNode as the driver node and port number as the driver port.  
% The simulink names can be fetched using the simulink handle saved in the 
% IR if needed.  Special nodes are assigned handles to the port nodes in 
% simulink that they represent.  Simulink ports that are not at the 
% boundary of an enabled subsystem do not have associated IR nodes.

%The node and port from which we are tracing are given as simulink handles
%since we may recurse on input/output ports in non-enabled subsystems which
%will not have assiated IR nodes.
        
function [] = simulink_to_graphml_arc_follower(system, node, parent, parent_port, node_list_in, node_count_in, edge_count_in, enabled_parent_stack_in, enabled_status_stack_in, hierarchy_nodeid_stack_in, hierarchy_node_path_stack_in, file, verbose)