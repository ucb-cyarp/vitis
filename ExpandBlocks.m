function [new_nodes, synth_vector_fans, new_arcs, arcs_to_delete] = ExpandBlocks(nodes, UnconnectedMasterNode)
%ExpandBlocks Expands vector operations and special blocks into primitive,
%scalar blocks.

%There is a sequence in which expansion should occur:

% Special Blocks should go first since they may include vector math within
% them.  (ex. a tapped delay line whose inputs are vectors would produce
% delay blocks whose inputs are vectors).

%TODO: For now, we will restrict the cases where special blocks can have
%vector inputs to a couple specific situations.  Allowed cases will include
%vector inputs to the FIR filter coefficient port.  Other cases will throw
%an error for now.

%The primitive expansion should go last since the special blocks which may
%include vector operations would have been expanded by then.

%Finally, vector concatinates and bus cleanup go last.  Vector concatenates
%are completely removed since they perfrom no operation.


%**** Expansion ****
%---- Block Expansion ----
%Block expansion invoves replacing high level blocks with lower level
%implementations.  In order to maintain hierarchy and to make it easier to
%replace the low level implementation later, the high level block is not
%removed - it is converted into a subsystem.  To preserve information which
%may be lost when a node is expanded, intermediate node values are included
%in arc objects.  When an arc is rewired from a port on the outer block to
%one of the inner ones, the intermediate node values are populated.  The
%arc is also removed from the in_arc and out_arc lists.

%NOTE: There may be multiple intermediate nodes.  For example, a tapped
%delay block may be fed into a vector multiply.  This results in 2
%intermediates.  As a result, the intermediate variables can contain
%arrays.  When a block is expanded, the fan-in/fan-out arcs that are
%created copy the intermediate node entries

%NOTE: The Expand operations will currently only use the numerically parsed
%versions of parameters.  While symbolic versions of these expansions are
%theroetically possible, it is not as simple as creating a parser for [, , , ]
%styled arrays.  This is because parameters can use the full suite of m
%code.  For example, 0.5.*ones(1, varname) is a valid expression.  Properly
%parsing this symbolically requieres implementing the ones function.  List
%creation of the form 1:3 is also allowed.  To avoid re-implementing the m
%lanaguage (at least to start), only the nuermcially parsed versions of 
%parameters will be used.

%---- Bus/Vector Expansion ----
%When blocks are expanded, especially ones which handle vector operations,
%the input or output busses should also be split into seperate wires.

%Since expansion of blocks does not depend on busses being alrrady
%expanded, the bus may become split at the input/output of a block that was
%expanded.  Likewise, the same may occure on the other end of the bus.

%These two split ends need to be rectified by attaching indevidual arcs
%once all blocks have been extanded.

%The reconsiling of connecting split busses is complicated by the inclusion
%of vector concatenate blocks.  The presence of these blocks means that the
%wire number at one end may not match the wire number at the other end.
%The widths of the bus at one end may also not be the same as the width at
%the other end.  The bus at one end may also be composed from wires from
%different blocks.

%++ Procedure ++

%The prodedure for bus expansion starts at block expansion:
%If a block is expanded such that a bus is split into indevidual wires
%(will not handle the case where a bus is split into other busses for now),
%a VectorFan object is created.  The VectorFan object is used to contain arcs
%which have been fanned-in or fanned-out of the bus.  Modeled as a node,
%The VectorFan object contains array of arcs where one direction is left
%unconnected.  It also contains a reference to the origional arc oject before
%fanout.  The origional arc is modified to terminate at the VectorFan object

%Simulink does not allow vector concatenation without a vector concat
%block.  As a result the single end of the VectorFan object will only
%consist of one arc object.  This arc may connect to a VectorConcatenate or
%select block which change the input/output wire numbers for a vector.

%Bus cleanup involves visiting each arc contained in each VectorFan object
%and tracing through the graph (via the VectorFan object).  While tracing,
%it keeps track of what wire number it is in the bus/vector.  This number
%may change if a vector concatenate or select block is reached.  This
%terminated once another VectorFan block is reached at which point the arcs
%can be connected.  Actually, only one of the arcs remains, one is deleted.

%Note: Intermediate node entries are added to the arc as the graph is
%traversed.  The intermediate node entries for the nodes in the remote
%VectorFan object are also added before the other is deleted.

%There should usually be VectorFan objects at either end of a vector/bus.
%that is because bus cleanup occurs only after expansion.  By this point,
%all blocks that deal with vectors should have been expanded.  Each of
%these blocks should have created VectorFan objects.
% **** The exception is if the bus origionates from or terminates at a
% special node.  In this case, the fanned arcs are directly connected to 
% the VectorFan object and the VectorFan is actually emitted as a node.
% The port number of the arcs connected to the VectorFan denote their wire
% numbers.  The VectorFan is a subclass of GraphNode.  If it is to be
% emitted, the arcs need to be connected as in_arcs and out_arcs.  This is
% so the emit command works as expected.

%The arcs will be linked together by looking at the intermediate node
%entries and noting the port entries

%As part of the cleanup process, VectorFan object's whose arcs have all
%been successfully connected can be deleted.  Those that still have arcs
%connected likely have a path to/from a master node.  The arcs are
%connected to the VectorFan object which is emitted.

%Once a vector fan object is marked for deletion, the bus arc can also be
%deleted as it has been suplanted by direct connections.  The arc should
%also be removed from any concatenate blocks.

%Any time an arc is deleted, it needs to be deleted from the arc arrays at
%the top level.

%Cleanup should also remove arcs from concatenate blocks that have no 
%inputs or outputs.  Likewise any selects that have no input/output should 
%also have arcs removed.
%The blocks will not be removed so that the intermediate node entries
%associated with these blocks will have valid references.

%This can be done itterativly until no new arcs are removed.  Sould only
%need to check concatrnate and select blocks since all other block
%expansions should have resulted in a VectorFan node.  In fact, no vector
%concatenates should exist in the graph except when connected to master
%nodes.

%
%

%% Expand

%Try to expand each node in the graph.  Nodes that should not be expanded
%will be left untouched.

new_nodes = [];
synth_vector_fans = [];
new_arcs = [];
arcs_to_delete = [];

%Used internally and not returned
vector_fans = []; %Will be removed in bus cleanup

% ---- Expand FIR Only ----

% ---- Expand Tapped Delay ---

% ---- Expand Primitives ----
%Include new nodes created durring FIR expansion
%Also includes master nodes

% ++++ Catch Single Width Concat Inputs, Single Width Concat Output ++++
% If Single width Input & Ouput, simply remove and reconnect wires
% (deleting one)
% Otherwise, for all inputs of width 1, add degenerate VectorFan (Fan-In) objects
% ++++ Catch Single Width Select Inputs, Single Width Concat ++++
% If Single width Input & Ouput, simply remove and reconnect wires
% (deleting one)
% Otherwise, on output, add degenerate VectorFan (Fan-Out) object

for i = 1:length(nodes) %Do not need to include VectorFan nodes in this list
    node = nodes(i);
    
    %Call the expanded for the type of block
    %DO NOT
    if node.isMaster()
        %Expand masters to handle bus types (inserts VectorFan objects)
        [must_keep_vector_fans, other_vector_fans, new_new_arcs] = ExpandMaster(node);
        synth_vector_fans = [synth_vector_fans, must_keep_vector_fans];
        vector_fans = [vector_fans, other_vector_fans];
        new_arcs = [new_arcs, new_new_arcs];
    elseif node.isSpecial()
        error('Special Block Expansion Not Implemented Yet');
    elseif node.isStandard() && (strcmp(node.simulinkBlockType, 'Sum') || strcmp(node.simulinkBlockType, 'Product'))
        error('Op Block Expansion Not Implemented Yet');
    elseif node.isStandard() && strcmp(node.simulinkBlockType, 'Delay')
        error('Delay Block Expansion Not Implemented Yet');
    elseif node.isStandard() && strcmp(node.simulinkBlockType, 'Switch')
        error('Switch Block Expansion Not Implemented Yet');
    elseif node.isStandard() && strcmp(node.simulinkBlockType, 'Constant')
        %Expand Constants
        [expansion_occured, new_expanded_nodes, new_vector_fans, new_new_arcs] = ExpandConst(node);
        new_nodes = [new_nodes, new_expanded_nodes];
        vector_fans = [vector_fans, new_vector_fans];
        new_arcs = [new_arcs, new_new_arcs];
    elseif node.isStandard() && strcmp(node.simulinkBlockType, 'Concatenate')
        %Catch Concatenate Corner Cases
        [new_arcs_to_delete, new_new_arcs, new_vector_fans] = CheckConcatCorner(node);
        arcs_to_delete = [arcs_to_delete, new_arcs_to_delete];
        new_arcs = [new_arcs, new_new_arcs];
        vector_fans = [vector_fans, new_vector_fans];
    elseif node.isStandard() && strcmp(node.simulinkBlockType, 'Select')
        %Catch Select Corner Cases
        [new_arcs_to_delete, new_new_arcs, new_vector_fans] = CheckSelectCorner(node);
        arcs_to_delete = [arcs_to_delete, new_arcs_to_delete];
        new_arcs = [new_arcs, new_new_arcs];
        vector_fans = [vector_fans, new_vector_fans];
    end
    
    %If not one of the recognized types, do not expand
    

end

%% Reconnection
% Traverse from all VectorFan (Fan-In) objects 

%% Bus Cleanup
% Remove all VectorFan objects not directly connected to a master node.
%
% For Fan-In objects, all of the wires should either have been reconnected
% to a Fan-Out vectorfan or connected to the "Unconnected" master node.
%
% For Fan-Out objects, each of their wires has to be driven by either an
% input (including from the "Unconnected" master node) or the result of 
% another block.  Otherwise a bus width error would have occured.  Either
% of these cases would have resulted in the reconnection of the Fan-Out's
% wires with wires of a Fan-In object durring the Fan-In trace.
%
% As a result, any bus arc within the design (which should always be
% between 2 VectorFan objects and potentially some Concatnate and Select
% blocks) should be able to be deleted.  This is because all wires in the
% bus should have been connected
%
% The VectorFan objects connected to Master nodes need to be retained
% because all buses within the design will have been flattened but the I/O
% will not have been flattened.
%
% Concatenate and Select blocks can have all arcs connected to them removed
% as they basically change the wire mapping of busses. (NOTE: this requires
% that the degenerate case of single width busses is handled by either
% direct bypass of the block or the insertion of degenerate VectorFan
% objects).
%
% In fact, any bus arc within the design that is not connected to a master
% node can be removed.  This should be accomplished by removing any arc
% connected to the bus end of a VectorFan (not connected to a master) and
% the arcs connected to select and concatenate blocks

%Reconnect Arcs - From Fan-In VectorFan objects
for i = 1:length(vector_fans)
    vector_fan = vector_fans(i);
    
    if vector_fan.isFanIn()
        new_arcs_to_delete = vector_fan.reconnectArcs(UnconnectedMasterNode);
        arcs_to_delete = [arcs_to_delete, new_arcs_to_delete];
    end
        
end

%Remove Arcs from Concat and Select Blocks
arc_to_remove_from_ends = [];
for i = 1:length(nodes) %Do not need to include VectorFan nodes in this list
    node = nodes(i);
    
    arc_to_remove_from_ends = [];
    if node.isStandard() && (strcmp(node.simulinkBlockType, 'Concatenate') || strcmp(node.simulinkBlockType, 'Select'))
        arc_to_remove_from_ends = [arc_to_remove_from_ends, node.in_arcs];
        arc_to_remove_from_ends = [arc_to_remove_from_ends, node.out_arcs];
    end
end

for i = 1:length(arc_to_remove_from_ends)
    arc = arc_to_remove_from_ends(i);
    srcNode = arc.srcNode;
    dstNode = arc.dstNode;
    
    srcNode.removeOut_arc(arc);
    dstNode.removeIn_arc(arc);
end

arcs_to_delete = [arcs_to_delete, arc_to_remove_from_ends];
