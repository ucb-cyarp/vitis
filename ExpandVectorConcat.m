function [outputArg1,outputArg2] = ExpandVectorConcat(inputArg1,inputArg2)
%ExpandVectorConcat Expands vector concatinate block.  In effect, removes
%the vector concatenate block since it performs no operations when
%flattened.  Connect input and output bus wires directly.


outputArg1 = inputArg1;
outputArg2 = inputArg2;
end

