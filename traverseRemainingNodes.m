function [new_nodes, new_arcs, new_special_nodes] = traverseRemainingNodes(simulink_system, system_ir_node, output_master_node, unconnected_master_node, terminator_master_node, vis_master_node, node_handle_ir_map)
%traverseRemainingNodes This function finds each node within the specified
%subsystem.  If the node is a subsystem, it creates a subsystem node if
%none already exists and calls itself recursivly on that node.  If the node
%is a standard node (not a visualizer node, Enable node, port node), it
%calls the helper on that node.
%
% Since the helper checks if the node is already in the node map, the
% helper can be called on each node without creating duplicates.  Also,
% since edges are only formed by tracing outgoing arcs from a node being
% traversed, duplicate nodes will not be created.  Nodes can be traversed
% out of order, 
%
% This function is used as a clean up pass after the forward traversal
% occurs.
%
% This function traverses the children of the simulink_system.
% The system ir_node is the ir node representing this system

%Init Return Values
new_nodes = [];
new_arcs = [];
new_special_nodes = [];

%Get the handle for the simulink_system
simulink_system_handle = get_param(simulink_system, 'Handle');

%Get the list of of children for the simulink subsystem (not from the IR
%node)
%Note, this list also includes the system itself
children = find_system(simulink_system_handle,  'FollowLinks', 'on', 'LoadFullyIfNeeded', 'on', 'LookUnderMasks', 'on', 'SearchDepth', 1);

for i = 1:length(children)
    child = children(i);
    child_handle = get_param(child, 'Handle');
    
    if child_handle ~= simulink_system_handle
        %This is actually one of the children, needed because the system
        %itself is returned in the search
        
        child_type = get_param(child_handle, 'Type');
        
        if strcmp(child_type, 'block')
            %This is a block, check if it should be traversed
            
            child_block_type = get_param(child_handle, 'BlockType');
            child_reference_block = get_param(child_handle, 'ReferenceBlock');
            
            %Special case for the FIFO subsystem.
            if strcmp(child_block_type, 'SubSystem') && strcmp(child_reference_block, 'hdlsllib/HDL RAMs/HDL FIFO')
                %This is a special FIFO block and is treated seperatly (as a
                %primitive rather than a subsystem.

                %TODO: IMPLEMENT
                errror('HDL FIFO Not Yet Implemented');
                
            elseif strcmp(child_block_type, 'SubSystem')
                %This is a subsystem
                system_enabled = is_system_enabled(child_handle);
                
                %Create (or get) the correct IR node 
                if system_enabled
                    [child_ir_node, node_created] = GraphNode.createNodeIfNotAlready(child_handle, 'Enabled Subsystem', node_handle_ir_map, system_ir_node);
                    if node_created
                        new_nodes = [new_nodes, child_ir_node];
                    end
                else
                    [child_ir_node, node_created] = GraphNode.createNodeIfNotAlready(child_handle, 'Subsystem', node_handle_ir_map, system_ir_node);
                    if node_created
                        new_nodes = [new_nodes, child_ir_node];
                    end
                end
                
                %Recurse on this child
                [new_nodes_recur, new_arcs_recur, new_special_nodes_recur] = traverseRemainingNodes(child_handle, child_ir_node, output_master_node, unconnected_master_node, terminator_master_node, vis_master_node, node_handle_ir_map);
                new_nodes = [new_nodes, new_nodes_recur];
                new_arcs = [new_arcs, new_arcs_recur];
                new_special_nodes = [new_special_nodes, new_special_nodes_recur];
                
            elseif ~strcmp(child_block_type, 'Inport') && ~strcmp(child_block_type, 'Outport') && ~strcmp(child_block_type, 'EnablePort') && ~(strcmp(child_block_type, 'ConstellationDiagram') || strcmp(child_block_type, 'Scope') || strcmp(child_block_type, 'Display') || strcmp(child_block_type, 'SpectrumAnalyzer')) && ~strcmp(child_block_type, 'Terminator')
                %This is a node which should be traversed (not a port, 
                %enable, vis, or terminator)
                
                [block_ir_node, new_nodes_recur, new_arcs_recur, new_special_nodes_recur] = simulink_to_graphml_helper(child_handle, system_ir_node, output_master_node, unconnected_master_node, terminator_master_node, vis_master_node, node_handle_ir_map);
                new_nodes = [new_nodes, new_nodes_recur];
                new_arcs = [new_arcs, new_arcs_recur];
                new_special_nodes = [new_special_nodes, new_special_nodes_recur];
            end
        else
            %This is not a block
            child_name = get_param(child_handle, 'Name');
            warning([child_name ' is not a Simulink block ... skipping']);
        end
    end
end

