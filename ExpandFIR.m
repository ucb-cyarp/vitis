function [expansion_occured, expanded_nodes, vector_fans, new_arcs] = ExpandFIR(firNode)
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

% ==== Init Outputs ====
expansion_occured = [];
expanded_nodes = [];
vector_fans = [];
new_arcs = [];

% ==== Check for source of coefs ====
coef_src = firNode.dialogProperties('CoefSource');
if strcmp(coef_src, 'Input port')
    input_coefs = true;
elseif strcmp(coef_src, 'Dialog parameters')
    input_coefs = false;
else
    error(['Unsupported option for ''Coef Source'': ' coef_src])
end

%==== Get input arcs (using port numbers) ====
if input_coefs && length(firNode.in_arcs) ~= 2
    error('FIR filter does not have the expected number of input arcs.  Unsupported FIR variant.');
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

% ==== Get Initial Conditions ====

%Width of the FIR filter comes either from the dialog or input port
if input_coefs
    coefWidth = coef_arc.width;
else
    coefWidth = length(firBlock.dialogPropertiesNumeric('Coefficients'));
end

%Get Initial States
orig_init_value = firBlock.dialogPropertiesNumeric('InitialStates');

if length(orig_init_value) > 1 
    %The value is a matric
    
    %Check that the dimensions match the port
    if length(orig_init_value) ~= (coefWidth-1)
        error('Dimension mismatch for FIR filter initial conditions.');
    end
    
    init_array = orig_init_value;
else
    %The value is a scalar.
    
    %The array is just the scalar replicated for each array entry
    init_array = orig_init_value .* ones(1, coefWidth-1);
end

%Get initial coefs
if ~input_coefs
    orig_coef_value = firBlock.dialogPropertiesNumeric('Coefficients');

    if length(orig_coef_value) > 1 
        %The value is a matric

        %Check that the dimensions match the port
        if length(orig_coef_value) ~= coefWidth
            error('Dimension mismatch for FIR filter initial conditions.');
        end

        coef_array = orig_coef_value;
    else
        %The value is a scalar.

        %The array is just the scalar replicated for each array entry
        coef_array = orig_coef_value .* ones(1, coefWidth);
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

input_data_type = input_arc.datatype;

product_data_type_str = get_param(firNode.simulinkHandle, 'ProductDataTypeStr');
accum_data_type_str = get_param(firNode.simulinkHandle, 'AccumDataTypeStr');
out_data_type_str = get_param(firNode.simulinkHandle, 'OutDataTypeStr');
coef_data_type_str = get_param(firNode.simulinkHandle, 'CoefDataTypeStr');

product_use_specific_datatype = false;
coef_use_specific_datatype = false;
accum_use_specific_datatype = false;
out_use_specific_datatype = false;

%Note that these datatypes are not compiled types.  They can be special
%strings that dictate the inheritance of the type from other types.

% TODO: Verify bit growth behavior

% Coef Data Type can have the following special types:
% Inherit: Same word length as input
%     This will be intepreted to be the same type as the input port
if strcmp(coef_data_type_str, 'Inherit: Same word length as input')
    %Take data type from input arc
    coef_data_type = input_data_type;
    coef_use_specific_datatype = false;
else
    %will need to cast the coefficient to the requested type
    %This sets the coef type in the gain block
    coef_data_type = coef_data_type_str;
    coef_use_specific_datatype = true;
end

% Product Data type can have the following special types:
% Inherit: Same as input
%     The same type as the input port
% Inherit: Inherit via internal rule
%     Will assume normal width expansion in a multiply.
%         If floating point, type unchanged
if strcmp(product_data_type_str, 'Inherit: Same as input')
    %Take data type from input arc
    product_data_type = input_arc.datatype;
    %Do not need to use specific datatype here because the input of the
    %product will be the input of the block
elseif strcmp(product_data_type_str, 'Inherit: Inherit via internal rule')
    input_type_obj = SimulinkType.FromStr(input_data_type);
    coef_type_obj = SimulinkType.FromStr(coef_data_type);
    product_type_obj = SimulinkType.bitGrowMult(input_type_obj, coef_type_obj, 'typical');
    product_data_type = product_type_obj.toString();
else
    product_data_type = product_data_type_str;
    product_use_specific_datatype = true;
end

% Accumulator Data Type can have the following special types:
% Inherit: Same as product output
%     Same type as the product Data type (above)
% Inherit: Inherit via internal rule
%     Will assume the number of added bits is ceil(log2(num_taps))
%         If floating point, type unchanged
% Inherit: Same as input
%     Same as the input port
if strcmp(accum_data_type_str, 'Inherit: Same as input')
    accum_data_type = input_data_type;
    accum_use_specific_datatype = true;
elseif strcmp(accum_data_type_str, 'Inherit: Same as product output')
    accum_data_type = product_data_type;
    accum_use_specific_datatype = false;
    %Do not wire in cast block
elseif strcmp(accum_data_type_str, 'Inherit: Inherit via internal rule')
    typeA_obj = SimulinkType.FromStr(product_data_type);
    typeB_obj = SimulinkType.FromStr(product_data_type);
    accum_data_type_obj = bitGrowAdd(typeA_obj, typeB_obj, coefWidth, 'typical');
    accum_data_type = accum_data_type_obj.toString();
    accum_use_specific_datatype = true;
else
    %Take the specific type
    accum_data_type = accum_data_type_str;
    accum_use_specific_datatype = true;
end

% Output Data Type can have the following special types:
% Inherit: Same as accumulator
%     Same as the accumulator type
% Inherit: Same as input
%     Same as the input type
if strcmp(out_data_type_str, 'Inherit: Same as input')
    out_data_type = input_data_type;
    out_use_specific_datatype = true;
elseif strcmp(out_data_type_str, 'Inherit: Same as accumulator')
    out_data_type = accum_data_type;
    out_use_specific_datatype = false;
else
    %Take the specific type
    out_data_type = out_data_type_str;
    out_use_specific_datatype = true;
end

% ==== Implement ====

%Set Multiply output datatype to that specified unless.  Special case 
%Set the Gain coef type to "Same as input" or to what is specified
%Note that there is a change to 'Inherit: Same as input'
%
%Sum ouput datatype (for the block) will be the same type as input 

%Do not insert cast blocks unless a cast is required

%Data Type Conversion: BlockType=DataTypeConversion
%    OutDataTypeStr: The data type to convert to.

%Note: do not need to interpret DataType conversion, just need to make it

if coefWidth == 1
    %Special case for single coef
    if input_coefs
        product_node = GraphNode.createExpandNodeNoSimulinkParams(firBlock, 'Standard', 'Product', 1);
        product_node.simulinkBlockType = 'Product';
        product_node.dialogPropertiesNumeric('SampleTime') = -1; % Copy from orig
        product_node.dialogProperties('Inputs') = '*';
        if product_use_specific_datatype
            product_node.dialogProperties('OutDataTypeStr') = product_data_type;
        else
            if strcmp(product_data_type_str, 'Inherit: Same as input')
                %Need to change for product
                product_node.dialogProperties('OutDataTypeStr') = 'Inherit: Same as first input';
            else
                product_node.dialogProperties('OutDataTypeStr') = product_data_type_str;
            end
        end
        expanded_nodes = [expanded_nodes, product_node];
        
        firBlock.removeIn_arc(input_arc);
        input_arc.dstNode = product_node;
        input_arc.dstPortNumber = 1;
        input_arc.appendIntermediateNodeEntry(firBlock, 1, 1, 'Standard', 'In');
        product_node.addIn_arc(input_arc);
        
        firBlock.removeIn_arc(coef_arc);
        coef_arc.dstNode = product_node;
        coef_arc.dstPortNumber = 2;
        coef_arc.appendIntermediateNodeEntry(firBlock, 2, 1, 'Standard', 'In');
        product_node.addIn_arc(coef_arc);
        
    else
        product_node = GraphNode.createExpandNodeNoSimulinkParams(firBlock, 'Standard', 'Gain', 1);
        product_node.simulinkBlockType = 'Gain';
        product_node.dialogPropertiesNumeric('Gain') = coef_array(1); % Copy from orig
        product_node.dialogPropertiesNumeric('SampleTime') = -1;
        product_node.dialogProperties('Multiplication') = 'Element-wise(K.*u)';
        if product_use_specific_datatype
            product_node.dialogProperties('ParamDataTypeStr') = coef_data_type;
        else
            product_node.dialogProperties('ParamDataTypeStr') = coef_data_type_str;
        end
        if product_use_specific_datatype
            product_node.dialogProperties('OutDataTypeStr') = product_data_type;
        else
            product_node.dialogProperties('OutDataTypeStr') = product_data_type_str;
        end
        expanded_nodes = [expanded_nodes, product_node];
        
    end
    
    if accum_use_specific_datatype
        accum_dtc_node = GraphNode.createExpandNodeNoSimulinkParams(firBlock, 'Standard', 'DataTypeConversion', 1);
        accum_dtc_node.simulinkBlockType = 'DataTypeConversion';
        accum_dtc_node.dialogPropertiesNumeric('SampleTime') = -1; % Copy from orig
        accum_dtc_node.dialogProperties('OutDataTypeStr') = accum_data_type;
        expanded_nodes = [expanded_nodes, accum_dtc_node];
        
        accum_dtc_atc = GraphArc(product_node, 1, accum_dtc_node, 1, 'Standard');
        accum_dtc_atc.datatype = product_data_type;
        accum_dtc_atc.complex = complex_type;
        accum_dtc_atc.width = 1;
        accum_dtc_atc.dimension = [1, 1];
        new_arcs = [new_arcs, accum_dtc_atc];
        
        prev_block = accum_dtc_node;
    else
        prev_block = product_node;
    end
    
    if out_use_specific_datatype
        out_dtc_node = GraphNode.createExpandNodeNoSimulinkParams(firBlock, 'Standard', 'DataTypeConversion', 1);
        out_dtc_node.simulinkBlockType = 'DataTypeConversion';
        out_dtc_node.dialogPropertiesNumeric('SampleTime') = -1; % Copy from orig
        out_dtc_node.dialogProperties('Inputs') = '*';
        out_dtc_node.dialogProperties('OutDataTypeStr') = out_data_type;
        expanded_nodes = [expanded_nodes, out_dtc_node];
        
        out_dtc_atc = GraphArc(product_node, 1, accum_dtc_node, 1, 'Standard');
        out_dtc_atc.datatype = product_data_type;
        out_dtc_atc.complex = complex_type;
        out_dtc_atc.width = 1;
        out_dtc_atc.dimension = [1, 1];
        new_arcs = [new_arcs, out_dtc_atc];
        
        prev_block = out_dtc_node;
    %else
        %Do not change prev block
    end
   
    %Attach output arcs
    out_arc_to_remove_from_base = [];
    for i = 1:length(firBlock.out_arcs)
        out_arc = firBlock.out_arcs(i);
        
        %By this point, the output arcs should be of the required datatype,
        %only need to change srcs
        out_arc.srcNode = prev_block;
        out_arc.srcPortNumber = 1;
        out_arc.prependIntermediateNodeEntry(firBlock, 1, 1, 'Standard', 'Out');
        prev_block.addOut_arc(out_arc);
        
        out_arc_to_remove_from_base = [out_arc_to_remove_from_base, out_arc];
    end
    
    for i = 1:length(out_arc_to_remove_from_base)
        out_arc = out_arc_to_remove_from_base(i);
        firBlock.removeOut_arc(out_arc);
    end
    
else
    if input_coefs
        vector_fan_coefs = VectorFan('Fan-Out', opBlock); %Indevidual Arcs will be fanning out from the bus
        vector_fans = [vector_fans, vector_fan_coefs];

        %rewire input coef arc
        coef_arc.dstNode = vector_fan_coefs;
        %dst will be further resolved durring bus cleanup
        firBlock.removeIn_arc(coef_arc);
        vector_fan_coefs.addBusArc(coef_arc);
    end

    topology = firNode.dialogProperties('FilterStructure');
    if strcmp(topology, 'Direct form transposed')
        % ---- Direct Form Transposed ----


    elseif strcmp(topology, 'Direct form')
        % ---- Direct Form ----
        delay1_node = GraphNode.createExpandNodeNoSimulinkParams(firBlock, 'Standard', 'Delay', i);
        delay1_node.simulinkBlockType = 'Delay';
        %Currently, only handling numerics
        delay1_node.dialogPropertiesNumeric('InitialCondition') = init_array(i);
        delay1_node.dialogPropertiesNumeric('DelayLength') = 1; %all delay values
        delay1_node.dialogPropertiesNumeric('SampleTime') = -1; % Copy from orig
        delay1_node.dialogProperties('DelayLengthSource') = 'Dialog';
        delay1_node.dialogProperties('InitialConditionSource') = 'Dialog';
        expanded_nodes = [expanded_nodes, delay1_node];

        %Get a template arc
        template_input_arc = copy(input_arc);

        %Connect input wire to delay
        input_arc.dstNode = delay1_node;
        input_arc.dstPortNumber = 1;
        input_arc.appendIntermediateNodeEntry(firBlock, 1, 1, 'Standard', 'In');
        delay1_node.addIn_arc(input_arc)

        %Create new arc
        input_arc2 = copy(template_input_arc);
        srcNode = template_input_arc.srcNode;
        srcNode.addOut_arc(input_arc2);
        srcNode.appendIntermediateNodeEntry(firBlock, 1, 1, 'Standard', 'In');
        new_arcs = [new_arcs, input_arc2]; %Add to new arcs
        %Will set dst in next stage
        
        %Attach arc to delay1
        delay1_arc1 = copy(template_input_arc);
        delay1_arc1.srcNode = delay1_node;
        delay1_arc1.srcPortNumber = 1;
        delay1_node.addOut_arc(delay1_arc1);
        %No intermediate node entry needed since this is all internal
        new_arcs = [new_arcs, delay1_arc1]; %Add to new arcs

        product_input_arcs = [];
        product_input_arcs(1) = input_arc2;
        product_input_arcs(2) = delay1_node;
        
        prev_delay_block = delay1_node;

        %Create the rest of the delays
        for i = 3:coefWidth
            delay_node = GraphNode.createExpandNodeNoSimulinkParams(firBlock, 'Standard', 'Delay', i);
            delay_node.simulinkBlockType = 'Delay';
            %Currently, only handling numerics
            delay_node.dialogPropertiesNumeric('InitialCondition') = init_array(i-1);
            delay_node.dialogPropertiesNumeric('DelayLength') = 1; %all delay values
            delay_node.dialogPropertiesNumeric('SampleTime') = -1; % Copy from orig
            delay_node.dialogProperties('DelayLengthSource') = 'Dialog';
            delay_node.dialogProperties('InitialConditionSource') = 'Dialog';
            expanded_nodes = [expanded_nodes, delay_node];
            
            %Connect to prev delay
            delay_chain_arc = copy(template_input_arc);
            delay_chain_arc.srcNode = prev_delay_block;
            delay_chain_arc.srcPortNumber = 1;
            delay_chain_arc.dstNode = delay_node;
            delay_chain_arc.dstPortNumber = 1;
            prev_delay_block.addOut_arc(delay_chain_arc);
            delay_node.addIn_arc(delay_chain_arc);
            new_arcs = [new_arcs, delay_chain_arc]; %Add to new arcs
            
            %Create new arc
            delay_arc = copy(template_input_arc);
            delay_arc.srcNode = delay_node;
            delay_arc.srcPortNumber = 1;
            delay_node.addOut_arc(delay_arc);
            %No intermediate node entry needed since this is all internal
            new_arcs = [new_arcs, delay_arc]; %Add to new arcs
            
            product_input_arcs(i) = delay_arc;
            prev_delay_block = delay_node;
        end

        product_nodes = [];
        
        %Create Multiply or gain blocks
        for i = 1:coefWidth
            if input_coefs
                %Create multiplies
                product_node = GraphNode.createExpandNodeNoSimulinkParams(firBlock, 'Standard', 'Product', i);
                product_node.simulinkBlockType = 'Product';
                %Currently, only handling numerics
                product_node.dialogProperties('Inputs') = '*';
                product_node.dialogPropertiesNumeric('SampleTime') = -1; % Copy from orig
                if product_use_specific_datatype
                    product_node.dialogProperties('OutDataTypeStr') = product_data_type;
                else
                    if strcmp(product_data_type_str, 'Inherit: Same as input')
                        %Need to change this for product
                        product_node.dialogProperties('OutDataTypeStr') = 'Inherit: Same as first input';
                    else
                        product_node.dialogProperties('OutDataTypeStr') = product_data_type_str;
                    end
                end
                expanded_nodes = [expanded_nodes, delay_node];
                product_nodes(i) = product_node;
                
                %Connect a wire from delay
                product_input_arc = product_input_arcs(i);
                product_input_arc.dstNode = product_node;
                product_input_arc.dstPortNumber = 1;
                product_node.addIn_arc(product_input_arc);
                
                %Add a wire from the vectorfan to the multiply
                coef_wire = copy(coef_arc); %use orig arc as a template
                coef_wire.width = 1;
                coef_wire.dimension = [1, 1];
                coef_wire.dstNode = product_node;
                coef_wire.dstPortNumber = 2;
                coef_wire.appendIntermediateNodeEntry(firBlock, 2, i, 'Standard', 'In');
                %Src will be filled in durring bus cleanup
                new_arcs = [new_arcs, coef_wire];
                product_node.addIn_arc(coef_wire);
                
                vector_fan_coefs.addArc(coef_wire, i);
                
            else
                %Create gains
                product_node = GraphNode.createExpandNodeNoSimulinkParams(firBlock, 'Standard', 'Gain', 1);
                product_node.simulinkBlockType = 'Gain';
                product_node.dialogPropertiesNumeric('Gain') = coef_array(i); % Copy from orig
                product_node.dialogPropertiesNumeric('SampleTime') = -1;
                product_node.dialogProperties('Multiplication') = 'Element-wise(K.*u)';
                if product_use_specific_datatype
                    product_node.dialogProperties('ParamDataTypeStr') = coef_data_type;
                else
                    product_node.dialogProperties('ParamDataTypeStr') = coef_data_type_str;
                end
                if product_use_specific_datatype
                    product_node.dialogProperties('OutDataTypeStr') = product_data_type;
                else
                    product_node.dialogProperties('OutDataTypeStr') = product_data_type_str;
                end
                expanded_nodes = [expanded_nodes, product_node];
                product_nodes(i) = product_node;
                
                %Connect a wire from delay
                product_input_arc = product_input_arcs(i);
                product_input_arc.dstNode = product_node;
                product_input_arc.dstPortNumber = 1;
                product_node.addIn_arc(product_input_arc);
            end
        end
        
        %Add accum cast blocks
        accum_dtc_nodes = [];
        if accum_use_specific_datatype
            for i = 1:coefWidth
                accum_dtc_node = GraphNode.createExpandNodeNoSimulinkParams(firBlock, 'Standard', 'DataTypeConversion', 1);
                accum_dtc_node.simulinkBlockType = 'DataTypeConversion';
                accum_dtc_node.dialogPropertiesNumeric('SampleTime') = -1; % Copy from orig
                accum_dtc_node.dialogProperties('OutDataTypeStr') = accum_data_type;
                expanded_nodes = [expanded_nodes, accum_dtc_node];
                accum_dtc_nodes(i) = accum_dtc_node;

                accum_dtc_arc = GraphArc(product_node, 1, accum_dtc_node, 1, 'Standard');
                accum_dtc_arc.datatype = product_data_type;
                accum_dtc_arc.complex = complex_type;
                accum_dtc_arc.width = 1;
                accum_dtc_arc.dimension = [1, 1];
                new_arcs = [new_arcs, accum_dtc_arc];
            end
        else
            accum_dtc_nodes = product_node;
        end
        
        %Make add node
        sum_node = GraphNode.createExpandNodeNoSimulinkParams(firBlock, 'Standard', 'Sum', 1);
        sum_node.simulinkBlockType = 'Sum';
        sum_str = '';
        for i = 1:coefWidth
            sum_str = [sum_str, '+']
        end
        sum_node.dialogPropertiesNumeric('Inputs') = sum_str;
        sum_node.dialogPropertiesNumeric('SampleTime') = -1;
        sum_node.dialogProperties('OutDataTypeStr') = accum_data_type;
        sum_node.dialogProperties('AccumDataTypeStr') = accum_data_type;
        expanded_nodes = [expanded_nodes, sum_node];
        
        %Attach to add
        for i = 1:coefWidth
            accum_dtc_node = accum_dtc_nodes(i);
            sum_arc = GraphArc(accum_dtc_node, 1, sum_node, i, 'Standard');
            sum_arc.datatype = accum_data_type;
            sum_arc.complex = complex_type;
            sum_arc.width = 1;
            sum_arc.dimension = [1, 1];
            new_arcs = [new_arcs, sum_arc];
        end
        
        %Add out cast block
        out_dtc_block = [];
        if out_use_specific_datatype
            out_dtc_block = GraphNode.createExpandNodeNoSimulinkParams(firBlock, 'Standard', 'DataTypeConversion', 1);
            out_dtc_block.simulinkBlockType = 'DataTypeConversion';
            out_dtc_block.dialogPropertiesNumeric('SampleTime') = -1; % Copy from orig
            out_dtc_block.dialogProperties('OutDataTypeStr') = out_data_type;
            expanded_nodes = [expanded_nodes, out_dtc_block];
            
            %connect out cast
            out_dtc_arc = GraphArc(sum_node, 1, out_dtc_block, 1, 'Standard');
            out_dtc_arc.datatype = accum_data_type;
            out_dtc_arc.complex = complex_type;
            out_dtc_arc.width = 1;
            out_dtc_arc.dimension = [1, 1];
            new_arcs = [new_arcs, out_dtc_arc];
        else
            out_dtc_block = sum_node;
        end
        
        %Connect outputs
        out_arc_to_remove_from_base = [];
        for i = 1:length(firBlock.out_arcs)
            out_arc = firBlock.out_arcs(i);

            %By this point, the output arcs should be of the required datatype,
            %only need to change srcs
            out_arc.srcNode = out_dtc_block;
            out_arc.srcPortNumber = 1;
            out_arc.prependIntermediateNodeEntry(firBlock, 1, 1, 'Standard', 'Out');
            out_dtc_block.addOut_arc(out_arc);

            out_arc_to_remove_from_base = [out_arc_to_remove_from_base, out_arc];
        end

        for i = 1:length(out_arc_to_remove_from_base)
            out_arc = out_arc_to_remove_from_base(i);
            firBlock.removeOut_arc(out_arc);
        end
        
    else
        error('The FIR topology specified is not supported');
    end
end

firBlock.setNodeTypeFromText('Expanded');
end

