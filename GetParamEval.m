function param_eval = GetParamEval(simulinkBlockHandle, param_name)
%GetParamEval Similar to get_param except that the parameter is evaluated
%within the block's context

param_uneval = get_param(simulinkBlockHandle, param_name);
param_eval = EvalWithinBlockScope(simulinkBlockHandle, param_uneval);

end

