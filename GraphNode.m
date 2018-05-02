classdef GraphNode < handle
    %GraphNode Represents a Graph Node extracted from Simulink
    %   Is part of the intermediate representation for the data flow graph
    
    properties
        name %The name of the block (from simulink)
        
        nodeType %Type of block.  Can be 1 of the following
                 % 0 = Standard
                 % 1 = Subsystem
                 % 2 = Enabled Subsystem
                 % 3 = Special Input Port
                 % 4 = Special Output Port
                 % 5 = Top Level
                 % 6 = Master
        
        simulinkBlockType %Simulink blocktype
        simulinkHandle %Simulink node handle
        dialogProperties %A map of dialog Properties extracted from simulink
        
        parent %A reference to the parent node object (in the hierarchy)
               %Ie, a reference to the subsystem the node is directly
               %within
               % = [] if top level
        
        children %A cell array of children of this node (only valid for subsystems)
        
        out_arcs %A cell array of outward arcs (handles to GraphArc objects)
        in_arcs %A cell array of input arcs
        
        flattened % flag to indicate that the node has been flattened
        
        en_in_src_node %A reference to the node driving the enable line of the susbsytem if the node type is "Enabled Subsystem" or the gating line if the node is a "Special Port"
        en_in_src_port %A reference to the port of the node driving the enable line of the susbsytem if the node type is "Enabled Subsystem" or the gating line if the node is a "Special Port"
                       %Will not be populated for "Special Ports" until the
                       %graph has been completely traversed.  Populated in a
                       %later stage
        
        inputPorts %A map of input port numbers to names ("" if name not given)
                   %For special Input/Output ports the enable signal line
                   %is the 2nd input port.  The data goes to port 1
        outputPorts %A map of output port numbers to names ("" if name not given)
                    %For special Input/Output there is only 1 port for the
                    %output
        
        sampleTime %The sample time for the block (from simulink)
        
        nodeId %A generic node ID.  Will be an integer rather than the simulink handle doubles
    end
    
    methods
        function obj = GraphNode(name, type, parent)
            %GraphNode Construct an instance of this class
            %   Type is 'Standard', 'Subsystem', 'Enabled Subsystem', 'Special
            %   Input Port', 'Special Output Port', 'Top Level'
            
            %Copy arguments
            obj.name = name;
            obj.parent = parent;
            
            %Set Type
            if strcmp(type, 'Standard')
                obj.nodeType = 0;
            elseif strcmp(type, 'Subsystem')
                obj.nodeType = 1;
            elseif strcmp(type, 'Enabled Subsystem')
                obj.nodeType = 2;
            elseif strcmp(type, 'Special Input Port')
                obj.nodeType = 3;
            elseif strcmp(type, 'Special Output Port')
                obj.nodeType = 4;
            elseif strcmp(type, 'Top Level')
                obj.nodeType = 5;
            elseif strcmp(type, 'Master')
                obj.nodeType = 6;
            else
                obj.nodeType = 7;
                error(['''', type, ''' is not a recognized node type']);
            end
            
            %Set Defaults
            obj.simulinkBlockType = "";
            obj.simulinkHandle = 0;
            obj.dialogProperties = containers.Map('KeyType','char','ValueType','any');
            obj.children = [];
            obj.out_arcs = [];
            obj.in_arcs = [];
            obj.en_in_src_node = [];
            obj.en_in_src_port = [];
            
            obj.inputPorts = containers.Map();
            obj.outputPorts = containers.Map();
        
            obj.nodeId  = 0; 
            obj.flattened = false;
        end
        
        function addChild(obj, child)
            %addChild Add a child node to the list of children
            obj.children = [obj.children, child];
        end
        
        function outputArg = getAncestorHierarchy(obj)
            %getAncestorHierarchy Returns the hierarchy containing the
            %given node
            %   The first element is the top level node and the last is the
            %   given node
            outputArg = [];
            
            done = false;
            cursor = obj;
            while ~done
                outputArg = [cursor, outputArg];
                
                if ~isempty(cursor.parent)
                    cursor = cursor.parent;
                else
                    done = true;
                end
            end
        end
        
        function outputArg = getFullSimulinkPath(obj)
            %getFullSimulinkPath Get the full simulink path string of the
            %node
            hierarchy = obj.getAncestorHierarchy();
            
            %Assume there is at least 1 node in the hierarchy
            outputArg = hierarchy(1).name;
            for i = 2:length(hierarchy)
                outputArg = [outputArg, '/', hierarchy(i).name];
            end
        end
        
        function outputArg = getFullIDPath(obj, delim, formatSpec, includeTop)
            %METHOD1 Summary of this method goes here
            %   Detailed explanation goes here
            
            hierarchy = obj.getAncestorHierarchy();
            
            if includeTop
                %Assume there is at least 1 node in the hierarchy
                outputArg = sprintf(formatSpec, hierarchy(1).nodeId);
                start = 2;
            else
                %Check if there is at least 2 nodes
                if length(hierarchy) > 1
                    outputArg = sprintf(formatSpec, hierarchy(2).nodeId);
                    start = 3;
                else
                    outputArg = '';
                    start = length(hierarchy) + 1; %Prevent the loop from running
                end
            end
            
            for i = start:length(hierarchy)
                outputArg = [outputArg, delim, sprintf(formatSpec, hierarchy(i).nodeId)];
            end
            
        end
        
        function addOut_arc(obj, newArc)
            %addOut_arc Add an arc to the out_arcs list;
            obj.out_arcs = [obj.out_arcs, newArc];
        end
            
        function addIn_arc(obj, newArc)
            %addIn_arc Add an arc to the in_arcs list;
            obj.in_arcs = [obj.in_arcs, newArc];
        end
        
        function topLvl = isTopLevelSystem(obj)
            %isTopLevelSystem Returns true if node is a top level system
            topLvl = obj.nodeType == 5;
        end
        
        function enbld = isEnabledSubsystem(obj)
            %isEnabledSubsystem Returns true if the node is an enabled
            %subsystem
            enbld = obj.nodeType == 2;
        end
        
        function subsys = isSubsystem(obj)
            %isSystem Returns true if the node is a subsystem
            subsys = obj.nodeType == 1 || obj.nodeType == 2;
        end
        
        function master = isMaster(obj)
            %isMaster Returns true if the node is a master node
            master = obj.nodeType == 6;
        end
        
        function special = isSpecial(obj)
            %isMaster Returns true if the node is a special Input/Output node
            special = obj.nodeType == 3 || obj.nodeType == 4;
        end
        
        function special = isSpecialInput(obj)
            %isSpecialInput Returns true if the node is a special Input node
            special = obj.nodeType == 3;
        end
        
        function special = isSpecialOutput(obj)
            %isSpecialOutput Returns true if the node is a special Output node
            special = obj.nodeType == 4;
        end
        
        function emitDialogParameters(obj, file, numTabs)
        %emitDialogParameters Emit dialog parameters to file
            param_names = keys(obj.dialogProperties);
            for i = 1:length(param_names)
                param_name = param_names{i};
                
                param_val = obj.dialogProperties(param_name);
                
                %Convert to string if needed
                param_val_str = anyToString(param_val);
                
                writeNTabs(file, numTabs);
                fprintf(file, '<data key="%s">%s</data>\n', param_name, param_val_str);
                
            end
        end
        
        function emitSelfAndChildrenGraphml(obj, file, numTabs)
           %emitSelfAndChildrenGraphml Writes GraphML entries for this
           %node and its children.  numTabs specifies the initial indent
           %(in hardtabs)
           if obj.isSubsystem()
               %Emit the entry for this node
               nodeIdPath = obj.getFullIDPath('::', 'n%d', false);
               writeNTabs(file, numTabs);
               fprintf(file, '<node id=\"%s\">\n', nodeIdPath);
               writeNTabs(file, numTabs+1);
               fprintf(file, '<data key="instance_name">%s</data>\n', obj.name);
               % Block Type -> Block Function
               writeNTabs(file, numTabs+1);
               fprintf(file, '<data key="block_function">%s</data>\n', obj.simulinkBlockType);
               % Emit Dialog Properties
               obj.emitDialogParameters(file, numTabs+1);
               
               %Create the nested graph entry for the subsystem
               writeNTabs(file, numTabs+1);
               fprintf(file, '<graph id="%s:" edgedefault="directed">\n', nodeIdPath);
               
               %Emit children
               for i = 1:length(obj.children)
                   child = obj.children(i);
                   child.emitSelfAndChildrenGraphml(file, numTabs+2);
               end
               
               %Close Graph entry
               writeNTabs(file, numTabs+1);
               fprintf(file, '</graph>\n');
               
               %Close node entry
               writeNTabs(file, numTabs);
               fprintf(file, '</node>\n');
               
           else
               if ~isempty(obj.children)
                   error('Node is not a subsystem but has children')
               end
               
               %Emit graphml for a node which is not a subsystem
               nodeIdPath = obj.getFullIDPath('::', 'n%d', false);
               writeNTabs(file, numTabs);
               fprintf(file, '<node id="%s">\n', nodeIdPath);
               writeNTabs(file, numTabs+1);
               fprintf(file, '<data key="instance_name">%s</data>\n', obj.name);
               % Block Type -> Block Function
               writeNTabs(file, numTabs+1);
               fprintf(file, '<data key="block_function">%s</data>\n', obj.simulinkBlockType);
               % Emit Dialog Properties
               obj.emitDialogParameters(file, numTabs+1);
               
               writeNTabs(file, numTabs);
               fprintf(file, '</node>\n');
           end
        end
        
        function port_handle = getSpecialNodeSimulinkPortHandle(obj)
            %getSpecialNodeSimulinkPortHandle Gets the Simulink port handle
            %of the port block in the simulink graph associated with this
            %special port.
            
            src_port_handle = get_param(obj.simulinkHandle, 'PortHandles');
            
            if obj.isSpecialInput()
                port_handle = src_port_handle.Outport;
            elseif obj.isSpecialOutput()
                port_handle = src_port_handle.Inport;
            else
                %This is not a special node, throw an error
                error('getSpecialNodeSimulinkPortHandle called on a non-special node');
            end
            
            %TODO: Handle cell array case
            if iscell(port_handle)
                port_handle = port_handle{1};
            end
            
        end
        
    end
    
    methods(Static)
        function [node, node_created] = createNodeIfNotAlready(simulink_block_handle, node_type, node_handle_map, hierarchy_parent_node)
            %createNodeIfNotAlready Create a new GraphNode object if one
            %does not already exist in the map.  Return either the new node
            %or the existing node in the map.
            
            %Hierarchy parent node should be the handle to another
            %GraphNode
            
            if isKey(node_handle_map, simulink_block_handle)
                %node already exists, return it
                node = node_handle_map(simulink_block_handle);
                node_created = false;
            else
                %node does not exist yet, create it and add it to the map
                node_name = get_param(simulink_block_handle, 'Name');
                
                node = GraphNode(node_name, node_type, hierarchy_parent_node);
                hierarchy_parent_node.addChild(node);
                
                node_handle_map(simulink_block_handle) = node; %Add to map
                
                %Set Block Properties
                node.simulinkHandle = simulink_block_handle;
                node.simulinkBlockType = get_param(simulink_block_handle, 'BlockType');
                
                %---Get parameters from simulink---
                %Get the list of parameter names
                dialog_params = get_param(simulink_block_handle, 'DialogParameters');
                dialog_param_names = fieldnames(dialog_params);
                
                %Itterate through the dialog parameter names
                for i = 1:length(dialog_param_names)
                    dialog_param_name = dialog_param_names{i};
                    
                    dialog_param_value = get_param(simulink_block_handle, dialog_param_name);
                    
                    node.dialogProperties(dialog_param_name) = dialog_param_value;
                end
                
                node_created = true;
            end
            
            %return the node
            
        end
        
        function node = createSpecialInput(simulink_inport_block, node_handle_map, hierarchy_parent_node)
            %createSpecialInput Create a new GraphNode object for a special
            %input node.  Sets the appropriate parameters in the new node.
            %Does not 
            
            node_name = get_param(simulink_inport_block, 'Name');

            node = GraphNode(node_name, 'Special Input Port', hierarchy_parent_node);
            hierarchy_parent_node.addChild(node);

            node_handle_map(simulink_inport_block) = node; %Add to map
            
            node.simulinkHandle = simulink_inport_block; %Set the Simulink Handel to be the inport block
            node.simulinkBlockType = get_param(simulink_inport_block, 'BlockType'); %The type should be inport
            
            %---Get parameters from simulink---
            %Get the list of parameter names
            dialog_params = get_param(simulink_inport_block, 'DialogParameters');
            dialog_param_names = fieldnames(dialog_params);

            %Itterate through the dialog parameter names
            for i = 1:length(dialog_param_names)
                dialog_param_name = dialog_param_names{i};

                dialog_param_value = get_param(simulink_inport_block, dialog_param_name);

                node.dialogProperties(dialog_param_name) = dialog_param_value;
            end

            node_created = true;
            
            %return the node
            
        end
        
        function node = createSpecialOutput(simulink_outport_block, node_handle_map, hierarchy_parent_node)
            %createSpecialInput Create a new GraphNode object for a special
            %input node.  Sets the appropriate parameters in the new node.
            %Does not 
            
            node_name = get_param(simulink_outport_block, 'Name');

            node = GraphNode(node_name, 'Special Output Port', hierarchy_parent_node);
            hierarchy_parent_node.addChild(node);

            node_handle_map(simulink_outport_block) = node; %Add to map
            
            node.simulinkHandle = simulink_outport_block; %Set the Simulink Handel to be the outport block
            node.simulinkBlockType = get_param(simulink_outport_block, 'BlockType'); %The type should be outport
            
            %TODO: IMPLEMENT, GET Parameters from simulink
            
            %---Get parameters from simulink---
            %Get the list of parameter names
            dialog_params = get_param(simulink_outport_block, 'DialogParameters');
            dialog_param_names = fieldnames(dialog_params);

            %Itterate through the dialog parameter names
            for i = 1:length(dialog_param_names)
                dialog_param_name = dialog_param_names{i};

                dialog_param_value = get_param(simulink_outport_block, dialog_param_name);

                node.dialogProperties(dialog_param_name) = dialog_param_value;
            end

            node_created = true;
            
            %return the node
            
        end
        
    end
end

