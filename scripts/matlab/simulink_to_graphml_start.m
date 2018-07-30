function simulink_to_graphml_start( system, verbose, parent_enabled, parent_enable_src)
%simulink_to_graphml_start Start simulink system to a GraphML conversion.
%   Detailed explanation goes here

% Looks at the given system and checks if it is an enabled subsystem
    % If so, creates input nodes and enabled input nodes.  Input nodes are
    % connected to the "inputs" node.
    % Enabled subssytem is added to the stack
    % Calls helper on each of the inputs 
    
    % Otherwise, creates input nodes and connects them to the "inputs"
    % node.
    % Calls helper on each of the inputs
    
% TODO: Currently only have 1 level of masked susbsystems in design.  Need
% to maintain stack of workspaces?
% -- UPDATE: Does not look like we need to maintain a stack of workspaces
% for nested masked subsystems.  It looks like matlab evaluates earlier
% expressions.
    
system_enabled = is_system_enabled(system);

if system_enabled:
    %Get ports
    
else
    
    
end