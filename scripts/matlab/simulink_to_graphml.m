function simulink_to_graphml(simulink_file, system, graphml_filename, verbose_in)
%simulink_to_graphml Converts a simulink system to a GraphML file.
%   Detailed explanation goes here

global verbose;

if ~exist('verbose_in', 'var')
    verbose = 0;
else
    verbose = verbose_in;
end

%% Load Simulink and put into compile mode
if verbose >= 1
    disp('[SimulinkToGraphML] **** Opening System ****');
end

%Open the system
open_system(simulink_file);

%Get function for simulink top level system
top_system_func = str2func(simulink_file);
top_system_func([], [], [], 'compile');

%% Traverse Graph and Transcribe to GraphML File
%simulink_to_graphml_helper(system, verbose, false, 0);


% NOTE: The GraphML spec creates nested graph elements within node
% elements.  This is how they handle nested graphs.  This means that all
% node declarations need to respect the hierarchy and (as a concequence)
% many not be declared in any order.
% However, it appears that the declarations of edges is more flexible.
% Edges must be declared in a graph that is an ancestor of both the source
% and destination nodes.  This does restrict edge declaration.  However, as
% noted in the primer
% (http://graphml.graphdrawing.org/primer/graphml-primer.html) all edges
% can be declared at the top level.

%Thus, we will keep a datastructure representing the hierarchy of the nodes
%in the graph.  This will take the form of a map from a node to an array of
%nodes that are direct children of it in the hierarchy.

%We will declare all edges at the top level (at least to start with).

%Due to the specifics of Simulink and GraphML, this script works in 2
%phases:
%    1. The graph structure is extracted from Simulink into an intermediate
%       representation which is easier to deal with.
%    2. The intermediate representation is outputted to GraphML in
%       accordance with it's requirements

%% Create Top Level IR Node -> Set Type to Top Level

if verbose >= 1
    disp('[SimulinkToGraphML] **** Creating Top Level Nodes ****');
end

%% Create Virtual Nodes
top_level_ir_node = GraphNode(system, 'Top Level', []);
top_level_ir_node.nodeId = 0;

%% Find Inports Within System
input_master_node = GraphNode('Input Master', 'Master', top_level_ir_node);
input_master_node.nodeId = 1;
output_master_node = GraphNode('Output Master', 'Master', top_level_ir_node);
output_master_node.nodeId = 2;
vis_master_node = GraphNode('Visualization Master', 'Master', top_level_ir_node);
vis_master_node.nodeId = 3;
unconnected_master_node = GraphNode('Unconnected Master', 'Master', top_level_ir_node);
unconnected_master_node.nodeId = 4;
terminator_master_node = GraphNode('Terminator Master', 'Master', top_level_ir_node);
terminator_master_node.nodeId = 5;

master_nodes = [input_master_node, output_master_node, vis_master_node, unconnected_master_node, terminator_master_node];

%Put workspace vars in terminator master.  Need a node which is actually in
%the graph.  The top level node is actually not emitted.
PopulateTopLevelNodeWorkspaceVars(terminator_master_node);

%% Create the lists to keep track of nodes and arcs as well as the node map
nodes = [];
arcs = [];
special_nodes = [];
node_handle_ir_map = containers.Map('KeyType','double','ValueType','any');

if verbose >= 1
    disp('[SimulinkToGraphML] **** Traversing Simulink Graph from Input Ports ****');
end

%% Call Arc Follower on Each With Driver Set To Input Virtual Node and Port 1 (Also Add Name to Input Master Node)
%Get a list of Inports within system
inports = find_system(system,  'FollowLinks', 'on', 'LoadFullyIfNeeded', 'on', 'LookUnderMasks', 'on', 'SearchDepth', 1, 'BlockType', 'Inport');

%Call the arc follower on each
for i = 1:length(inports)
    inport = inports(i);
    
    %Get the port number of the inport, this will be used to distinguish
    %the different inputs.
    port_number_str = get_param(inport, 'Port'); %Returned as a string
    port_number = str2double(port_number_str);
    
    %Get port name and create entry in Input Master Node
    port_name = get_param(inport, 'Name');
    input_master_node.outputPorts{port_number} = port_name{1};
    
    inport_block_port_handles = get_param(inport, 'PortHandles');
    inport_block_port_handles = inport_block_port_handles{1};
    inport_block_out_port_handle = inport_block_port_handles.Outport;
    inport_block_out_port_number = get_param(inport_block_out_port_handle, 'PortNumber');
    
    %The simulink port is the inport object and its associated port number.
    %The driver is the Input Master node and the port number drom the
    %inport's dialog
    %The in_system_node is the top level node
    [new_nodes_recur, new_arcs_recur, new_special_nodes_recur] = simulink_to_graphml_arc_follower(inport, inport_block_out_port_number, input_master_node, port_number, top_level_ir_node, output_master_node, unconnected_master_node, terminator_master_node, vis_master_node, node_handle_ir_map);
    
    %Add the nodes and arcs created in the 
    nodes = [nodes, new_nodes_recur];
    arcs = [arcs, new_arcs_recur];
    special_nodes = [special_nodes, new_special_nodes_recur];
end

%% Add Names to Output Master Node 
%Get a list of Inports within system
outports = find_system(system,  'FollowLinks', 'on', 'LoadFullyIfNeeded', 'on', 'LookUnderMasks', 'on', 'SearchDepth', 1, 'BlockType', 'Outport');

%Call the arc follower on each
for i = 1:length(outports)
    outport = outports(i);
    
    %Get the port number of the inport, this will be used to distinguish
    %the different inputs.
    port_number_str = get_param(outport, 'Port'); %Returned as a string
    port_number = str2double(port_number_str);
    
    %Get port name and create entry in Input Master Node
    port_name = get_param(outport, 'Name');
    output_master_node.inputPorts{port_number} = port_name{1}; 
end

if verbose >= 1
    disp('[SimulinkToGraphML] **** Traversing Simulink Graph To Find Sources that are not Inputs ****');
end

%% Traverse Remaining Nodes (ie. ones that would not be reached by just following inputs
[new_nodes_recur, new_arcs_recur, new_special_nodes_recur] = traverseRemainingNodes(system, top_level_ir_node, output_master_node, unconnected_master_node, terminator_master_node, vis_master_node, node_handle_ir_map);
nodes = [nodes, new_nodes_recur];
arcs = [arcs, new_arcs_recur];
special_nodes = [special_nodes, new_special_nodes_recur];

%% Connect Special Input/Output Ports to Enable Drivers

if verbose >= 1
    disp('[SimulinkToGraphML] **** Connecting Enable Ports to Drivers ****');
end

%Interate through the special nodes
for i = 1:length(special_nodes)
    special_node = special_nodes(i);
    
    %Get the enable driver from the parent node
    %Special nodes are direct children of the enabled subsystem node which
    %contains the enable driver information
    special_node_parent = special_node.parent;
    
    en_driver_node = special_node_parent.en_in_src_node;
    en_driver_port = special_node_parent.en_in_src_port;
    
    %Now, make an arc to the special port.  The enable port for all special
    %nodes is 2
    if isempty(en_driver_node) || isempty(en_driver_port)
        error(['Enable Driver for ' special_node.getFullSimulinkPath() ' not found!'])
    end
    
    newArc = GraphArc.createEnableArc(en_driver_node, en_driver_port, special_node, 2);
    
    if exist('verbose', 'var') && verbose >= 3
        disp(['[SimulinkToGraphML] Imported Arc: ' newArc.srcNode.getFullSimulinkPath() ':' num2str(newArc.srcPortNumber) ' -> ' newArc.dstNode.getFullSimulinkPath() ':'  newArc.dstPortTypeStr() ':' num2str(newArc.dstPortNumber)]);
    end
    
    %Add the arc back to the list
    arcs = [arcs, newArc];
    
end

%% Done traversing simulink system - terminate compile mode
%Need to do this after enable arc creation so that datatypes can be
%captured
top_system_func([], [], [], 'term');

%% Expand graph & cleanup busses

if verbose >= 1
    disp('[SimulinkToGraphML] **** Expanding Nodes & Cleaning Up Busses ****');
end

[new_nodes, synth_vector_fans, new_arcs, arcs_to_delete] = ExpandBlocks(nodes, master_nodes, unconnected_master_node);
nodes = [nodes, new_nodes];
nodes = [nodes, synth_vector_fans];

%Some of the arcs to be deleted may be in the new arcs list.  Add arcs
%first, then process deletions
arcs = [arcs, new_arcs];

for i = 1:length(arcs_to_delete)
    arc_to_delete = arcs_to_delete(i);
    arcs(arcs == arc_to_delete) = [];
end

%% Assign Unique Ids To each Node and Arc

if verbose >= 1
    disp('[SimulinkToGraphML] **** Assigning Node and Arc IDs ****');
end

for i = 1:length(nodes)
    node = nodes(i);
    node.nodeId = i + 5; %+3 because of master nodes & top level (which is 0)
end

for i = 1:length(arcs)
    arc = arcs(i);
    arc.arcId = i;
end

%% Get Set of Parameters from Nodes
if verbose >= 1
    disp('[SimulinkToGraphML] **** Collecting Parameter Set ****');
end

node_param_names = {};
for i = 1:length(nodes)
    node = nodes(i);
    
    param_names = keys(node.dialogProperties);
    
    mask_names = keys(node.maskVariables);
    for j = 1:length(mask_names)
        mask_name = mask_names{j};
        next_ind = length(param_names)+1;
        param_names{next_ind} = ['MaskVariable.' mask_name];
    end
    
    param_numeric_names = keys(node.dialogPropertiesNumeric);
    for j = 1:length(param_numeric_names)
        param_numeric_name = param_numeric_names{j};
        next_ind = length(param_names)+1;
        param_names{next_ind} = ['Numeric.' param_numeric_name];
    end
    
    for j = 1:length(node.inputPorts)
        next_ind = length(param_names)+1;
        param_names{next_ind} = ['input_port_name_' num2str(j)];
    end
    
    for j = 1:length(node.outputPorts)
        next_ind = length(param_names)+1;
        param_names{next_ind} = ['output_port_name_' num2str(j)];
    end
    
    node_param_names = union(node_param_names, param_names);
end

%Get Master Node Params (Port Names)
param_names = {};
for i = 1:length(master_nodes)
    node = master_nodes(i);
    
    for j = 1:length(node.inputPorts)
        next_ind = length(param_names)+1;
        param_names{next_ind} = ['input_port_name_' num2str(j)];
    end

    for j = 1:length(node.outputPorts)
        next_ind = length(param_names)+1;
        param_names{next_ind} = ['output_port_name_' num2str(j)];
    end
    
    node_param_names = union(node_param_names, param_names);
end

%% Open GraphML file for writing
if verbose >= 1
    disp('[SimulinkToGraphML] **** Emitting GraphML ****');
end

graphml_filehandle = fopen(graphml_filename,'w');

%% ==== Emit GraphML Preamble =====
% Write the GraphML XML Preamble
fprintf(graphml_filehandle, '<?xml version="1.0" encoding="UTF-8"?>\n');
fprintf(graphml_filehandle, '<graphml xmlns="http://graphml.graphdrawing.org/xmlns" \n');
fprintf(graphml_filehandle, '\txmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" \n');
fprintf(graphml_filehandle, '\txsi:schemaLocation="http://graphml.graphdrawing.org/xmlns \n');
fprintf(graphml_filehandle, '\thttp://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd">\n');

%% +++++ Write GraphML Attribute Definitions +++++
% ---- Static Definitions ----
% Write the GraphML Attribute Definitions
% Instance Name
fprintf(graphml_filehandle, '\t<key id="instance_name" for="node" attr.name="instance_name" attr.type="string">\n');
fprintf(graphml_filehandle, '\t\t<default>""</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

% Instance Name
fprintf(graphml_filehandle, '\t<key id="block_node_type" for="node" attr.name="block_node_type" attr.type="string">\n');
fprintf(graphml_filehandle, '\t\t<default>""</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

% Block Type -> Block Function
fprintf(graphml_filehandle, '\t<key id="block_function" for="node" attr.name="block_function" attr.type="string">\n');
fprintf(graphml_filehandle, '\t\t<default>""</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

% Block Type -> Block Label
fprintf(graphml_filehandle, '\t<key id="block_label" for="node" attr.name="block_label" attr.type="string">\n');
fprintf(graphml_filehandle, '\t\t<default>""</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

% Number of Named Ports
fprintf(graphml_filehandle, '\t<key id="named_input_ports" for="node" attr.name="named_input_ports" attr.type="int">\n');
fprintf(graphml_filehandle, '\t\t<default>0</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

fprintf(graphml_filehandle, '\t<key id="named_output_ports" for="node" attr.name="named_output_ports" attr.type="int">\n');
fprintf(graphml_filehandle, '\t\t<default>0</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

% Arc Src Port (For libraries that do not support ports)
fprintf(graphml_filehandle, '\t<key id="arc_src_port" for="edge" attr.name="arc_src_port" attr.type="int">\n');
fprintf(graphml_filehandle, '\t\t<default>0</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

% Arc Dst Port (For libraries that do not support ports)
fprintf(graphml_filehandle, '\t<key id="arc_dst_port" for="edge" attr.name="arc_dst_port" attr.type="int">\n');
fprintf(graphml_filehandle, '\t\t<default>0</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

% Arc Intermediate Node (for flattened nodes)
fprintf(graphml_filehandle, '\t<key id="arc_intermediate_node" for="edge" attr.name="arc_intermediate_node" attr.type="string">\n');
fprintf(graphml_filehandle, '\t\t<default>""</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

% Arc Intermediate Port (For libraries that do not support ports)
fprintf(graphml_filehandle, '\t<key id="arc_intermediate_port" for="edge" attr.name="arc_intermediate_port" attr.type="int">\n');
fprintf(graphml_filehandle, '\t\t<default>0</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

% Arc Intermediate Port Type (for flattened nodes)
fprintf(graphml_filehandle, '\t<key id="arc_intermediate_port_type" for="edge" attr.name="arc_intermediate_port_type" attr.type="string">\n');
fprintf(graphml_filehandle, '\t\t<default>""</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

% Arc Intermediate Port Direction (for flattened nodes)
fprintf(graphml_filehandle, '\t<key id="arc_intermediate_direction" for="edge" attr.name="arc_intermediate_direction" attr.type="string">\n');
fprintf(graphml_filehandle, '\t\t<default>""</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

% The display label for edges
% Contains properties which are printed for visualization
fprintf(graphml_filehandle, '\t<key id="arc_disp_label" for="edge" attr.name="arc_disp_label" attr.type="string">\n');
fprintf(graphml_filehandle, '\t\t<default>""</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

% Arc Static Properties
%arc_datatype
fprintf(graphml_filehandle, '\t<key id="arc_datatype" for="edge" attr.name="arc_datatype" attr.type="string">\n');
fprintf(graphml_filehandle, '\t\t<default>""</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

%arc_complex
fprintf(graphml_filehandle, '\t<key id="arc_complex" for="edge" attr.name="arc_complex" attr.type="string">\n');
fprintf(graphml_filehandle, '\t\t<default>""</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

%arc_dimension
fprintf(graphml_filehandle, '\t<key id="arc_dimension" for="edge" attr.name="arc_dimension" attr.type="string">\n');
fprintf(graphml_filehandle, '\t\t<default>""</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

%arc_width
fprintf(graphml_filehandle, '\t<key id="arc_width" for="edge" attr.name="arc_width" attr.type="string">\n');
fprintf(graphml_filehandle, '\t\t<default>""</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

%arc_vis_type
fprintf(graphml_filehandle, '\t<key id="arc_vis_type" for="edge" attr.name="arc_vis_type" attr.type="string">\n');
fprintf(graphml_filehandle, '\t\t<default>""</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

%---- Dynamic Defitions (from Dialog Properties) ----
for i = 1:length(node_param_names)
    param_name = node_param_names{i};
    
    %We will assume type is string for now
    fprintf(graphml_filehandle, '\t<key id="%s" for="node" attr.name="%s" attr.type="string">\n', param_name, param_name);
    fprintf(graphml_filehandle, '\t\t<default>""</default>\n');
    fprintf(graphml_filehandle, '\t</key>\n');
end

%% Start Graph Entry
fprintf(graphml_filehandle, '\t<graph id="G" edgedefault="directed">\n'); 

%% Traverse Node Hierarchy and Emit GraphML Node Entries
%The top level is a special case.  We do not create a node entry for the
%top level system as it is covered by the graph entry.

%Instead, we will call the emit function on all of the children of the top
%level

%Emit master nodes
input_master_node.emitSelfAndChildrenGraphml(graphml_filehandle, 2);
output_master_node.emitSelfAndChildrenGraphml(graphml_filehandle, 2);
vis_master_node.emitSelfAndChildrenGraphml(graphml_filehandle, 2);
unconnected_master_node.emitSelfAndChildrenGraphml(graphml_filehandle, 2);
terminator_master_node.emitSelfAndChildrenGraphml(graphml_filehandle, 2);

for i = 1:length(top_level_ir_node.children)
    child = top_level_ir_node.children(i);
    child.emitSelfAndChildrenGraphml(graphml_filehandle, 2);
end

%% Emit GraphML Arc Entries
for i = 1:length(arcs)
    arc = arcs(i);
    arc.emitGraphml(graphml_filehandle, 2);
end

%% Close GraphML File
fprintf(graphml_filehandle, '\t</graph>\n');
fprintf(graphml_filehandle, '</graphml>');

fclose(graphml_filehandle);

%Report Export

if verbose >= 1
    disp('[SimulinkToGraphML] **** Finished GraphML Export ****');
    disp(['[SimulinkToGraphML] Exported ' num2str(length(nodes)+5) ' Nodes']); %+5 for master nodes
    disp(['[SimulinkToGraphML] Exported ' num2str(length(arcs)) ' Arcs']);
    
    disp('[SimulinkToGraphML] Statistics:');
    %Report Some Stats
    countMap = GetNodeStatistics(nodes);
    nodeTypes = sort(keys(countMap));
    for i = 1:length(nodeTypes)
       key = nodeTypes{i};
       count = countMap(key);
       
       disp(sprintf('%s | Count: %6d', key, count));
    end
end

end
