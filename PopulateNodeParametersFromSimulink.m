function PopulateNodeParametersFromSimulink(node, simulink_block_handle)
%PopulateNodeParametersFromSimulink Populates the IR node parameters from
%simulink.  The DialogParameters for each block are saved into the
%dialogParameters map.

%If the node is a Mask, the mask variables are saved to the maskVariables
%map.  They are emitted to the XML with the prefix MaskVariable.<VariableName>.

%If the node is of a type which is known (ex. Delay, Discrite FIR Filter,
%...) certain parameters will be evaluated into numeric values.  This is
%because parameters can be arbnitrary m code expressions referencing
%variables and even functions.
%Evaluated parameters are saved to the dialogParametersNumeric map.  They
%are emitted to the XML with the prefix Numeric.<ParameterName>.

node.simulinkHandle = simulink_block_handle;
node.simulinkBlockType = get_param(simulink_block_handle, 'BlockType');

%---Get parameters from simulink---
%Get the list of parameter names
dialog_params = get_param(simulink_block_handle, 'DialogParameters');
dialog_param_names = fieldnames(dialog_params);

%Itterate through the dialog parameter names
for i = 1:length(dialog_param_names)
    dialog_param_name = dialog_param_names{i};

    dialog_param_value = get_param(simulink_block_handle, dialog_param_name);

    node.dialogProperties(dialog_param_name) = dialog_param_value;
end

%Check if this is a mask
params = get_param(simulink_block_handle, 'ObjectParameters');
if isfield(params, 'Mask')
    mask_param = get_param(simulink_block_handle, 'Mask');
    
    if ~strcmp(mask_param, 'off')
        %This is a mask so dump the variables into the map
        mask_obj = Simulink.Mask.get(simulink_block_handle);
        mask_vars = mask_obj.getWorkspaceVariables;
        
        for i = 1:length(mask_vars)
            mask_var = mask_vars(i);
            node.maskVariables(mask_var.Name) = mask_var.Value;
        end
    end
end

%Check if this block is one where numeric evaluations are available

%---- Constant ----
if strcmp(node.simulinkBlockType, 'Constant')
    node.dialogPropertiesNumeric('Value') = GetParamEval(simulink_block_handle, 'Value');
    %VectorParams1D is a checkbox that evaluates to text (ex 'on').  Will
    %not parse
    node.dialogPropertiesNumeric('SampleTime') = GetParamEval(simulink_block_handle, 'SampleTime');

%---- Delay ----
elseif strcmp(node.simulinkBlockType, 'Delay')
    
    if strcmp( get_param(simulink_block_handle, 'DelayLengthSource'), 'Dialog')
        %Only parse if dialog box is used for value
        node.dialogPropertiesNumeric('DelayLength') = GetParamEval(simulink_block_handle, 'DelayLength');
    end
    
    if strcmp( get_param(simulink_block_handle, 'InitialConditionSource'), 'Dialog')
        %Only parse if dialog box is used for value
        node.dialogPropertiesNumeric('InitialCondition') = GetParamEval(simulink_block_handle, 'InitialCondition');
    end
    
    node.dialogPropertiesNumeric('SampleTime') = GetParamEval(simulink_block_handle, 'SampleTime');
  
%---- FIR ----
elseif strcmp(node.simulinkBlockType, 'DiscreteFir')
    if strcmp( get_param(simulink_block_handle, 'CoefSource'), 'Dialog parameters')
        %Only parse if dialog box is used for value
        node.dialogPropertiesNumeric('Coefficients') = GetParamEval(simulink_block_handle, 'Coefficients');
    end
    
    node.dialogPropertiesNumeric('InitialStates') = GetParamEval(simulink_block_handle, 'InitialStates');
    
    node.dialogPropertiesNumeric('SampleTime') = GetParamEval(simulink_block_handle, 'SampleTime');

%---- Tapped Delay ----
%Note: block type is S-Function
elseif strcmp( get_param(simulink_block_handle, 'ReferenceBlock'), 'simulink/Discrete/Tapped Delay' ) || strcmp( get_param(simulink_block_handle, 'ReferenceBlock'), 'hdlsllib/Discrete/Tapped Delay' )
        node.dialogPropertiesNumeric('vinit') = GetParamEval(simulink_block_handle, 'vinit');
        
        node.dialogPropertiesNumeric('NumDelays') = GetParamEval(simulink_block_handle, 'NumDelays');
        
        node.dialogPropertiesNumeric('samptime') = GetParamEval(simulink_block_handle, 'samptime');
    
%---- Selector ----
elseif strcmp(node.simulinkBlockType, 'Selector')
    %Selects from a matrix/vector
    node.dialogPropertiesNumeric('NumberOfDimensions') = GetParamEval(simulink_block_handle, 'NumberOfDimensions');
    
    if node.dialogPropertiesNumeric('NumberOfDimensions') ~= 1
        error('Currently only support 1D array wires (vectors).  Selector is set to a dimension != 1');
    end
    
    index_mode = get_param(simulink_block_handle, 'IndexMode');
    
    if strcmp(index_mode{1}, 'One-based')
        node.dialogPropertiesNumeric('index_mode') = 1;
    elseif strcmp(index_mode{1}, 'Zero-based')
        node.dialogPropertiesNumeric('index_mode') = 0;
    else
        error('Unexpected IndexMode for Selector');
    end
    
    index_options = get_param(simulink_block_handle, 'IndexOptionArray');
    
    if ~strcmp(index_options{1}, 'Index vector (dialog)')
        error('Seletor is currently only supported when the Index Option is set to ''Index vector (dialog)''');
    end
    
    index_params = GetParamEval(simulink_block_handle, 'IndexParamArray');
    node.dialogPropertiesNumeric('IndexParamArray') = index_params{1};
    
    node.dialogPropertiesNumeric('InputPortWidth') = GetParamEval(simulink_block_handle, 'InputPortWidth');
    
    node.dialogPropertiesNumeric('SampleTime') = GetParamEval(simulink_block_handle, 'SampleTime');
end
%TODO: More Blocks
    
end

