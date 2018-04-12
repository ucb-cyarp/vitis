%Parent and parent port are important to create the GraphML arc entry.

function [] = simulink_to_graphml_arc_follower(system, node, parent, parent_port, node_list_in, node_count_in, edge_count_in, enabled_parent_stack_in, enabled_status_stack_in, hierarchy_nodeid_stack_in, hierarchy_node_path_stack_in, file, verbose)