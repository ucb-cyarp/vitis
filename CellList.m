classdef CellList < handle
    %CellList A wrapper arround a cell array that provides stack and queue
    %semantics.  Is a handle class (so it acts like an object in C++/Java
    %with references being passed instead of values).
    
    properties (SetAccess = private)
        list
        count
    end
    
    methods
        function obj = CellList()
            %CellList Construct an empty CellList
            obj.list = {};
            obj.count = 0;
        end
        
        function pushBack(obj,item)
            %pushBack Push onto the stack (top) or queue (tail)
            %   Internally inserts element to end of cell array
            obj.list{obj.count+1} = item;
            obj.count = obj.count+1;
        end
        
        function pushFront(obj,item)
            %pushFront Push onto the front of the internal array
            %   Not commonly used unless pushing a recently dequeued item
            %   back on the queue to be accessed in the next call to
            %   dequeueFront
            obj.list = [{item}, obj.list];
            obj.count = obj.count+1;
        end
        
        function outputArg = popBack(obj)
            %popBack Pop off the top of the stack
            %   Internally pops off the back of the stack
            %   If empty returns empty array and takes no action
                
            if obj.count > 0
                outputArg = obj.list{obj.count}; %Get the last element
                obj.list = obj.list(1:(obj.count-1));
                obj.count = obj.count-1;
            else
                outputArg = [];
            end
            
        end
        
        function outputArg = peakBack(obj)
            %peakBack Peek at the element at the top of the stack
            %   If empty returns empty array
            
            if obj.count > 0
                outputArg = obj.list{obj.count}; %Get the last element
            else
                outputArg = [];
            end
        end
        
        function outputArg = dequeueFront(obj)
            %popBack dequeue off the front of the queue
            %   Internally removes from the front of the array
            %   If empty returns empty array and takes no action
            
            if obj.count > 0
                outputArg = obj.list{1}; %Get the first element
                obj.list = obj.list(2:(obj.count));
                obj.count = obj.count-1;
            else
                outputArg = [];
            end
            
        end
        
        function outputArg = peakFront(obj)
            %peakFront Peek at the element at the front of the queue
            %   If empty returns empty array
            
            if obj.count > 0
                outputArg = obj.list{1}; %Get the last element
            else
                outputArg = [];
            end
            
        end
        
        function outputArg = size(obj)
            %size Get the size of the stack or queue
            outputArg = obj.count;
        end
        
        function outputArg = cellArray(obj)
            %cellArray Get a copy of the underlying cell array
            outputArg = obj.list;
        end
        
        
    end
end

