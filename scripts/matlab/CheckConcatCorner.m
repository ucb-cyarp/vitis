function [arcs_to_delete, new_arcs, new_vector_fans] = CheckConcatCorner(node)
%CheckConcatCorner Checks for the corner cases when the Concatenate block
%has a single wire input and output or cases where there are multiple single
%wire inputs

%% Checking for Bad Input
%This method should only be called on concatenate objects, let's check for
%that first
if ~strcmp(node.simulinkBlockType, 'Concatenate')
    error('CheckConcatCorner was called on a block that is not a Concatenate.');
end

arcs_to_delete = [];
new_arcs = [];
new_vector_fans = [];

if length(node.in_arcs) == 1 && length(node.out_arcs) == 1
   in_arc = node.in_arcs(1);
   out_arc = node.out_arcs(1);
   
   if in_arc.width == 1 && out_arc.width == 1
      %Rewire through the concatenate
      %Add concat as intermediate
      
      out_Arc.prependIntermediateNodeEntry(obj, node, in_arc.dstPortNumber, 1, in_arc.dstPortTypeStr(), 'In');
      
      out_arc.srcNode = in_arc.srcNode;
      out_arc.srcPortNumber = in_arc.srcPortNumber;
      
      src_node = in_arc.srcNode;
      
      src_node.removeOut_arc(in_arc);
      src_node.addOut_arc(out_arc);
      
      node.removeIn_arc(in_arc);
      node.removeOut_arc(out_arc);
      
      arcs_to_delete = [arcs_to_delete, in_arc];
      
   end
else
    %Add a VectorFan (Fan-In) for each input port that has width 1 (note,
    %there cannot be multiple drivers to an input port)
    
    %Note, the output port must have a width > 1 if this is the case since
    %wires of width 0 are not allowed and we already checked for the case
    %of single wire in & out.  A single input is possible but it must have
    %a width > 1.  The width of the output must be >= width of input
    out_arc = node.out_arcs(1);
    if out_arc.width <= 1
        error('Concat block with more than 1 input must have a width > 1');
    end
    
    in_arcs_to_remove = [];
    for i = length(node.in_arcs)
        in_arc = node.in_arcs(i);
        
        if in_arc.width == 1
            %Add VectorFan (Fan-In) object
            fan_in = VectorFan('Fan-In', node.parent);
            new_vector_fans = [new_vector_fans, fan_in];
            
            %Add this arc to the fan-in object
            fan_in.addArc(in_arc);
            
            %Dst entries will be filled in durring the bus cleanup
            %Src entries are correct (point to orig driver of concat port)
            %Already an out_arc of the origional driver
            
            %Remove this arc as an input from the concat block.
            in_arcs_to_remove = [in_arcs_to_remove, in_arc]; %Can't remove here since we are reading from that array
            
            %Create a new arc to go from the Fan-in to the concat block
            arc_copy = copy(in_arc);
            new_arcs = [new_arcs, arc_copy];
            %Src entries will be filled in durring bus cleanup
            %Dst entries are correct (point to concat block) since this is
            %a copy of the unmodified in_arc
            
            %Add as an in_arc of concat block
            node.addIn_arc(arc_copy);
            
            %Add as bus arc of fan-in
            fan_in.addBusArc(arc_copy);
        end
    end
    
    %Now actually remove
    for i = 1:length(in_arcs_to_remove)
        in_arc = in_arcs_to_remove(i);
        node.removeIn_arc(in_arc);
    end
    
end

end

