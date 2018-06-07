classdef VectorFan < handle
    %VectorFan A node which represents the fan-in/fan-out of a vector/bus
    %   This is used durring block expansion and contains an array of arc
    %   objects which represent indevidual wires of the bus.  The bus arc
    %   is connected to the VectorFan object durring the 
    
    properties
        arcList
        wireIndexList
    end
    
    methods
        function obj = VectorFan()
            %VectorFan Construct an instance of the VectorFan class
            %   Starts with an empty list of arcs
            obj.arcList = [];
            obj.wireIndexList = [];
        end
        
        function addArc(obj, arc, wireIndex)
            %addArc Adds a fan-in/fan-out arc to the VectorFan object
            obj.arcList = [obj.arcList, arc];
            obj.wireIndexList = [obj.wireIndexList, wireIndex];
        end
    end
end

