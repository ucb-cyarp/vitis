function [outputArg1,outputArg2] = ExpandOp(inputArg1,inputArg2)
%ExpandOp Expands a vector operation (+, -, *, /, ...) into replicas of
%that operation on each element of the vector.  Replicas are contained
%within the given node.  Node type is changed to subsystem.

%Handle case of some inputs being vectors and some inputs being scalars.
%The 

%scalar op scalar has no effect and the function simply returns

outputArg1 = inputArg1;
outputArg2 = inputArg2;
end

