function result = EvalWithinBlockScope(simulinkBlockHandle, expr)
%EvalWithinBlockScope Evaluates an expression (provided as a string) from the
%scope of a given simulink block.

%NOTE: It is expected that blocks within masked subsystems only use
%varibles defined in the mask.
%TODO: make this more general

%First, search up through the Simulink hierarchy to find if the 

hier = get_param(simulinkBlockHandle, 'Parent');

mask_stack = CellList();

while ~isempty(hier)
    %While we have not yet reached the top
    
    %Check for the Mask parameter (top level does not have it for example)
    params = get_param(hier, 'ObjectParameters');
    
    if isfield(params, 'Mask')
        mask_param = get_param(hier, 'Mask');
        
        if ~strcmp(mask_param, 'off')
            %We found a masked subsystem
            mask_obj = Simulink.Mask.get(hier);
            mask_vars = mask_obj.getWorkspaceVariables;
            mask_stack.pushBack(mask_vars);
        end
    end
    
    hier = get_param(hier, 'Parent');
end


% Construct the variable struct array starting with the base workspace
% and including the mask variables.  The mask variables will be included
% from the upper mask down to the lower one.
vars = [];

base_workspace_var_names = evalin('base', 'who');
for i = 1:length(base_workspace_var_names)
    name = base_workspace_var_names{i};
    val = evalin('base', name);
    
    var_struct.Name = name;
    var_struct.Value = val;
    vars = [vars var_struct];
end

%now append the vars from the masks
while mask_stack.size() > 0
    mask = mask_stack.popBack();
    vars = [vars mask];
end
    
result = EvalWithVars(expr, vars);

delete(mask_stack);
end

