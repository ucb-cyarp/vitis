function param_eval = GetParamEval(simulinkBlockHandle, param_name)
%GetParamEval Similar to get_param except that the parameter is evaluated
%within the block's context

param_uneval = get_param(simulinkBlockHandle, param_name);

if iscell(param_uneval)
    for i = 1:length(param_uneval)
        param_eval{i} = EvalWithinBlockScope(simulinkBlockHandle, param_uneval{i});
    end
else
    param_eval = EvalWithinBlockScope(simulinkBlockHandle, param_uneval);
end

end

