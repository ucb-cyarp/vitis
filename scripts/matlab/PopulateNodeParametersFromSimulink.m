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

%Handle Stateflow and Bit Shift which Returns a Type of Subsystem
isStateflow = false;
isBitShift = false;
if strcmp(node.simulinkBlockType, 'SubSystem')
    %Using solution from https://www.mathworks.com/matlabcentral/answers/156628-how-to-recognize-stateflow-blocks-using-simulink-api-get_param
    %did not know about the SFBlockType parameter (I assume this means
    %StateFlowBlockType.
    subsystemType = get_param(simulink_block_handle, 'SFBlockType');
    if strcmp(subsystemType, 'Chart')
        isStateflow = true;
    end
    
    if strcmp( get_param(simulink_block_handle, 'ReferenceBlock'), ['hdlsllib/Logic and Bit' newline 'Operations/Bit Shift'])
        isBitShift = true;
    end
end

%---Get parameters from simulink---
%Get the list of parameter names
dialog_params = get_param(simulink_block_handle, 'DialogParameters');
dialog_param_names = [];
if ~isempty(dialog_params)
    dialog_param_names = fieldnames(dialog_params);
end

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

%---- LUT ----
elseif strcmp(node.simulinkBlockType, 'Lookup_n-D')
    %Check
    if ~strcmp(get_param(simulink_block_handle, 'DataSpecification'), 'Table and breakpoints')
        error('Currently LUT Export Only Supports ''Table and breakpoints'' Specification');
    end
    
    if ~strcmp(get_param(simulink_block_handle, 'BreakpointsSpecification'), 'Explicit values')
        error('Currently LUT Export Only Supports ''Explicit values'' Specification');
    end
    
    numDimensions = GetParamEval(simulink_block_handle, 'NumberOfTableDimensions');
    node.dialogPropertiesNumeric('NumberOfTableDimensions') = numDimensions;
    
    %Get Breakpoint Scales
    for dimension = 1:numDimensions
        breakpointForDimensionName = ['BreakpointsForDimension' num2str(dimension)];
        node.dialogPropertiesNumeric(breakpointForDimensionName) = GetParamEval(simulink_block_handle, breakpointForDimensionName);
    end
    
    %Get Table Data
    node.dialogPropertiesNumeric('Table') = GetParamEval(simulink_block_handle, 'Table');
    
    %Other Important Parameters:
    %DataSpecification = How the LUT Is Specified
    %    Needs to be 'Table and breakpoints'
    %
    %BreakpointsSpecification = How the BreakPoints are Specified
    %    Needs to be 'Explicit values' for now
    %       If First Point and Spacing were to be supported in the future,
    %       BreakpointsForDimension1FirstPoint and
    %       BreakpointsForDimension1Spacing would be needed
    %
    %BreakpointsForDimension1DataTypeStr = DataType of the breakpoints
    %
    %IndexSearchMethod = The Method By Which The LUT is searched
    %    - Evenly spaced points = Only looks at 1st and Last breakpoint and
    %      assumes even spacing -> allows a simple array lookup after 
    %      scaling the input
    %    - Linear search
    %    - Binary search
    %
    %BeginIndexSearchUsingPreviousIndexResult = Specifies if Linear or
    %Binary Search start from the previous result.  If value rarely
    %changes, this can save time.
    %
    %TableDataTypeStr = The DataType of the table data
    %
    %InterpMethod = How to deal with an input between breakpoints
    %    - Flat = take the breakpoint below
    %    - Nearest = take the nearest breakpoint, if equidistent, round up
    %    - Linear = linear interpolation between breakpoints
    %    - Cubic spline = Cubic spline interpolation between breakpoints
    %ExtrapMethod = How to deal with inputs outside of breakpoint range
    %    - Clip = no extrapolation, returns end of range
    %    - Linear = linear extrapolation between outer pair of breakpoints
    %    - Cubic spline = cubic spline extrapolation between outer pair of breakpoints
    %    NOTE: Extrap Method is Clip for Flat or Nearest Inter Method.
    %    HOWEVER will display linear if queried
    
%---- Gain ----
elseif strcmp(node.simulinkBlockType, 'Gain')
    node.dialogPropertiesNumeric('Gain') = GetParamEval(simulink_block_handle, 'Gain');
    node.dialogPropertiesNumeric('SampleTime') = GetParamEval(simulink_block_handle, 'SampleTime');
    
    %Other Important Parameters
    %Multiplication = Type of Multiplication
    %    Only currently support: 'Element-wise(K.*u)'
    %ParamDataTypeStr = The Datatype of the parameter
    %    'Inherit: Inherit via internal rule' = Not quite sure what this
    %        means as Matlab defaults to double but the result of passing
    %        a fixed point input is not double the width.  It looks like it
    %        determines the smallest type which could contain the given
    %        number
    %    'Inherit: Same as input' = the coefficient type is the same as the
    %    input type
    
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
    
%---- Switch ----
elseif strcmp(node.simulinkBlockType, 'Switch')
    %1st and 3rd Ports are inputs.  2nd port is switch
    node.dialogPropertiesNumeric('Threshold') = GetParamEval(simulink_block_handle, 'Threshold');
    node.dialogPropertiesNumeric('SampleTime') = GetParamEval(simulink_block_handle, 'SampleTime');
    %Other important dialog parameters
    %Chriteria - Switching Criteria
    %ZeroCross - Zero Crossing
    
%---- Sum ----
elseif strcmp(node.simulinkBlockType, 'Sum')
    node.dialogPropertiesNumeric('SampleTime') = GetParamEval(simulink_block_handle, 'SampleTime');
    
    %Important DialogParameters
    %Inputs = The string showing what operation each input is (+ or -).  |
    %is used to shift ports around on the GUI but does not take a port
    %number
    %It can also be a number specifying the number of + operations
    %Because it is not always a number, it cannot be reliably parsed into a
    %number
    
    %OutDataTypeStr: output datatype str
    %AccumDataTypeStr: accumulator datatype
    %  A valid type is 'Inherit: Inherit via internal rule'
    
%---- Product ----
elseif strcmp(node.simulinkBlockType, 'Product')
%     if strcmp( get_param(simulink_block_handle, 'Multiplication'), 'Element-wise(.*)')
%         error('Product is currently only supported in Element-wise mode.  Matrix mode is not supported.');
%     end
    
    node.dialogPropertiesNumeric('SampleTime') = GetParamEval(simulink_block_handle, 'SampleTime');
    
    %Important DialogParameters
    %Inputs = Can be a number (indeicating the number of multiply inputs).
    %         Can also be a string of '*' and '/'s to indicate the operations on each port
    %Because it is not always a number, it cannot be reliably parsed into a
    %number
    
    %Multiplication: Indicates if multiplication is element wise or matrix
    %   - 'Element-wise(.*)' : Element Wise
    %   - 'Matrix(*)'        : Matrix
    
    %OutDataTypeStr: output datatype str
    %  A valid type is 'Inherit: Inherit via internal rule'
  
%---- FIR ----
elseif strcmp(node.simulinkBlockType, 'DiscreteFir')
    if strcmp( get_param(simulink_block_handle, 'CoefSource'), 'Dialog parameters')
        %Only parse if dialog box is used for value
        node.dialogPropertiesNumeric('Coefficients') = GetParamEval(simulink_block_handle, 'Coefficients');
    end
    
    node.dialogPropertiesNumeric('InitialStates') = GetParamEval(simulink_block_handle, 'InitialStates');
    
    node.dialogPropertiesNumeric('SampleTime') = GetParamEval(simulink_block_handle, 'SampleTime');
    
    %Other important parameters:
    %FilterStructure = The implementation structure of the FIR filter
    %    'Direct form' = Direct Form
    %    'Direct form transposed' = Direct form Transposed
    %TapSumDataTypeStr
    %CoefDataTypeStr
    %ProductDataTypeStr
    %AccumDataTypeStr
    %StateDataTypeStr
    %OutDataTypeStr
    
%---- Tapped Delay ----
%Note: block type is S-Function
elseif strcmp( get_param(simulink_block_handle, 'ReferenceBlock'), 'simulink/Discrete/Tapped Delay' ) || strcmp( get_param(simulink_block_handle, 'ReferenceBlock'), 'hdlsllib/Discrete/Tapped Delay' )
        node.dialogPropertiesNumeric('vinit') = GetParamEval(simulink_block_handle, 'vinit');
        
        node.dialogPropertiesNumeric('NumDelays') = GetParamEval(simulink_block_handle, 'NumDelays');
        
        node.dialogPropertiesNumeric('samptime') = GetParamEval(simulink_block_handle, 'samptime');
        
        %Changing block type from 'S-Function'
        node.simulinkBlockType = 'TappedDelay';
        
        %Other Parameters: 
        %DelayOrder = the order in which delays are concattenated.
        %    Oldest = the result of the last delay in the chain
        %    is the first index in the exported vector
        %    Newest = the result of the first delay in the chain is the
        %    first index in the exported vector
        %
        %includeCurrent = if 'on', include the current input in the output
        %    vector
    
%---- Selector ----
elseif strcmp(node.simulinkBlockType, 'Selector')
    %Selects from a matrix/vector
    node.dialogPropertiesNumeric('NumberOfDimensions') = GetParamEval(simulink_block_handle, 'NumberOfDimensions');
    
    if node.dialogPropertiesNumeric('NumberOfDimensions') ~= 1
        error('Currently only support 1D array wires (vectors).  Selector is set to a dimension != 1');
    end
    
    index_mode = get_param(simulink_block_handle, 'IndexMode');
    
    if iscell(index_mode)
        index_mode = index_mode{1};
    end
    
    if strcmp(index_mode, 'One-based')
        node.dialogPropertiesNumeric('index_mode') = 1;
    elseif strcmp(index_mode, 'Zero-based')
        node.dialogPropertiesNumeric('index_mode') = 0;
    else
        error('Unexpected IndexMode for Selector');
    end
    
    index_options = get_param(simulink_block_handle, 'IndexOptionArray');
    
    if ~strcmp(index_options{1}, 'Index vector (dialog)') && ~strcmp(index_options{1}, 'Index vector (port)') && ~strcmp(index_options{1}, '') && ~strcmp(index_options{1}, 'Starting index (dialog)') && ~strcmp(index_options{1}, 'Starting index (port)')
        error('Seletor is currently only supported when the Index Option is set to ''Index vector (dialog)'', ''Starting index (port)'', or ''Starting index (dialog)''');
    end
    
    %This is where we get the index vector when the index_options is 'Index vector (dialog)'
    index_params = GetParamEval(simulink_block_handle, 'IndexParamArray');
    node.dialogPropertiesNumeric('IndexParamArray') = index_params;
    
    %This is where we get the output vector width (starting index comes from the port) when the index_options is 'Starting index (port)'
    output_size_array = GetParamEval(simulink_block_handle, 'OutputSizeArray');
    node.dialogPropertiesNumeric('OutputSizeArray') = output_size_array;
    
    %Get the 1st dimension
    node.dialogPropertiesNumeric('IndexParam1st') = index_params{1};
    node.dialogProperties('IndexOption1st') = index_options{1};
    node.dialogPropertiesNumeric('OutputSize1st') = output_size_array{1};
    
    node.dialogPropertiesNumeric('InputPortWidth') = GetParamEval(simulink_block_handle, 'InputPortWidth');
    
    node.dialogPropertiesNumeric('SampleTime') = GetParamEval(simulink_block_handle, 'SampleTime');

%---- Logical Operator ----
elseif strcmp(node.simulinkBlockType, 'Logic')
    node.dialogPropertiesNumeric('Inputs') = GetParamEval(simulink_block_handle, 'Inputs');

    node.dialogPropertiesNumeric('SampleTime') = GetParamEval(simulink_block_handle, 'SampleTime');

    node.simulinkBlockType = 'Logic';

%---- Saturate ----
elseif strcmp(node.simulinkBlockType, 'Saturate' )
    node.dialogPropertiesNumeric('UpperLimit') = GetParamEval(simulink_block_handle, 'UpperLimit');
    node.dialogPropertiesNumeric('LowerLimit') = GetParamEval(simulink_block_handle, 'LowerLimit');

    node.dialogPropertiesNumeric('SampleTime') = GetParamEval(simulink_block_handle, 'SampleTime');

%---- Data Type Propagation (Unsupported but Changing Type) ----
elseif strcmp( get_param(simulink_block_handle, 'ReferenceBlock'), ['simulink/Signal' newline 'Attributes/Data Type' newline 'Propagation'])
        %Changing block type from 'S-Function'
        node.simulinkBlockType = 'DataTypePropagation';

%---- Output Ports ----
elseif strcmp(node.simulinkBlockType, 'Outport' )
    node.dialogPropertiesNumeric('InitialOutput') = GetParamEval(simulink_block_handle, 'InitialOutput');
        
%---- Stateflow ----
elseif isStateflow
    %Getting generated C files occures at a later stage.  Here, need to get
    %port names
    outports = find_system(node.getFullSimulinkPath(),  'FollowLinks', 'on', 'LoadFullyIfNeeded', 'on', 'LookUnderMasks', 'on', 'SearchDepth', 1, 'BlockType', 'Outport');

    for i = 1:length(outports)
        outport = outports(i);

        %Get the port number of the inport, this will be used to distinguish
        %the different inputs.
        port_number_str = get_param(outport, 'Port'); %Returned as a string
        port_number = str2double(port_number_str);

        %Get port name and create entry in Output Master Node
        port_name = get_param(outport, 'Name');
        node.outputPorts{port_number} = port_name{1}; 
    end
    
    inports = find_system(node.getFullSimulinkPath(),  'FollowLinks', 'on', 'LoadFullyIfNeeded', 'on', 'LookUnderMasks', 'on', 'SearchDepth', 1, 'BlockType', 'Inport');

    for i = 1:length(inports)
        inport = inports(i);

        %Get the port number of the inport, this will be used to distinguish
        %the different inputs.
        port_number_str = get_param(inport, 'Port'); %Returned as a string
        port_number = str2double(port_number_str);

        %Get port name and create entry in Output Master Node
        port_name = get_param(inport, 'Name');
        node.inputPorts{port_number} = port_name{1}; 
    end
    
%---- NCO ----
elseif strcmp( get_param(simulink_block_handle, 'ReferenceBlock'), 'dspsigops/NCO' )
    node.dialogPropertiesNumeric('TableDepth') = GetParamEval(simulink_block_handle, 'TableDepth'); %The size of the full wave table address (in bits).  1 period of a sin wave is split into 2^TableDepth points
    %If using a quarter wave table, the number of entries in the table is
    %(2^TableDepth)/4
    
    node.dialogPropertiesNumeric('AccumWL') = GetParamEval(simulink_block_handle, 'AccumWL'); %The accumulator 
    node.dialogPropertiesNumeric('DitherWL') = GetParamEval(simulink_block_handle, 'DitherWL');
    
    %HasDither = 'off' or 'on'.  If 'off', ignore DitherWL
    
    %Formula is 'Complex exponential', 'Sine', 'Cosine', or 'Sine and cosine'

    %Changing block type from 'S-Function'
    node.simulinkBlockType = 'NCO';
    
%---- BPSK Modulator ----
elseif strcmp( get_param(simulink_block_handle, 'ReferenceBlock'), ['commdigbbndpm3/BPSK' newline 'Modulator' newline 'Baseband'])
    %Changing block type from 'S-Function'
    node.simulinkBlockType = 'BPSK_ModulatorBaseband';
    node.dialogPropertiesNumeric('Ph') = GetParamEval(simulink_block_handle, 'Ph');
    % Phase offset (from point on horizontal)
    % 0 -> 1 + 0j, 1 -> -1 + 0j
    
%---- QPSK Modulator ----
elseif strcmp( get_param(simulink_block_handle, 'ReferenceBlock'), ['commdigbbndpm3/QPSK' newline 'Modulator' newline 'Baseband'])
    %Changing block type from 'S-Function'
    node.simulinkBlockType = 'QPSK_ModulatorBaseband';
    
    node.dialogPropertiesNumeric('Ph') = GetParamEval(simulink_block_handle, 'Ph');
    %Phase offset of pi/4 has points off the axes (the standard way we view 4QAM)
    %Grey, pi/4
    %0 ->  sqrt(2)/2 + j*sqrt(2)/2
    %1 -> -sqrt(2)/2 + j*sqrt(2)/2
    %2 ->  sqrt(2)/2 - j*sqrt(2)/2
    %3 -> -sqrt(2)/2 - j*sqrt(2)/2
    
    %Enc is either "Gray" or "Binary"
    
%---- M-QAM Modulator ----
elseif strcmp( get_param(simulink_block_handle, 'ReferenceBlock'), ['commdigbbndam3/Rectangular QAM' newline 'Modulator' newline 'Baseband'])
    %Changing block type from 'S-Function'
    node.simulinkBlockType = 'RectangularQAM_ModulatorBaseband';
    node.dialogPropertiesNumeric('M') = GetParamEval(simulink_block_handle, 'M'); % M-ary QAM
    
    node.dialogPropertiesNumeric('Ph') = GetParamEval(simulink_block_handle, 'Ph'); %The phase rotation.  A phase rotation of 0 has the constellation in a grid oriented long the axes
    
    %Enc is either "Gray" or "Binary"
    
    %PowType is the normalization method used for scalaing the constellation
    %"Average Power", "Min. distance between symbols", "Peak Power"
    
    node.dialogPropertiesNumeric('MinDist') = GetParamEval(simulink_block_handle, 'MinDist'); % The min distance if "Min. distance between symbols" is selected for PowType
    node.dialogPropertiesNumeric('AvgPow') = GetParamEval(simulink_block_handle, 'AvgPow'); % The avg power if "Average Power" is selected for PowType (referenced to 1 ohm (watts))
    node.dialogPropertiesNumeric('PeakPow') = GetParamEval(simulink_block_handle, 'PeakPow'); % The peak power if "Peak Power" is selected for PowType (referenced to 1 ohm (watts))

%---- BPSK Demodulator ----
elseif strcmp( get_param(simulink_block_handle, 'ReferenceBlock'), ['commdigbbndpm3/BPSK' newline 'Demodulator' newline 'Baseband'])
    %Changing block type from 'S-Function'
    node.simulinkBlockType = 'BPSK_DemodulatorBaseband';
    node.dialogPropertiesNumeric('Ph') = GetParamEval(simulink_block_handle, 'Ph');
    % Phase offset (from point on horizontal)
    % 0 -> 1 + 0j, 1 -> -1 + 0j
    
    %DecType is the type of decoder and can be "Hard decision",
    %"Log-likelihood ratio", or "Approximate log-likelihood ratio"
    
%---- QPSK Demodulator ----
elseif strcmp( get_param(simulink_block_handle, 'ReferenceBlock'), ['commdigbbndpm3/QPSK' newline 'Demodulator' newline 'Baseband'])
    %Changing block type from 'S-Function'
    node.simulinkBlockType = 'QPSK_DemodulatorBaseband';
    
    node.dialogPropertiesNumeric('Ph') = GetParamEval(simulink_block_handle, 'Ph');
    %Phase offset of pi/4 has points off the axes (the standard way we view 4QAM)
    %Grey, pi/4
    %0 ->  sqrt(2)/2 + j*sqrt(2)/2
    %1 -> -sqrt(2)/2 + j*sqrt(2)/2
    %2 ->  sqrt(2)/2 - j*sqrt(2)/2
    %3 -> -sqrt(2)/2 - j*sqrt(2)/2
    
    %Dec is either "Gray" or "Binary"
    
    %OutType is the type of output with is either "Integer" or "Bit"
    
    %DecType is only relevent for OutType "Bit" and is the type of decoder 
    %which can be "Hard decision", "Log-likelihood ratio", or 
    %"Approximate log-likelihood ratio".  For OutType "Bit" it is a hard
    %decision decoder
    
%---- M-QAM Demodulator ----
elseif strcmp( get_param(simulink_block_handle, 'ReferenceBlock'), ['commdigbbndam3/Rectangular QAM' newline 'Demodulator' newline 'Baseband'])
    %Changing block type from 'S-Function'
    node.simulinkBlockType = 'RectangularQAM_DemodulatorBaseband';
    node.dialogPropertiesNumeric('M') = GetParamEval(simulink_block_handle, 'M'); % M-ary QAM
    
    node.dialogPropertiesNumeric('Ph') = GetParamEval(simulink_block_handle, 'Ph'); %The phase rotation.  A phase rotation of 0 has the constellation in a grid oriented long the axes
    
    %Dec is either "Gray" or "Binary"
    
    %PowType is the normalization method used for scalaing the constellation
    %"Average Power", "Min. distance between symbols", "Peak Power"
    
    node.dialogPropertiesNumeric('MinDist') = GetParamEval(simulink_block_handle, 'MinDist'); % The min distance if "Min. distance between symbols" is selected for PowType
    node.dialogPropertiesNumeric('AvgPow') = GetParamEval(simulink_block_handle, 'AvgPow'); % The avg power if "Average Power" is selected for PowType (referenced to 1 ohm (watts))
    node.dialogPropertiesNumeric('PeakPow') = GetParamEval(simulink_block_handle, 'PeakPow'); % The peak power if "Peak Power" is selected for PowType (referenced to 1 ohm (watts))
    
    %OutType is the type of output with is either "Integer" or "Bit"
    
    %DecType is only relevent for OutType "Bit" and is the type of decoder 
    %which can be "Hard decision", "Log-likelihood ratio", or 
    %"Approximate log-likelihood ratio".  For OutType "Bit" it is a hard
    %decision decoder
    
%---- Bitwise Operator ----
elseif strcmp( get_param(simulink_block_handle, 'ReferenceBlock'), ['simulink/Logic and Bit' newline 'Operations/Bitwise' newline 'Operator'])
        %Changing block type from 'S-Function'
        node.simulinkBlockType = 'BitwiseOperator';
        
        %logicop = 'AND', 'OR', 'NAND', 'NOR', 'XOR', 'NOT'
        
        %Use UseBitMask = 'off' or 'on'.  Specifies if the mask is provided
        %as a dialog parameter
        
        node.dialogPropertiesNumeric('NumInputPorts') = GetParamEval(simulink_block_handle, 'NumInputPorts'); %Number of input ports
        
        node.dialogPropertiesNumeric('BitMask') = GetParamEval(simulink_block_handle, 'BitMask'); %Bitmask if given in dialog
        
        %BitMaskRealWorld not quite sure what this is.  'Stored Integer' is
        %an option.
       
%---- Bit Shift ----
elseif isBitShift
        %Changing block type from Subsystem
        node.simulinkBlockType = 'BitShift';
        
        %mode = 'Shift Left Logical'
        %       'Shift Right Logical'
        %       'Shift Right Arithmetic'
        
        node.dialogPropertiesNumeric('N') = GetParamEval(simulink_block_handle, 'N'); %Shift amount
        
%---- Trigonometry Operator ----
elseif strcmp( get_param(simulink_block_handle, 'BlockType'), 'Trigonometry')
        %Changing block type from 'S-Function'
        node.simulinkBlockType = 'Trigonometry';
        
        %node.dialogPropertiesNumeric('Operator') = GetParamEval(simulink_block_handle, 'Operator'); %The operator (sin, cos, atan, atan2, ...)
        %atan2, first port is y, second port is x
        %node.dialogPropertiesNumeric('ApproximationMethod') = GetParamEval(simulink_block_handle, 'ApproximationMethod'); %Approximation method (None, or CORDIC)
        %node.dialogPropertiesNumeric('NumberOfIterations') = GetParamEval(simulink_block_handle, 'NumberOfIterations'); %Number of iterations if CORDIC used  
        
%---- Math Operator ----
elseif strcmp( get_param(simulink_block_handle, 'BlockType'), 'Math')
        %Changing block type from 'S-Function'
        node.simulinkBlockType = 'Math';
        
        %node.dialogPropertiesNumeric('Operator') = GetParam(simulink_block_handle, 'Operator'); %The operator (log, exp, log10, ...)

%##### Rate Change Blocks #####
%---- Upsample ----
elseif strcmp( get_param(simulink_block_handle, 'ReferenceBlock'), 'dspsigops/Upsample')
    %Change block type from 'S-Function'
    node.simulinkBlockType = 'UpSample'; %CammelCased to match downsample
    
    node.dialogPropertiesNumeric('N') = GetParamEval(simulink_block_handle, 'N'); %This is the amount of upsampling (L in the dialog)
    node.dialogPropertiesNumeric('phase') = GetParamEval(simulink_block_handle, 'phase'); %This is the phase offset (sample offset).  Can be 0 to N-1
    node.dialogPropertiesNumeric('ic') = GetParamEval(simulink_block_handle, 'ic'); %These are the initial conditions for the block (presumably when the phase is not 0)
    
    %RateOptions contains the "Rate options" entry which is greyed out and
    %is set to: 'Allow multirate processing'
    
    %InputProcessing contains the "Input processing" entry which is either
    %'Elements as channels (sample based)' or 'Columns as channels 
    %(frame based)'.  This has an impact if the input is a matrix type.
    
    %Note that the phase change could also be accomplished by inserting
    %delays after the rate change with the given initial conditions.
    
%---- Downsample ----
elseif strcmp( get_param(simulink_block_handle, 'BlockType'), 'DownSample')
    %Interestingly, this is not an S-Function like Upsample is
    
    node.dialogPropertiesNumeric('N') = GetParamEval(simulink_block_handle, 'N'); %This is the amount of downsampling (K in the dialog)
    node.dialogPropertiesNumeric('phase') = GetParamEval(simulink_block_handle, 'phase'); %This is the phase offset (sample offset).  Can be 0 to N-1
    node.dialogPropertiesNumeric('ic') = GetParamEval(simulink_block_handle, 'ic'); %These are the initial conditions for the block (presumably when the phase is not 0)
    
    %RateOptions contains the "Rate options" entry which not greyed out (compare to Upsample)
    %and can be 'Allow multirate processing' or 'Enforce single-rate processing'
    
    %InputProcessing contains the "Input processing" entry which is either
    %'Elements as channels (sample based)' or 'Columns as channels 
    %(frame based)'.  This has an impact if the input is a matrix type.
    
%---- Repeat ----
elseif strcmp( get_param(simulink_block_handle, 'ReferenceBlock'), 'dspsigops/Repeat')
    %Change block type from 'S-Function'
    node.simulinkBlockType = 'Repeat';
    
    node.dialogPropertiesNumeric('N') = GetParamEval(simulink_block_handle, 'N'); %This is the amount of upsampling (Repetition count in the dialog)
    node.dialogPropertiesNumeric('ic') = GetParamEval(simulink_block_handle, 'ic'); %These are the initial conditions for the block (this is odd since there is no phase parameter)

    %There is no phase parameter for this block.
    %Note that the phase change could also be accomplished by inserting
    %delays after the rate change with the given initial conditions.
    
    %RateOptions contains the "Rate options" entry which not greyed out (compare to Upsample)
    %and can be 'Allow multirate processing' or 'Enforce single-rate processing'
    
    %InputProcessing contains the "Input processing" entry which is either
    %'Elements as channels (sample based)' or 'Columns as channels 
    %(frame based)'.  This has an impact if the input is a matrix type.

%---- Concatenate -----
elseif strcmp( get_param(simulink_block_handle, 'BlockType'), 'Concatenate')
    node.dialogPropertiesNumeric('NumInputs') = GetParamEval(simulink_block_handle, 'NumInputs'); %The number of inputs to concatenate together
    node.dialogPropertiesNumeric('ConcatenateDimension') = GetParamEval(simulink_block_handle, 'ConcatenateDimension'); %The dimension to concatinate allong (for multidimensional concatenate)
    
    %Mode: The concatenate mode
    %    - 'Vector' - Vector Concatenate (ignores ConcatenateDimension)
    %    - 'Multidimensional array' - Multidimensional Array (requires ConcatenateDimension)
    
%TODO: More Blocks
end
    
end

