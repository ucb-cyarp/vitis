function [expansion_occured, expanded_nodes, vector_fan_output, new_arcs] = ExpandConst(constBlock)
%ExpandConst Expands a vector constant into seperate constants.
%The existing node is converted into a Expanded(Subsystem).  A VectorFan
%node is created and arcs from each of the constants are included in it's
%array.

expansion_occured = false;
expanded_nodes= [];
vector_fan_output = [];
new_arcs = [];

%% Checking for Bad Input
%This method should only be called on constant objects, let's check for
%that first
if ~strcmp(constBlock.simulinkBlockType, 'Constant')
    error('ExpandConst was called on a block that is not a constant.');
end

%There may be many output arcs (connected to different blocks) but should
%all have the same properties
out_arc = constBlock.out_arcs(1);

%Check for matrix ports, we don't support them at the moment
if length(out_arc.dimension) >2 || (out_arc.dimension(1) > 1 && out_arc.dimension(2) > 1)
    error('Constant block expansion does not currently support N-dimensional matrix port types.');
end

if out_arc.width <= 1
    %TODO: Remove this check
    %Sanity check for disagreement with dimension parameter and width
    %parameter
    if out_arc.dimension(1) > 1 || out_arc.dimension(2) > 1
        error('Disagreement between dimension and width parameters for arc durring Constant block expansion');
    end
    
    %The output arc is not a vector.  No work to do, return
    return;
end

%% Find Expansion Array Values
%OK, now ready to start expanding
expansion_occured = true;

%Check if the value is a scalar or vector.
orig_value = constBlock.dialogPropertiesNumeric('Value');

if length(orig_value) > 1 
    %The value is a matric
    
    %Check that the dimensions match the port
    if length(orig_value) ~= out_arc.width
        error('Dimension mismatch between Constant block value and port width.');
    end
    
    value_array = orig_value;
else
    %The value is a scalar.
    
    %The array is just the scalar replicated for each array entry
    value_array = orig_value .* ones(1, out_arc.width);
end

%% Apply Expansion

%value_array constains what each constant should be

%Create Vector_Fan object
vector_fan_output = VectorFan('Fan-In', constBlock); %Indevidual Arcs will be fanning into the bus

for i = 1:length(value_array)
    %Create new constant block.  Copy Certain parameters from origional.
    node = createExpandNodeNoSimulinkParams(constBlock, 'Standard', 'Constant'); %Constants are 'Standard' nodes.  This function adds the node as a child of the parent.
    
    %Set Params
    node.simulinkBlockType = constBlock.simulinkBlockType; %'Constant', copy from orig
    
    %Currently, only handling numerics
    node.dialogPropertiesNumeric('Value') = value_array(i);
    node.dialogPropertiesNumeric('SampleTime') = constBlock.dialogPropertiesNumeric('SampleTime'); % Copy from orig
    
    %Create new arc
    %The arc will be a shallow copy of the existing output arc
    %This should copy the arrays (because they are values) and will copy
    %the handles to handle objects but will not clone the handle objects
    %themselves
    arc = copy(out_arc);
    
    %Connect the new arc to new block (add as out_arc)
    arc.srcNode = node;
    arc.srcPortNumber = 1; %Constant outputs use port 1
    arc.width = 1;
    arc.dimension = [1, 1];
    node.addOut_arc(arc);
    %DO NOT add as in_arc since this will be done in bus cleanup
    
    %Fill in intermediate node entries
    %The intermediate node is the origional Constant
    appendIntermediateNodeEntry(obj, constBlock, 1, i, 'Standard', 'Out'); %Since we are expanding a const, the output port of the origional block would be 'standard' and the direction is 'out'
    
    %Add arc to VectorFan
    vector_fan_output.addArc(arc, i);
    
    %Add new arc
    new_arcs = [new_arcs, arc];
    
    %Add new node to array
    expanded_nodes(i) = node;
    
end

%Attach the old arcs (maybe multiple) to the VectorFan and remove it from the out_arcs list
%of the orig object

arcs_to_remove_from_constBlock = [];

for i = 1:length(constBlock.out_arcs)
    out_arc = constBlock.out_arcs(i);
    arcs_to_remove_from_constBlock = [arcs_to_remove_from_constBlock, out_arc];
    
    out_arc.srcNode = vector_fan_output;
    vector_fan_output.addBusArc(out_arc);
end

for i = 1:length(arcs_to_remove_from_constBlock)
    constBlock.removeOut_arc(arcs_to_remove_from_constBlock(i));
end

%Set orig node type to "Expanded" 
constBlock.setNodeTypeFromText('Expanded');


end

