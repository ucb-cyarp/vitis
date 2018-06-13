function [must_keep_vector_fans, other_vector_fans, new_arcs] = ExpandMaster(node)
%ExpandMaster Handles the insertion of VectorFan objects to handle Vector
%inputs and outputs of master nodes.

%For vector out_arcs of master nodes, a VectorFan (Fan-Out) is inserted
%followed by a VectorFan (Fan-In).  The Fan-Out must always be kept 

%For vector in_arcs of master nodes, a VectorFan (Fan-Out) is inserted
%followed by a VectorFan (Fan-In) which is in turn connected to the master
%node.  The Fan-In node must be kept.

%% Checking for Bad Input
%This method should only be called on constant objects, let's check for
%that first
if ~node.isMaster()
    error('ExpandConst was called on a block that is not a master.');
end

must_keep_vector_fans = [];
other_vector_fans = [];

%Itterate through the out_arcs
for i = 1:length(node.out_arcs)
    out_arc = node.out_arcs(i);
    
    if out_arc.width > 1
        %Insert the VectorFans
        
        fan_out = VectorFan('Fan-Out', node.parent);
        must_keep_vector_fans = [must_keep_vector_fans, fan_out];
        fan_in = VectorFan('Fan-In', node.parent);
        other_vector_fans = [other_vector_fans, fan_in];
        
        arc_copy = copy(out_arc);
        new_arcs = [new_arcs, arc_copy];
        
        %Connect orig arc to the bus side of the Fan-Out
        %*Since this node is not traversed (and must be emitted), let's add
        %this arc directly as an in_arc.
        out_arc.dstNode = fan_out;
        fan_out.addIn_arc(out_arc);
        
        %Connect the arc copy to the bus side of the Fan-In
        arc_copy.srcNode = fan_in;
        fan_in.addBusArc(arc_copy);
        
        %Create indevidual wires between the VectorFan objects
        for j = 1:out_arc.width
            wire = copy(out_arc); %Copy to preserve metadata
            new_arcs = [new_arcs, wire];
            
            %*Since the Fan-Out node is not traversed (and must be
            %emitted), these wires will be added as out_arcs rather than
            %wire arcs
            wire.srcNode = fan_out;
            wire.srcPortNumber = i;
            wire.width = 1;
            wire.dimension = [1, 1];
            fan_out.addOut_arc(wire);
            
            %Dst will be fixed by the traversal of the Fan-In.
            fan_in.addArc(wire, i);
        end
    end
end

%Itterate through the in_arcs
for i = 1:length(node.in_arcs)
    in_arc = node.in_arcs(i);
    
    if in_arc.width > 1
        %Insert the VectorFans
        
        fan_out = VectorFan('Fan-Out', node.parent);
        other_vector_fans = [other_vector_fans, fan_out];
        fan_in = VectorFan('Fan-In', node.parent);
        must_keep_vector_fans = [must_keep_vector_fans, fan_in];
        
        arc_copy = copy(in_arc);
        new_arcs = [new_arcs, arc_copy];
        
        %Connect orig arc to the bus side of the Fan-in
        %*Since this node is not traversed (and must be emitted), let's add
        %this arc directly as an out_arc.
        out_arc.srcNode = fan_out;
        fan_out.addOut_arc(out_arc);
        
        %Connect the arc copy to the bus side of the Fan-Out
        arc_copy.dstNode = fan_out;
        fan_out.addBusArc(arc_copy);
        
        %Create indevidual wires between the VectorFan objects
        for j = 1:out_arc.width
            wire = copy(out_arc); %Copy to preserve metadata
            new_arcs = [new_arcs, wire];
            
            %*Since the Fan-In node is not traversed (and must be
            %emitted), these wires will be added as in_arcs rather than
            %wire arcs
            wire.dstNode = fan_in;
            wire.dstPortNumber = i;
            wire.width = 1;
            wire.dimension = [1, 1];
            fan_in.addIn_arc(wire);
            
            %src will be fixed by the traversal of the pair Fan-In nodes (elsewhere in the design).
            fan_out.addArc(wire, i);
        end
    end
end


end

