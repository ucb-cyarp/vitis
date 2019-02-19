function codegen_cs = CreateStateFlowCodeGenCS(orig_cs)
%CreateCodeGenCS Creates a copy of the given orig_cs with code generation
%option set for Stateflow generation

%Parameters based from setting parameters in 
%Using the method from https://www.mathworks.com/examples/simulink-coder/mw/simulinkcoder-ex86414034-configure-model-from-command-line
%to find the commands to set these parameters by dumping the cs to a file
%(using saveAS) and looking at the section titled 'Code Generation'

codegen_cs = copy(orig_cs);

%From CS (saveAs)
% Original configuration set target is ert.tlc
codegen_cs.switchTarget('ert.tlc','');

codegen_cs.set_param('RemoveResetFunc', 'off');   % Remove reset function
codegen_cs.set_param('ExistingSharedCode', '');   % Existing shared code
codegen_cs.set_param('TargetLang', 'C');   % Language
codegen_cs.set_param('Toolchain', 'Automatically locate an installed toolchain');   % Toolchain
codegen_cs.set_param('BuildConfiguration', 'Faster Runs');   % Build configuration
codegen_cs.set_param('ObjectivePriorities', {'Execution efficiency','RAM efficiency'});   % Prioritized objectives
codegen_cs.set_param('CheckMdlBeforeBuild', 'Off');   % Check model before generating code
codegen_cs.set_param('SILDebugging', 'off');   % Enable source-level debugging for SIL
codegen_cs.set_param('GenCodeOnly', 'off');   % Generate code only
codegen_cs.set_param('PackageGeneratedCodeAndArtifacts', 'off');   % Package code and artifacts
codegen_cs.set_param('RTWVerbose', 'on');   % Verbose build
codegen_cs.set_param('RetainRTWFile', 'off');   % Retain .rtw file
codegen_cs.set_param('ProfileTLC', 'off');   % Profile TLC
codegen_cs.set_param('TLCDebug', 'off');   % Start TLC debugger when generating code
codegen_cs.set_param('TLCCoverage', 'off');   % Start TLC coverage when generating code
codegen_cs.set_param('TLCAssert', 'off');   % Enable TLC assertion
codegen_cs.set_param('RTWUseSimCustomCode', 'on');   % Use the same custom code settings as Simulation Target
codegen_cs.set_param('CustomLAPACKCallback', '');   % Custom LAPACK library callback
codegen_cs.set_param('CustomFFTCallback', '');   % Custom FFT library callback
codegen_cs.set_param('CodeExecutionProfiling', 'off');   % Measure task execution time
codegen_cs.set_param('CodeProfilingInstrumentation', 'off');   % Measure function execution times
codegen_cs.set_param('CodeCoverageSettings', coder.coverage.CodeCoverageSettings([],'off','off','None'));   % Third-party tool
codegen_cs.set_param('CreateSILPILBlock', 'None');   % Create block
codegen_cs.set_param('PortableWordSizes', 'off');   % Enable portable word sizes
codegen_cs.set_param('PostCodeGenCommand', '');   % Post code generation command
codegen_cs.set_param('SaveLog', 'on');   % Save build log
codegen_cs.set_param('TLCOptions', '');   % TLC command line options
codegen_cs.set_param('GenerateReport', 'on');   % Create code generation report
codegen_cs.set_param('LaunchReport', 'off');   % Open report automatically
codegen_cs.set_param('IncludeHyperlinkInReport', 'on');   % Code-to-model
codegen_cs.set_param('GenerateTraceInfo', 'on');   % Model-to-code
codegen_cs.set_param('GenerateWebview', 'off');   % Generate model Web view
codegen_cs.set_param('GenerateTraceReport', 'on');   % Eliminated / virtual blocks
codegen_cs.set_param('GenerateTraceReportSl', 'on');   % Traceable Simulink blocks
codegen_cs.set_param('GenerateTraceReportSf', 'on');   % Traceable Stateflow objects
codegen_cs.set_param('GenerateTraceReportEml', 'on');   % Traceable MATLAB functions
codegen_cs.set_param('GenerateCodeMetricsReport', 'on');   % Static code metrics
codegen_cs.set_param('GenerateCodeReplacementReport', 'on');   % Summarize which blocks triggered code replacements
codegen_cs.set_param('GenerateComments', 'on');   % Include comments
codegen_cs.set_param('SimulinkBlockComments', 'on');   % Simulink block comments
codegen_cs.set_param('StateflowObjectComments', 'off');   % Stateflow object comments
codegen_cs.set_param('MATLABSourceComments', 'off');   % MATLAB source code as comments
codegen_cs.set_param('ShowEliminatedStatement', 'on');   % Show eliminated blocks
codegen_cs.set_param('ForceParamTrailComments', 'on');   % Verbose comments for SimulinkGlobal storage class
codegen_cs.set_param('OperatorAnnotations', 'on');   % Operator annotations
codegen_cs.set_param('InsertBlockDesc', 'on');   % Simulink block descriptions
codegen_cs.set_param('SFDataObjDesc', 'on');   % Stateflow object descriptions
codegen_cs.set_param('SimulinkDataObjDesc', 'on');   % Simulink data object descriptions
codegen_cs.set_param('ReqsInCode', 'off');   % Requirements in block comments
codegen_cs.set_param('EnableCustomComments', 'off');   % Custom comments (MPT objects only)
codegen_cs.set_param('MATLABFcnDesc', 'off');   % MATLAB user comments
codegen_cs.set_param('CustomSymbolStrGlobalVar', '$R$N$M');   % Global variables
codegen_cs.set_param('CustomSymbolStrType', '$N$R$M_T');   % Global types
codegen_cs.set_param('CustomSymbolStrField', '$N$M');   % Field name of global types
codegen_cs.set_param('CustomSymbolStrFcn', '$R$N$M$F');   % Subsystem methods
codegen_cs.set_param('CustomSymbolStrFcnArg', 'rt$I$N$M');   % Subsystem method arguments
codegen_cs.set_param('CustomSymbolStrTmpVar', '$N$M');   % Local temporary variables
codegen_cs.set_param('CustomSymbolStrBlkIO', 'rtb_$N$M');   % Local block output variables
codegen_cs.set_param('CustomSymbolStrMacro', '$R$N$M');   % Constant macros
codegen_cs.set_param('CustomSymbolStrUtil', '$N$C');   % Shared utilities
codegen_cs.set_param('CustomSymbolStrEmxType', 'emxArray_$M$N');   % EMX array types identifier format
codegen_cs.set_param('CustomSymbolStrEmxFcn', 'emx$M$N');   % EMX array utility functions identifier format
codegen_cs.set_param('MangleLength', 1);   % Minimum mangle length
codegen_cs.set_param('SharedChecksumLength', 8);   % Shared checksum length
codegen_cs.set_param('MaxIdLength', 31);   % Maximum identifier length
codegen_cs.set_param('InternalIdentifier', 'Shortened');   % System-generated identifiers
codegen_cs.set_param('InlinedPrmAccess', 'Literals');   % Generate scalar inlined parameters as
codegen_cs.set_param('SignalNamingRule', 'None');   % Signal naming
codegen_cs.set_param('ParamNamingRule', 'None');   % Parameter naming
codegen_cs.set_param('DefineNamingRule', 'None');   % #define naming
codegen_cs.set_param('UseSimReservedNames', 'off');   % Use the same reserved names as Simulation Target
codegen_cs.set_param('ReservedNameArray', []);   % Reserved names
codegen_cs.set_param('IgnoreCustomStorageClasses', 'off');   % Ignore custom storage classes
codegen_cs.set_param('IgnoreTestpoints', 'off');   % Ignore test point signals
codegen_cs.set_param('CommentStyle', 'Auto');   % Comment style
codegen_cs.set_param('InsertPolySpaceComments', 'off');   % Insert Polyspace comments
codegen_cs.set_param('CustomUserTokenString', '');   % Custom token text
codegen_cs.set_param('TargetLangStandard', 'C99 (ISO)');   % Standard math library
codegen_cs.set_param('CodeReplacementLibrary', 'None');   % Code replacement library
codegen_cs.set_param('UtilityFuncGeneration', 'Auto');   % Shared code placement
codegen_cs.set_param('CodeInterfacePackaging', 'Nonreusable function');   % Code interface packaging
codegen_cs.set_param('GRTInterface', 'off');   % Classic call interface
codegen_cs.set_param('PurelyIntegerCode', 'off');   % Support floating-point numbers
codegen_cs.set_param('SupportNonFinite', 'off');   % Support non-finite numbers
codegen_cs.set_param('SupportComplex', 'on');   % Support complex numbers
codegen_cs.set_param('SupportAbsoluteTime', 'on');   % Support absolute time
codegen_cs.set_param('SupportContinuousTime', 'off');   % Support continuous time
codegen_cs.set_param('SupportNonInlinedSFcns', 'off');   % Support non-inlined S-functions
codegen_cs.set_param('SupportVariableSizeSignals', 'off');   % Support variable-size signals
codegen_cs.set_param('MultiwordTypeDef', 'System defined');   % Multiword type definitions
codegen_cs.set_param('CombineOutputUpdateFcns', 'off');   % Single output/update function
codegen_cs.set_param('IncludeMdlTerminateFcn', 'off');   % Terminate function required
codegen_cs.set_param('MatFileLogging', 'off');   % MAT-file logging
codegen_cs.set_param('SuppressErrorStatus', 'on');   % Remove error status field in real-time model data structure
codegen_cs.set_param('CombineSignalStateStructs', 'off');   % Combine signal/state structures
codegen_cs.set_param('ParenthesesLevel', 'Nominal');   % Parentheses level
codegen_cs.set_param('CastingMode', 'Nominal');   % Casting modes
codegen_cs.set_param('GenerateSampleERTMain', 'on');   % Generate an example main program
codegen_cs.set_param('IncludeFileDelimiter', 'Auto');   % #include file delimiter
codegen_cs.set_param('ERTCustomFileBanners', 'on');   % Enable custom file banner
codegen_cs.set_param('GenerateFullHeader', 'on');   % Generate full file banner
codegen_cs.set_param('InferredTypesCompatibility', 'off');   % Create preprocessor directive in rtwtypes.h
codegen_cs.set_param('TargetLibSuffix', '');   % Suffix applied to target library name
codegen_cs.set_param('TargetPreCompLibLocation', '');   % Precompiled library location
codegen_cs.set_param('RemoveDisableFunc', 'off');   % Remove disable function
codegen_cs.set_param('LUTObjectStructOrderExplicitValues', 'Size,Breakpoints,Table');   % LUT object struct order for explicit value specification
codegen_cs.set_param('LUTObjectStructOrderEvenSpacing', 'Size,Breakpoints,Table');   % LUT object struct order for even spacing specification
codegen_cs.set_param('MemSecPackage', '--- None ---');   % Memory sections package for model data and functions
codegen_cs.set_param('MemSecFuncSharedUtil', 'Default');   % Memory section for shared utility functions
codegen_cs.set_param('MemSecFuncInitTerm', 'Default');   % Memory section for initialize/terminate functions
codegen_cs.set_param('MemSecFuncExecute', 'Default');   % Memory section for execution functions
codegen_cs.set_param('MemSecDataParameters', 'Default');   % Memory section for parameters
codegen_cs.set_param('MemSecDataInternal', 'Default');   % Memory section for internal data
codegen_cs.set_param('MemSecDataIO', 'Default');   % Memory section for inputs/outputs
codegen_cs.set_param('MemSecDataConstants', 'Default');   % Memory section for constants
codegen_cs.set_param('GlobalDataDefinition', 'Auto');   % Data definition
codegen_cs.set_param('GlobalDataReference', 'Auto');   % Data declaration
codegen_cs.set_param('ExtMode', 'off');   % External mode
codegen_cs.set_param('ERTFilePackagingFormat', 'Compact');   % File packaging format
codegen_cs.set_param('EnableUserReplacementTypes', 'off');   % Replace data type names in the generated code
codegen_cs.set_param('ConvertIfToSwitch', 'on');   % Convert if-elseif-else patterns to switch-case statements
codegen_cs.set_param('ERTCustomFileTemplate', 'example_file_process.tlc');   % File customization template
codegen_cs.set_param('ERTDataHdrFileTemplate', 'ert_code_template.cgt');   % Header file template
codegen_cs.set_param('ERTDataSrcFileTemplate', 'ert_code_template.cgt');   % Source file template
codegen_cs.set_param('ERTHdrFileBannerTemplate', 'ert_code_template.cgt');   % Header file template
codegen_cs.set_param('ERTSrcFileBannerTemplate', 'ert_code_template.cgt');   % Source file template
codegen_cs.set_param('EnableDataOwnership', 'off');   % Use owner from data object for data definition placement
codegen_cs.set_param('GenerateASAP2', 'off');   % ASAP2 interface
codegen_cs.set_param('IndentSize', '2');   % Indent size
codegen_cs.set_param('IndentStyle', 'K&R');   % Indent style
codegen_cs.set_param('InlinedParameterPlacement', 'Hierarchical');   % Parameter structure
codegen_cs.set_param('ParamTuneLevel', 10);   % Parameter tune level
codegen_cs.set_param('EnableSignedLeftShifts', 'on');   % Replace multiplications by powers of two with signed bitwise shifts
codegen_cs.set_param('EnableSignedRightShifts', 'on');   % Allow right shifts on signed integers
codegen_cs.set_param('PreserveExpressionOrder', 'off');   % Preserve operand order in expression
codegen_cs.set_param('PreserveExternInFcnDecls', 'on');   % Preserve extern keyword in function declarations
codegen_cs.set_param('PreserveStaticInFcnDecls', 'on');   % Preserve static keyword in function declarations
codegen_cs.set_param('PreserveIfCondition', 'off');   % Preserve condition expression in if statement
codegen_cs.set_param('RTWCAPIParams', 'off');   % Generate C API for parameters
codegen_cs.set_param('RTWCAPIRootIO', 'off');   % Generate C API for root-level I/O
codegen_cs.set_param('RTWCAPISignals', 'off');   % Generate C API for signals
codegen_cs.set_param('RTWCAPIStates', 'off');   % Generate C API for states
codegen_cs.set_param('SignalDisplayLevel', 10);   % Signal display level
codegen_cs.set_param('SuppressUnreachableDefaultCases', 'on');   % Suppress generation of default cases for Stateflow switch statements if unreachable
codegen_cs.set_param('TargetOS', 'BareBoardExample');   % Target operating system
codegen_cs.set_param('BooleanTrueId', 'true');   % Boolean true identifier
codegen_cs.set_param('BooleanFalseId', 'false');   % Boolean false identifier
codegen_cs.set_param('MaxIdInt32', 'MAX_int32_T');   % 32-bit integer maximum identifier
codegen_cs.set_param('MinIdInt32', 'MIN_int32_T');   % 32-bit integer minimum identifier
codegen_cs.set_param('MaxIdUint32', 'MAX_uint32_T');   % 32-bit unsigned integer maximum identifier
codegen_cs.set_param('MaxIdInt16', 'MAX_int16_T');   % 16-bit integer maximum identifier
codegen_cs.set_param('MinIdInt16', 'MIN_int16_T');   % 16-bit integer minimum identifier
codegen_cs.set_param('MaxIdUint16', 'MAX_uint16_T');   % 16-bit unsigned integer maximum identifier
codegen_cs.set_param('MaxIdInt8', 'MAX_int8_T');   % 8-bit integer maximum identifier
codegen_cs.set_param('MinIdInt8', 'MIN_int8_T');   % 8-bit integer minimum identifier
codegen_cs.set_param('MaxIdUint8', 'MAX_uint8_T');   % 8-bit unsigned integer maximum identifier
codegen_cs.set_param('TypeLimitIdReplacementHeaderFile', '');   % Type limit identifier replacement header file
end

