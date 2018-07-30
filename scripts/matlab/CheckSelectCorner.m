function [arcs_to_delete, new_arcs, new_vector_fans] = CheckSelectCorner(node)
%CheckSelectCorner Checks for the corner cases when the Select block
%has a single wire input and output or cases where there is a single wire
%output.

%% Checking for Bad Input
%This method should only be called on select objects, let's check for
%that first
if ~strcmp(node.simulinkBlockType, 'Select')
    error('CheckSelectCorner was called on a block that is not a Select.');
end

arcs_to_delete = [];
new_arcs = [];
new_vector_fans = [];

if length(node.in_arcs) == 1 && length(node.out_arcs) == 1
   in_arc = node.in_arcs(1);
   out_arc = node.out_arcs(1);
   
   if in_arc.width == 1 && out_arc.width == 1
      %Rewire through the select
      %Add select as intermediate
      
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
    out_arc_rep = node.out_arcs(1);
    

    %All out_arcs should have the same width, get the width from the 1st
    if out_arc_rep.width == 1
        %Add a VectorFan (Fan-out) for the output.  Note: there may be
        %multiple output ports from the select
        
        %Add vector fan (fan-out) object
        fan_out = VectorFan('Fan-out', node.parent);
        new_vector_fans = [new_vector_fans, fan_out];
        
        %Create an arc to connect the select to the Fan-out
        connector = copy(out_arc_rep);
        new_arcs = [new_arcs, connector];
        %Src should be the selector as this arc is a copy of a selector
        %out_arc
        connector.dstNode = fan_out;
        %leave the dst port numbers and type unchanged
        
        %Add this as an out_arc of the select and a bus arc of the fanout
        node.addOut_arc(connector);
        fan_out.addBusArc(connector);
        
        %Connect the output arcs to the vector fan object
        %Do not need to remove them but do need to remove them as out_arcs
        %from the select node
        arcs_to_remove_from_select = [];
        for i = length(node.out_arcs)
            out_arc = node.out_arcs(i);
            %dst should remain unchanged
            %src will be fixed in bus cleanup

            fan_out.addArc(out_arc, 1);
            
            arcs_to_remove_from_select = [arcs_to_remove_from_select, out_arc];
        end
        
        for i = 1:length(arcs_to_remove_from_select)
            out_arc = arcs_to_remove_from_select(i);
            node.removeOut_arc(out_arc);
        end
            
    end
end

end

