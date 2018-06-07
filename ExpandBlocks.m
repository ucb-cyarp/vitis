function [outputArg1,outputArg2] = ExpandBlocks(inputArg1,inputArg2)
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

%There should alwas be VectorFan objects at either end of a vector/bus.
%that is because bus cleanup occurs only after expansion.  By this point,
%all blocks that deal with vectors should have been expanded.  Each of
%these blocks should have created VectorFan objects.

%The arcs will be linked together by looking at the intermediate node
%entries and noting the port entries

%NOTE: The vector fan objects are only temporary node objects and are not
%emitted.  The can actually be deleted at the end of the bus/vector cleanup
%process.

outputArg1 = inputArg1;
outputArg2 = inputArg2;
end

