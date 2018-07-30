function assigninHere(name,val)
%nestedAssignInCaller Replicates the behavior of assignin for the current
%scope.  Is helpful if you need to create variables in the current scope
%but do not have a string that can be passed to eval.
assignin('caller', name, val);
end

