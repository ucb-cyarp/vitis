function input_coefs = ExpandFIR(firNode, topology)
%ExpandFIR Expand FIR Filter Node
%   Expands the FIR filter node into primitive operations.
%   Changes the internal node type to subsystem but does not change the
%   Simulink node type.
%   Rewires arcs to internal blocks.  Adds intermediate node
%   entry to rewired arcs.
%   Removes arcs from in_arcs list.
%   Currently topologies supported are "direct"
%   Supports static coefficients or coefficients from input

% If more than 1 input, 2nd input is coef vector
% If coef vector provided, replace port entry with an array of port entries
% to subblocks.  Bus cleanup will replace this array with indevidual ports
% into block.

% Also inserts datatype conversion where needed.  Datatypes within filter
% can be extracted from DialogParameters.

% Note, intitial condition may be a vector or scalar.  If it is a vector,
% the length will be one less than the length of the coefficient array
% since the first coefficient has no state element preceeding it.

% ==== Check for source of coefs ====
coef_src = fir.dialogProperties('CoefSource');
if strcmp(coef_src, 'Input port')
    input_coefs = true;
elseif strcmp(coef_src, 'Dialog parameters')
    input_coefs = false;
else
    error(['Unsupported option for ''Coef Source'': ' coef_src])
end

%Get input arcs
if input_coefs && length(firNode.in_arcs) ~= 2
    error('FIR filter has more than the expected number of input arcs.  Unsupported FIR variant.');
elseif ~input_coefs && length(firNode.in_arcs) ~= 1
    error('FIR filter has more than the expected number of input arcs.  Unsupported FIR variant.');
end

input_arc = [];
coef_arc = [];
for i = length(firNode.in_arcs)
    in_arc = firNode.in_arcs(i);
    if in_arc.dstPortNumber == 1
        if isempty(input_arc)
            input_arc = in_arc;
        else
            error('FIR: Found 2 input arcs');
        end
    elseif in_arc.dstPortNumber == 2
        if isempty(coef_arc)
            coef_arc = in_arc;
        else
            error('FIR: Found 2 input arcs');
        end
    end
end

% ==== Determine Datatypes ====
% Check if input(s) are complex
complex_type = 0;
for i = length(firNode.in_arcs)
    in_arc = firNode.in_arcs(i);
    port_complexity = in_arc.complex;
    if port_complexity == 1
        complex_type = 1;
    end
end

product_data_type = get_param(firNode.simulinkHandle, 'ProductDataTypeStr');
accum_data_type = get_param(firNode.simulinkHandle, 'AccumDataTypeStr');
out_data_type = get_param(firNode.simulinkHandle, 'OutDataTypeStr');
coef_data_type = get_param(firNode.simulinkHandle, 'CoefDataTypeStr');

%Note that these datatypes are not compiled types.  They can be special
%strings that dictate the inheritance of the type from other types.

% TODO: Verify bit growth behavior

% Coef Data Type can have the following special types:
% Inherit: Same word length as input
%     TODO: Confirm
%     This will be intepreted to be the same type as the input port
if strcmp(coef_data_type, 'Inherit: Same word length as input')
    %Take data type from input arc
    coef_data_type = input_arc.datatype;
    
    %else, keep the specified datatype
end

% Product Data type can have the following special types:
% Inherit: Same as input
%     The same type as the input port
% Inherit: Inherit via internal rule
%     Will assume normal width expansion in a multiply.
%         If floating point, type unchanged
if strcmp(product_data_type, 'Inherit: Same as input')
    %Take data type from input arc
    product_data_type = input_arc.datatype;
elseif strcmp(product_data_type, 'Inherit: Inherit via internal rule')
    %Check if 

% Accumulator Data Type can have the following special types:
% Inherit: Same as product output
%     Same type as the product Data type (above)
% Inherit: Inherit via internal rule
%     Will assume the number of added bits is ceil(log2(num_taps))
%         If floating point, type unchanged
% Inherit: Same as input
%     Same as the input port

% Output Data Type can have the following special types:
% Inherit: Same as accumulator
%     Same as the accumulator type
% Inherit: Same as input
%     Same as the input type

% Using https://www.mathworks.com/help/fixedpoint/examples/perform-fixed-point-arithmetic.html
% as a guide on how bit growth work in matlab

outputArg1 = inputArg1;
outputArg2 = inputArg2;
end

