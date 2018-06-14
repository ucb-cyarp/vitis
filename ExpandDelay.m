function [expansion_occured, expanded_nodes, vector_fan_output, new_arcs] = ExpandDelay(delayBlock)
%ExpandDelay Expands a delay whose input/output are vector types.  The
%delay is expanded into single delay blocks for each element of the vector.
% If the input/output is a scalar, no action is taken.
%
% This is different from ExpandTappedDelay since, in that case, a series on inputs
% are delayed and the outputs of the delays are concatenated to form a
% vector.

% NOTE: The Intial condition can be a scalar or a vector/matrix.  If the
% initial condition is a vector/matrix, the dimension must match that of
% the input.  If the initial condition is a scalar, the value is used for
% each component of the vector.

% Note: The delay value can only be a scalar.

% Note: There can actually be 3 input ports

% TODO: For now, will only support initial condition and delay values from
% dialogs.  The semantics of changing delays is complicated: is there
% infinite memory and expanding the delay will result in values that would
% have been throwned away reappearing.  Initial conditions are more defined
% but are not used in the designs we are currently looking at.  This also
% related to the unimplemented reset feature.

expansion_occured = false;
expanded_nodes = [];
vector_fan_output = [];
new_arcs = [];

%% Checking for Bad Input
%This method should only be called on constant objects, let's check for
%that first
if ~strcmp(delayBlock.simulinkBlockType, 'Delay')
    error('ExpandDelay was called on a block that is not a delay.');
end

%There may be many output arcs (connected to different blocks) but should
%all have the same properties
out_arc = delayBlock.out_arcs(1);

%Check for matrix ports, we don't support them at the moment
if length(out_arc.dimension) >2 || (out_arc.dimension(1) > 1 && out_arc.dimension(2) > 1)
    error('Delay block expansion does not currently support N-dimensional matrix port types.');
end

%Check for unsupported modes
if ~strcmp(delayBlock.dialogProperties('DelayLengthSource'), 'Dialog')
    error('Delay length blocks with delay length not specified in the dialog are currently not supported.');
end

if ~strcmp(delayBlock.dialogProperties('InitialConditionSource'), 'Dialog')
    error('Delay length blocks with initial conditions not specified in the dialog are currently not supported.');
end
    
%Check for other inputs as a sign that delay or initial condition is not
%defined via dialog
if length(delayBlock.in_arcs) > 1
    error('Only delay blocks where delay values and initial conditions are specified in Dialog are currently supported.');
end

%There should only be 1 in port
in_arc = delayBlock.in_arcs(1);

if in_arc.width ~= out_arc.width
    error('Width disagrement between input and output arcs in Delay block.');
end

if out_arc.width <= 1
    %TODO: Remove this check
    %Sanity check for disagreement with dimension parameter and width
    %parameter
    if out_arc.dimension(1) > 1 || out_arc.dimension(2) > 1
        error('Disagreement between dimension and width parameters for arc durring Delay block expansion');
    end
    
    %The output arc is not a vector (and transitivitily the input arc is not a vector).  No work to do, return
    return;
end

%% Find Expansion Init Values
%OK, now ready to start expanding
expansion_occured = true;

%Check if the value is a scalar or vector.
orig_init_value = delayBlock.dialogPropertiesNumeric('InitialCondition');

if length(orig_init_value) > 1 
    %The value is a matric
    
    %Check that the dimensions match the port
    if length(orig_init_value) ~= out_arc.width
        error('Dimension mismatch between Delay block value and port width.');
    end
    
    init_array = orig_init_value;
else
    %The value is a scalar.
    
    %The array is just the scalar replicated for each array entry
    init_array = orig_init_value .* ones(1, out_arc.width);
end

%% Get delay value
delay_value = delayBlock.dialogPropertiesNumeric('DelayLength');

%% Apply Expansion

%value_array constains what each constant should be

%Create Vector_Fan object
vector_fan_output = VectorFan('Fan-In', delayBlock); %Indevidual Arcs will be fanning into the bus
vector_fan_input = VectorFan('Fan-Out', delayBlock); %Indevidual Arcs will be fanning out from the bus

for i = 1:length(init_array)
    %Create new delay block.  Copy Certain parameters from origional.
    node = GraphNode.createExpandNodeNoSimulinkParams(delayBlock, 'Standard', 'Delay'); %Delays are 'Standard' nodes.  This function adds the node as a child of the parent.
    
    %Set Params
    node.simulinkBlockType = delayBlock.simulinkBlockType; %'Delay', copy from orig
    
    %Currently, only handling numerics
    node.dialogPropertiesNumeric('InitialCondition') = init_array(i);
    node.dialogPropertiesNumeric('DelayLength') = delay_value; %all delay values
    node.dialogPropertiesNumeric('SampleTime') = delayBlock.dialogPropertiesNumeric('SampleTime'); % Copy from orig
    node.dialogProperties('DelayLengthSource') = delayBlock.dialogProperties('DelayLengthSource');
    node.dialogProperties('InitialConditionSource') = delayBlock.dialogProperties('InitialConditionSource');
    
    %---- Create new arc for output ----
    %The arc will be a shallow copy of the existing output arc
    %This should copy the arrays (because they are values) and will copy
    %the handles to handle objects but will not clone the handle objects
    %themselves
    out_wire_arc = copy(out_arc);
    
    %Connect the new arc to new block (add as out_arc)
    out_wire_arc.srcNode = node;
    out_wire_arc.srcPortNumber = 1; %delay outputs use port 1
    out_wire_arc.width = 1;
    out_wire_arc.dimension = [1, 1];
    node.addOut_arc(out_wire_arc);
    %DO NOT add as in_arc since this will be done in bus cleanup
    
    %Fill in intermediate node entries
    %The intermediate node is the origional Constant
    out_wire_arc.appendIntermediateNodeEntry(delayBlock, 1, i, 'Standard', 'Out'); %Since we are expanding a const, the output port of the origional block would be 'standard' and the direction is 'out'
    
    %Add arc to VectorFan Output
    vector_fan_output.addArc(out_wire_arc, i);
    
    %Add new arc
    new_arcs = [new_arcs, out_wire_arc];
    
    %---- Create new arc for input ----
    in_wire_arc = copy(in_arc);
    
    %Connect the new arc to new block (add as out_arc)
    in_wire_arc.dstNode = node;
    in_wire_arc.dstPortNumber = 1; %delay inputs use port 1
    in_wire_arc.width = 1;
    in_wire_arc.dimension = [1, 1];
    node.addIn_arc(in_wire_arc);
    %DO NOT add as out_arc since this will be done in bus cleanup
    
    %Fill in intermediate node entries
    %The intermediate node is the origional Constant
    in_wire_arc.appendIntermediateNodeEntry(delayBlock, 1, i, 'Standard', 'In'); %Since we are expanding a const, the output port of the origional block would be 'standard' and the direction is 'out'
    
    %Add arc to VectorFan Input
    vector_fan_input.addArc(in_wire_arc, i);
    
    %Add new arc
    new_arcs = [new_arcs, in_wire_arc];
    
    %Add new node to array
    expanded_nodes = [expanded_nodes, node];
    
end

%Attach the old out arcs (maybe multiple) to the VectorFan output and remove it from the out_arcs list
%of the orig object

arcs_to_remove_from_delayBlock = [];

for i = 1:length(delayBlock.out_arcs)
    out_arc = delayBlock.out_arcs(i);
    arcs_to_remove_from_delayBlock = [arcs_to_remove_from_delayBlock, out_arc];
    
    out_arc.srcNode = vector_fan_output;
    vector_fan_output.addBusArc(out_arc);
end

for i = 1:length(arcs_to_remove_from_delayBlock)
    delayBlock.removeOut_arc(arcs_to_remove_from_delayBlock(i));
end

%Attach the old in_arc (should only be one) to the VectorFan input and
%remove it from the orig object
in_arc = delayBlock.in_arcs(1);
in_arc.dstNode = vector_fan_input;
vector_fan_input.addBusArc(in_arc);
delayBlock.removeIn_arc(in_arc);

%Set orig node type to "Expanded" 
delayBlock.setNodeTypeFromText('Expanded');

end

