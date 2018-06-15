function [expansion_occured, expanded_nodes, vector_fans, new_arcs, arcs_to_delete] = ExpandSum(sumBlock)
%ExpandSum Expands a vector operation (+, -) into replicas of
%that operation on each element of the vector.  Replicas are contained
%within the given node.  Node type is changed to subsystem.

%Handle case of some inputs being vectors and some inputs being scalars.

%scalar op scalar has no effect and the function simply returns

%Note: It turns out that +, - are both Sum operators

%Note: There can be more than 2 inputs to operators but there should only
%be 1 output

expansion_occured = false;
expanded_nodes = [];
vector_fans = [];
new_arcs = [];
arcs_to_delete = [];

%% Checking for Bad Input
%This method should only be called on constant objects, let's check for
%that first
if ~strcmp(sumBlock.simulinkBlockType, 'Sum')
    error('ExpandSum was called on a block that is not a Sum.');
end

%There may be many output arcs (connected to different blocks) but should
%all have the same properties
out_arc = sumBlock.out_arcs(1);

%Check for matrix ports, we don't support them at the moment
if length(out_arc.dimension) >2 || (out_arc.dimension(1) > 1 && out_arc.dimension(2) > 1)
    error('Sum block expansion does not currently support N-dimensional matrix port types.');
end

if out_arc.width <= 1
    %TODO: Remove this check
    %Sanity check for disagreement with dimension parameter and width
    %parameter
    if out_arc.dimension(1) > 1 || out_arc.dimension(2) > 1
        error('Disagreement between dimension and width parameters for arc durring Sum block expansion');
    end
    
    %The output arc is not a vector (and transitivitily the input arc is not a vector).  No work to do, return
    return;
end

busWidth = out_arc.width;

%% Apply Expansion

%value_array constains what each constant should be

%Create Vector_Fan objects

%Should only be 1 output port (but potentially with many output arcs)
vector_fan_output = VectorFan('Fan-In', sumBlock); %Indevidual Arcs will be fanning into the bus

vector_fans = [vector_fan_output];

%Need to create VectorFan inputs for each vector port (there can only be 1
%arc to each input port).  Note that the array is not sorted by port number
vector_inputs = [];
input_arc_isvector = [];
input_arc_vectorfan_index = [];
input_vectorfan_count = 0;
for i = 1:length(sumBlock.in_arcs)
    in_arc = sumBlock.in_arcs(i);
    
    if in_arc.width > 1
        vector_fan_input = VectorFan('Fan-Out', sumBlock); %Indevidual Arcs will be fanning out from the bus
        vector_fans = [vector_fans, vector_fan_input];
        vector_inputs = [vector_inputs, vector_fan_input];
        input_arc_isvector = [input_arc_isvector, true];
        input_arc_vectorfan_index = [input_arc_vectorfan_index, input_vectorfan_count+1];
        input_vectorfan_count = input_vectorfan_count + 1;
    else
        input_arc_isvector = [input_arc_isvector, false];
        input_arc_vectorfan_index = [input_arc_vectorfan_index, 0];
    end 
end

%Create new blocks and connect them to vectorfans
%For scalar inputs
for i = 1:busWidth
    %Create new delay block.  Copy Certain parameters from origional.
    node = GraphNode.createExpandNodeNoSimulinkParams(sumBlock, 'Standard', 'Sum', i); %Sums are 'Standard' nodes.  This function adds the node as a child of the parent.
    
    %Set Params
    node.simulinkBlockType = sumBlock.simulinkBlockType; %'Sum', copy from orig
    
    %Currently, only handling numerics
    node.dialogPropertiesNumeric('SampleTime') = sumBlock.dialogPropertiesNumeric('SampleTime'); % Copy from orig
    node.dialogProperties('Inputs') = sumBlock.dialogProperties('Inputs');
    
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
    %Dst entries will be filled in durring bus cleanup
    %DO NOT add as in_arc since this will be done in bus cleanup
    
    %Fill in intermediate node entries
    %The intermediate node is the origional Constant
    out_wire_arc.prependIntermediateNodeEntry(sumBlock, 1, i, 'Standard', 'Out'); %Since we are expanding a sum, the output port of the origional block would be 'standard' and the direction is 'out'
    
    %Add arc to VectorFan Output
    vector_fan_output.addArc(out_wire_arc, i);
    
    %Add new arc
    new_arcs = [new_arcs, out_wire_arc];
    
    %There will likely be multiple input ports
    
    for j = 1:length(input_arc_isvector)
        in_arc = sumBlock.in_arcs(j);
        
        if input_arc_isvector(j)
            %This input comes from a VectorFan input object
            %---- Create new arc for input ----
            in_wire_arc = copy(in_arc);

            %Connect the new arc to new block (add as out_arc)
            in_wire_arc.dstNode = node;
            in_wire_arc.dstPortNumber = in_arc.dstPortNumber; %preserve the dst port number of the origional.  Note that j is not nessisarily the port number as the arc array is not sorted by port number
            in_wire_arc.width = 1;
            in_wire_arc.dimension = [1, 1];
            node.addIn_arc(in_wire_arc);
            %Src entries will be filled in by bus cleanup
            %DO NOT add as out_arc since this will be done in bus cleanup

            %Fill in intermediate node entries
            %The intermediate node is the origional Constant
            %i is still the wire number, j is the port number
            in_wire_arc.appendIntermediateNodeEntry(sumBlock, 1, i, 'Standard', 'In'); %Since we are expanding a const, the output port of the origional block would be 'standard' and the direction is 'out'

            %Add arc to VectorFan Input
            vector_fan_input_ind = input_arc_vectorfan_index(j);
            vector_fan_input = vector_inputs(vector_fan_input_ind);
            vector_fan_input.addArc(in_wire_arc, i);

            %Add new arc
            new_arcs = [new_arcs, in_wire_arc];
            
        else
            %The input is a scalar arc
            %Create a copy of this arc and attach it to the new node as
            %well as connecting it to the origional src node
            
            in_wire_arc = copy(in_arc);

            %Connect the new arc to new block (add as out_arc)
            in_wire_arc.dstNode = node;
            in_wire_arc.dstPortNumber = in_arc.dstPortNumber; %preserve the dst port number of the origional.  Note that j is not nessisarily the port number as the arc array is not sorted by port number
            in_wire_arc.width = 1;
            in_wire_arc.dimension = [1, 1];
            node.addIn_arc(in_wire_arc);
            %Src entries will be filled in by bus cleanup
            %DO NOT add as out_arc since this will be done in bus cleanup

            %Fill in intermediate node entries
            %The intermediate node is the origional Constant
            %i is still the wire number, j is the port number
            in_wire_arc.appendIntermediateNodeEntry(sumBlock, 1, i, 'Standard', 'In'); %Since we are expanding a const, the output port of the origional block would be 'standard' and the direction is 'out'

            %Add arc to origional src as an output
            %Src was unchanged in the arc copy
            srcNode = in_wire_arc.srcNode;
            srcNode.addOut_arc(in_wire_arc);

            %Add new arc
            new_arcs = [new_arcs, in_wire_arc];
        end
    end
    
    %Add new node to array
    expanded_nodes = [expanded_nodes, node];
    
end

%Attach the old out arcs (maybe multiple) to the VectorFan output and remove it from the out_arcs list
%of the orig object

arcs_to_remove_from_sumBlock = [];

for i = 1:length(sumBlock.out_arcs)
    out_arc = sumBlock.out_arcs(i);
    arcs_to_remove_from_sumBlock = [arcs_to_remove_from_sumBlock, out_arc];
    
    out_arc.srcNode = vector_fan_output;
    vector_fan_output.addBusArc(out_arc);
end

for i = 1:length(arcs_to_remove_from_sumBlock)
    sumBlock.removeOut_arc(arcs_to_remove_from_sumBlock(i));
end

%attach the old in_arcs to the VectorFans if they are busses and delete
%them if they were scalar (having been replaced)

in_arc_to_remove_from_sum = [];
for i = 1:length(sumBlock.in_arcs)
    in_arc = sumBlock.in_arcs(i);
    
    if input_arc_isvector(i)
        %This is a vector input, connect the arc to the VectorFan object
        %and remove it from the orig object
        
        vector_fan_input_ind = input_arc_vectorfan_index(i);
        vector_fan_input = vector_inputs(vector_fan_input_ind);
            
        in_arc.dstNode = vector_fan_input;
        vector_fan_input.addBusArc(in_arc);
        in_arc_to_remove_from_sum = [in_arc_to_remove_from_sum, in_arc];
    else
        %This is a scalar arc which has been replaced.  It will be removed
        %as an out_arc from the src.  This arc shoud also be removed
        
        srcNode = in_arc.srcNode;
        srcNode.removeOut_arc(in_arc);
        in_arc_to_remove_from_sum = [in_arc_to_remove_from_sum, in_arc];
        arcs_to_delete = [arcs_to_delete, in_arc];
    end
end

for i = 1:length(in_arc_to_remove_from_sum)
    in_arc = in_arc_to_remove_from_sum(i);
    sumBlock.removeIn_arc(in_arc);
end 

%Set orig node type to "Expanded" 
sumBlock.setNodeTypeFromText('Expanded');

end

