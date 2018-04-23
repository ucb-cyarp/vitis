classdef GraphArc < handle
    %GraphArc Represents a Graph Arc extracted from Simulink
    %   Is part of the intermediate representation for the data flow graph
    
    properties
        srcNode %GraphNode handle
        srcPortNumber %output port number of the src
        
        dstNode
        dstPortNumber
        dstPortType %One of the following
                     % 0 = Standard
                     % 1 = Enable (the enable signal)
                     % 2 = Reset (the reset signal) -- TODO:
                     % Implement this.  Only handle enable for now
        
        simulinkSrcPortHandle
        simulinkDestPortHandle
        
        %Simulink signal properties
        datatype
        complex
        dimension
        width
        
        %Properties for connection to visualizer
        vis_type
        
        %ArcId
        arcId
        
    end
    
    methods
        function obj = GraphArc(srcNode, srcPortNumber, dstNode, dstPortNumber, dstPortType)
            %GraphArc Construct an instance of this class
            %   dstPortType can be one of the following: 'Standard',
            %   'Enable'
            
            %Copy Parameters
            obj.srcNode = srcNode;
            obj.srcPortNumber = srcPortNumber;
            obj.dstNode = dstNode;
            obj.dstPortNumber = dstPortNumber;
            
            %Set Type
            if strcmp(dstPortType, 'Standard')
                obj.dstPortType = 0;
            elseif strcmp(dstPortType, 'Enable')
                obj.dstPortType = 1;
            else
                obj.dstPortType = 6;
                error(['''', dstPortType, ''' is not a recognized port type']);
            end
            
            %Set Defaults
            obj.simulinkSrcPortHandle = 0;
            obj.simulinkDestPortHandle = 0;
        
            obj.datatype = "";
            obj.complex = false;
            obj.dimension = [1, 0];
            obj.width = 1;
            obj.vis_type = "";
        end
        
        function emitGraphml(obj, file, numTabs)
           %emitGraphml Writes GraphML entries for this arc.
           %numTabs specifies the initial indent (in hardtabs)
           
           sourceIdPath = obj.srcNode.getFullIDPath('::', 'n%d', false);
           dstIdPath = obj.dstNode.getFullIDPath('::', 'n%d', false);
           
           %Emit Arc entry
           writeNTabs(file, numTabs);
           fprintf(file, '<edge id="e%d" source="%s" target="%s">\n', obj.arcId, sourceIdPath, dstIdPath);
           
           %Emit attributes
           writeNTabs(file, numTabs+1);
           fprintf(file, '<data key="arc_src_port">%d</data>\n', obj.srcPortNumber);
           writeNTabs(file, numTabs+1);
           fprintf(file, '<data key="arc_dst_port">%d</data>\n', obj.dstPortNumber);
           
           disp_label = sprintf('Src Port Num: %d, Dst Port Num: %d, Dst Port Type: %s', obj.srcPortNumber, obj.dstPortNumber, obj.dstPortTypeStr());
           writeNTabs(file, numTabs+1);
           fprintf(file, '<data key="arc_disp_label">%s</data>\n', disp_label);
           
           %Close arc entry
           writeNTabs(file, numTabs);
           fprintf(file, '</edge>\n');
            
        end
        
        function str = dstPortTypeStr(obj)
            %dstPortTypeStr Returns the string description of the dst port
            %type
            str = 'Unknown';
            
            if obj.dstPortType == 0
                str = 'Standard';
            elseif obj.dstPortType == 1
                str = 'Enable';
            elseif obj.dstPortType == 2
                str = 'Reset';
            end
                
        end
            
    end
    
    methods (Static)
        function newArc = createBasicArc(src_ir_node, src_ir_port_number, dst_ir_node, dst_ir_port_number, dstPortType)
        %createBasicArc Create a GraphArc object between the src and dst node.  Adds
        %this arc to the src node's out_arcs and to the dst node's in_arcs lists.
        %The ir_node arguments need to be handles.

        newArc = GraphArc(src_ir_node, src_ir_port_number, dst_ir_node, dst_ir_port_number, dstPortType);

        src_ir_node.addOut_arc(newArc);
        dst_ir_node.addIn_arc(newArc);

        %returns newArc

        end
    end
end

