function [expansion_occured, expanded_nodes, vector_fans, new_arcs] = ExpandDataTypeDuplicate(dataTypeDuplicateBlock)
%ExpandDelay Expands a ExpandDataTypeDuplicate whose input/output are vector types.  The
%ExpandDataTypeDuplicate is expanded into single delay blocks for each element of the vector.
% If the input/output is a scalar, no action is taken.

expansion_occured = false;
expanded_nodes = [];
vector_fans = [];
new_arcs = [];

%% Checking for Bad Input
%This method should only be called on constant objects, let's check for
%that first
if ~strcmp(dataTypeDuplicateBlock.simulinkBlockType, 'DataTypeDuplicate')
    error('ExpandDataTypeDuplicate was called on a block that is not a DataTypeDuplicate.');
end

in_arc1 = dataTypeDuplicateBlock.in_arcs(1);
%Check for matrix ports, we don't support them at the moment
if length(in_arc1.dimension) >2 || (in_arc1.dimension(1) > 1 && in_arc1.dimension(2) > 1)
    error(['DataTypeDuplicate block expansion does not currently support N-dimensional matrix port types. ' dataTypeDuplicateBlock.getFullSimulinkPath()]);
end

if in_arc1.width <= 1
    %TODO: Remove this check
    %Sanity check for disagreement with dimension parameter and width
    %parameter
    if in_arc1.dimension(1) > 1 || in_arc1.dimension(2) > 1
        error('Disagreement between dimension and width parameters for arc durring Delay block expansion');
    end
    
    %The output arc is not a vector (and transitivitily the input arc is not a vector).  No work to do, return
    return;
end

%Check that the input port dimensions are identical
numInPorts = length(dataTypeDuplicateBlock.in_arcs);
for i = 2:1:numInPorts
    in_arc_iter = dataTypeDuplicateBlock.in_arcs(i);
    
    %Check for matrix n-d matrix
    if length(in_arc_iter.dimension) >2 || (in_arc_iter.dimension(1) > 1 && in_arc_iter.dimension(2) > 1)
        error(['DataTypeDuplicate block expansion does not currently support N-dimensional matrix port types. ' dataTypeDuplicateBlock.getFullSimulinkPath()]);
    end
    
    if in_arc_iter.width <= 1
        %TODO: Remove this check
        %Sanity check for disagreement with dimension parameter and width
        %parameter
        if in_arc_iter.dimension(1) > 1 || in_arc_iter.dimension(2) > 1
            error('Disagreement between dimension and width parameters for arc durring Delay block expansion');
        end
    end
    
    %Check the width is equal
    if in_arc_iter.width ~= in_arc1.width
        error(['DataTypeDuplicate has input ports of different widths. ' dataTypeDuplicateBlock.getFullSimulinkPath()]);
    end
end

%Sanity Check and check for if no expansion is needed
if in_arc1.width <= 1
    %TODO: Remove this check
    %Sanity check for disagreement with dimension parameter and width
    %parameter
    if in_arc1.dimension(1) > 1 || in_arc1.dimension(2) > 1
        error('Disagreement between dimension and width parameters for arc durring Delay block expansion');
    end
    
    %The output arc is not a vector (and transitivitily the input arc is not a vector).  No work to do, return
    return;
end

busWidth = in_arc1.width;

%% Get number of ports value
numInputPortsParam = dataTypeDuplicateBlock.dialogProperties('NumInputPorts');

%% Apply Expansion
%Create new DataTypeDuplicate block.  Copy Certain parameters from origional.
%All of the inputs should have the same type, so we can just create a
%single DataTypeDuplicateBlock and expand the number of inputs

node = GraphNode.createExpandNodeNoSimulinkParams(dataTypeDuplicateBlock, 'Standard', 'DataTypeDuplicate', i); %DataTypeDuplicate are 'Standard' nodes.  This function adds the node as a child of the parent.
%Set Params
node.simulinkBlockType = dataTypeDuplicateBlock.simulinkBlockType; %'DataTypeDuplicate', copy from orig
node.dialogPropertiesNumeric('NumInputPorts') = ['(' numInputPortsParam ')*' num2str(busWidth)];

%Add new node to array
expanded_nodes = [expanded_nodes, node];

arcsToRemoveFromOrig = [];

for i = 1:numInPorts
    %Create Vector_Fan object for each input port
    vector_fan_input = VectorFan('Fan-Out', dataTypeDuplicateBlock); %Indevidual Arcs will be fanning out from the bus
    vector_fans = [vector_fans, vector_fan_input];
    
    %Attach the old in_arc (should only be one) to the VectorFan input and
    %remove it from the orig object
    in_arc = dataTypeDuplicateBlock.in_arcs(i);
    in_arc.dstNode = vector_fan_input;
    vector_fan_input.addBusArc(in_arc);
    arcsToRemoveFromOrig = [arcsToRemoveFromOrig, in_arc];
    
    %---- Create new arcs for input ----
    %Do this for each wire
    for j = 1:busWidth
        in_wire_arc = copy(in_arc);

        %Connect the new arc to new block (add as out_arc)
        in_wire_arc.dstNode = node;
        in_wire_arc.dstPortNumber = (i-1)*busWidth+j;
        in_wire_arc.width = 1;
        in_wire_arc.dimension = [1, 1];
        node.addIn_arc(in_wire_arc);

        %Fill in intermediate node entries
        %The intermediate node is the origional Constant
        in_wire_arc.appendIntermediateNodeEntry(dataTypeDuplicateBlock, i, j, 'Standard', 'In');

        %Add arc to VectorFan Input
        vector_fan_input.addArc(in_wire_arc, j);

        %Add new arc
        new_arcs = [new_arcs, in_wire_arc];
    end
    
end

for i = 1:numInPorts
    arcToRemove = arcsToRemoveFromOrig(i);
    dataTypeDuplicateBlock.removeIn_arc(arcToRemove);
end

%Set orig node type to "Expanded" 
dataTypeDuplicateBlock.setNodeTypeFromText('Expanded');
expansion_occured = true;

end

