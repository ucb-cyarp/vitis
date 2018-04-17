function simulink_to_graphml(system, graphml_filename, verbose)
%simulink_to_graphml Converts a simulink system to a GraphML file.
%   Detailed explanation goes here

%% Init
graphml_filehandle = fopen(graphml_filename,'w');

%% Write Preamble
% Write the GraphML XML Preamble
fprintf(graphml_filehandle, '<?xml version="1.0" encoding="UTF-8"?>\n');
fprintf(graphml_filehandle, '<graphml xmlns="http://graphml.graphdrawing.org/xmlns" \n');
fprintf(graphml_filehandle, '\txmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" \n');
fprintf(graphml_filehandle, '\txsi:schemaLocation="http://graphml.graphdrawing.org/xmlns \n');
fprintf(graphml_filehandle, '\thttp://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd">\n');

%% Write Attribute Definitions
% Write the GraphML Attribute Definitions
% Instance Name
fprintf(graphml_filehandle, '\t<key id="instance_name" for="node" attr.name="instance_name" attr.type="string">\n');
fprintf(graphml_filehandle, '\t\t<default>""</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

% Arc Weight
% fprintf(graphml_filehandle, '\t<key id="weight" for="edge" attr.name="weight" attr.type="double"/>\n');
% fprintf(graphml_filehandle, '\t\t<default>0.0</default>\n');
% fprintf(graphml_filehandle, '\t</key>\n');

% Arc Src Port (For libraries that do not support ports)
fprintf(graphml_filehandle, '\t<key id="arc_src_port" for="edge" attr.name="arc_src_port" attr.type="string"/>\n');
fprintf(graphml_filehandle, '\t\t<default>""</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

% Arc Dst Port (For libraries that do not support ports)
fprintf(graphml_filehandle, '\t<key id="arc_dst_port" for="edge" attr.name="arc_dst_port" attr.type="string"/>\n');
fprintf(graphml_filehandle, '\t\t<default>""</default>\n');
fprintf(graphml_filehandle, '\t</key>\n');

fprintf(graphml_filehandle, '\t<graph id="G" edgedefault="directed">\n');

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

%% Create Virtual Nodes

%% Call Helper on Each Input

%% Close GraphML File
fprintf(graphml_filehandle, '\t</graph>\n');
fprintf(graphml_filehandle, '</graphml>');

fclose(graphml_filehandle);

end

