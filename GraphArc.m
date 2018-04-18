classdef GraphArc
    %GraphArc Represents a Graph Arc extracted from Simulink
    %   Is part of the intermediate representation for the data flow graph
    
    properties
        srcNode %GraphNode handle
        srcPortNumber %output port number of the src
        
        destNode
        destPortNumber
        destPortType %One of the following
                     % 0 = Standard Input
                     % 1 = Enable Input (the enable signal)
                     % 2 = Reset Input (the reset signal) -- TODO:
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
        function obj = untitled2(inputArg1,inputArg2)
            %UNTITLED2 Construct an instance of this class
            %   Detailed explanation goes here
            obj.Property1 = inputArg1 + inputArg2;
        end
        
        function outputArg = method1(obj,inputArg)
            %METHOD1 Summary of this method goes here
            %   Detailed explanation goes here
            outputArg = obj.Property1 + inputArg;
        end
    end
end

