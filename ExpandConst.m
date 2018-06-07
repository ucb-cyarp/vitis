function [outputArg1,outputArg2] = ExpandConst(constBlock)
%ExpandConst Expands a vector constant into seperate constants.
%The existing node is converted into a Expanded(Subsystem).  A VectorFan
%node is created and arcs from each of the constants are included in it's
%array.

%This method should only be called on constant objects, let's check for
%that first
if ~strcmp(constBlock.simulinkBlockType, 'Constant')
    error('ExpandConst was called on a block that is not a constant.');
end

%Get the arc attached to the constant and check that it is indeed a
%vector/bus type.  There should only be 1 out arc.
if length(constBlock.out_arcs) ~= 1
    error('Constant is only expected to have a single output arc');
end

out_arc = constBlock.out_arcs(1);


outputArg1 = inputArg1;
outputArg2 = inputArg2;
end

