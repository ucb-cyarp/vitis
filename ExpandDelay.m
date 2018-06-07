function [outputArg1,outputArg2] = ExpandDelay(inputArg1,inputArg2)
%ExpandDelay Expands a delay whose input/output are vector types.  The
%delay is expanded into single delay blocks for each element of the vector.
% If the input/output is a scalar, no action is taken.
%
% This is from ExpandTappedDelay since, in that case, a series on inputs
% are delayed and the outputs of the delays are concatenated to form a
% vector.

% NOTE: The Intial condition can be a scalar or a vector/matrix.  If the
% initial condition is a vector/matrix, the dimension must match that of
% the input.  If the initial condition is a scalar, the value is used for
% each component of the vector.

outputArg1 = inputArg1;
outputArg2 = inputArg2;
end

