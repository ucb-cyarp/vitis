function PopulateTopLevelNodeWorkspaceVars(node)
%PopulateTopLevelNodeWorkspaceVars Populates the IR node mask parameters 
%of the top level node.  These consist of the base workstation vars.

base_workspace_var_names = evalin('base', 'who');
for i = 1:length(base_workspace_var_names)
    name = base_workspace_var_names{i};
    val = evalin('base', name);
    
    node.maskVariables(name) = val;
end

end

