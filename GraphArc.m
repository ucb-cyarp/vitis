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
        end
        
        function outputArg = method1(obj,inputArg)
            %METHOD1 Summary of this method goes here
            %   Detailed explanation goes here
            outputArg = obj.Property1 + inputArg;
        end
    end
end

