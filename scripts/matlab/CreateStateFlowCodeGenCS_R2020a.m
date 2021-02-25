function codegen_cs = CreateStateFlowCodeGenCS_R2020a(orig_cs, id)
%CreateCodeGenCS Creates a copy of the given orig_cs with code generation
%option set for Stateflow generation

%Parameters based from setting parameters in 
%Using the method from https://www.mathworks.com/examples/simulink-coder/mw/simulinkcoder-ex86414034-configure-model-from-command-line
%to find the commands to set these parameters by dumping the cs to a file
%(using saveAS) and looking at the section titled 'Code Generation'

codegen_cs = copy(orig_cs);

%From CS (saveAs)
%From Matlab R2020a

% Do not change the order of the following commands. There are dependencies between the parameters.
% Original configuration set target is ert.tlc
codegen_cs.switchTarget('ert.tlc','');

codegen_cs.set_param('HardwareBoard', 'None');   % Hardware board

codegen_cs.set_param('TargetLang', 'C');   % Language

codegen_cs.set_param('CodeInterfacePackaging', 'Reusable function');   % Code interface packaging

codegen_cs.set_param('GenerateAllocFcn', 'off');   % Use dynamic memory allocation for model initialization

codegen_cs.set_param('Solver', 'FixedStepAuto');   % Solver

% Solver
codegen_cs.set_param('StartTime', '0.0');   % Start time
codegen_cs.set_param('StopTime', '((((preLen+100+dataLenSymbols)*1+pad_first*3)*basePer))*2.5');   % Stop time
codegen_cs.set_param('SampleTimeConstraint', 'Unconstrained');   % Periodic sample time constraint
codegen_cs.set_param('SolverType', 'Fixed-step');   % Type
codegen_cs.set_param('ConcurrentTasks', 'off');   % Allow tasks to execute concurrently on target
codegen_cs.set_param('FixedStep', 'auto');   % Fixed-step size (fundamental sample time)
codegen_cs.set_param('EnableMultiTasking', 'off');   % Treat each discrete rate as a separate task
codegen_cs.set_param('PositivePriorityOrder', 'off');   % Higher priority value indicates higher task priority
codegen_cs.set_param('AutoInsertRateTranBlk', 'off');   % Automatically handle rate transition for data transfer

% Data Import/Export
codegen_cs.set_param('Decimation', '1');   % Decimation
codegen_cs.set_param('LoadExternalInput', 'off');   % Load external input
codegen_cs.set_param('SaveFinalState', 'off');   % Save final state
codegen_cs.set_param('LoadInitialState', 'off');   % Load initial state
codegen_cs.set_param('LimitDataPoints', 'off');   % Limit data points
codegen_cs.set_param('SaveFormat', 'Dataset');   % Format
codegen_cs.set_param('SaveOutput', 'on');   % Save output
codegen_cs.set_param('SaveState', 'off');   % Save states
codegen_cs.set_param('SignalLogging', 'on');   % Signal logging
codegen_cs.set_param('DSMLogging', 'on');   % Data stores
codegen_cs.set_param('InspectSignalLogs', 'off');   % Record logged workspace data in Simulation Data Inspector
codegen_cs.set_param('SaveTime', 'on');   % Save time
codegen_cs.set_param('ReturnWorkspaceOutputs', 'on');   % Single simulation output
codegen_cs.set_param('TimeSaveName', 'tout');   % Time variable
codegen_cs.set_param('OutputSaveName', 'yout');   % Output variable
codegen_cs.set_param('SignalLoggingName', 'logsout');   % Signal logging name
codegen_cs.set_param('DSMLoggingName', 'dsmout');   % Data stores logging name
codegen_cs.set_param('ReturnWorkspaceOutputsName', 'out');   % Simulation output variable
codegen_cs.set_param('LoggingToFile', 'off');   % Log Dataset data to file
codegen_cs.set_param('DatasetSignalFormat', 'timeseries');   % Dataset signal format
codegen_cs.set_param('LoggingIntervals', '[-inf, inf]');   % Logging intervals

% Optimization
codegen_cs.set_param('BlockReduction', 'on');   % Block reduction
codegen_cs.set_param('BooleanDataType', 'on');   % Implement logic signals as Boolean data (vs. double)
codegen_cs.set_param('ConditionallyExecuteInputs', 'on');   % Conditional input branch execution
codegen_cs.set_param('DefaultParameterBehavior', 'Inlined');   % Default parameter behavior
codegen_cs.set_param('UseDivisionForNetSlopeComputation', 'off');   % Use division for fixed-point net slope computation
codegen_cs.set_param('GainParamInheritBuiltInType', 'off');   % Gain parameters inherit a built-in integer type that is lossless
codegen_cs.set_param('UseFloatMulNetSlope', 'off');   % Use floating-point multiplication to handle net slope corrections
codegen_cs.set_param('DefaultUnderspecifiedDataType', 'double');   % Default for underspecified data type
codegen_cs.set_param('UseSpecifiedMinMax', 'off');   % Optimize using the specified minimum and maximum values
codegen_cs.set_param('InlineInvariantSignals', 'on');   % Inline invariant signals
codegen_cs.set_param('OptimizationCustomize', 'off');   % Specify custom optimizations
% Set objective for Speed instead of Speed & RAM ballance
codegen_cs.set_param('OptimizationPriority', 'Speed');   % Priority 
codegen_cs.set_param('OptimizationLevel', 'level2');   % Level
codegen_cs.set_param('DataBitsets', 'off');   % Use bitsets for storing Boolean data
codegen_cs.set_param('StateBitsets', 'off');   % Use bitsets for storing state configuration
codegen_cs.set_param('LocalBlockOutputs', 'on');   % Enable local block outputs
codegen_cs.set_param('EnableMemcpy', 'on');   % Use memcpy for vector assignment
codegen_cs.set_param('ExpressionFolding', 'on');   % Eliminate superfluous local variables (expression folding)
codegen_cs.set_param('BufferReuse', 'on');   % Reuse local block outputs
codegen_cs.set_param('OptimizeBlockIOStorage', 'on');   % Signal storage reuse
codegen_cs.set_param('AdvancedOptControl', '');   % Disable incompatible optimizations
codegen_cs.set_param('BitwiseOrLogicalOp', 'Same as modeled');   % Operator to represent Bitwise and Logical Operator blocks
codegen_cs.set_param('MemcpyThreshold', 64);   % Memcpy threshold (bytes)
codegen_cs.set_param('PassReuseOutputArgsAs', 'Individual arguments');   % Pass reusable subsystem outputs as
codegen_cs.set_param('PassReuseOutputArgsThreshold', 12);   % Maximum number of arguments for subsystem outputs
codegen_cs.set_param('RollThreshold', 5);   % Loop unrolling threshold
codegen_cs.set_param('ActiveStateOutputEnumStorageType', 'Native Integer');   % Base storage type for automatically created enumerations
codegen_cs.set_param('ZeroExternalMemoryAtStartup', 'on');   % Remove root level I/O zero initialization
codegen_cs.set_param('ZeroInternalMemoryAtStartup', 'on');   % Remove internal data zero initialization
codegen_cs.set_param('InitFltsAndDblsToZero', 'off');   % Use memset to initialize floats and doubles to 0.0
% Do not add some wrapping logic to the generated code - make sure any wrapping logic is handled as part of the FSM
codegen_cs.set_param('NoFixptDivByZeroProtection', 'on');   % Remove code that protects against division arithmetic exceptions
codegen_cs.set_param('EfficientFloat2IntCast', 'on');   % Remove code from floating-point to integer conversions that wraps out-of-range values
codegen_cs.set_param('EfficientMapNaN2IntZero', 'on');   % Remove code from floating-point to integer conversions with saturation that maps NaN to zero
codegen_cs.set_param('LifeSpan', 'auto');   % Application lifespan (days)
codegen_cs.set_param('MaxStackSize', 'Inherit from target');   % Maximum stack size (bytes)
codegen_cs.set_param('BufferReusableBoundary', 'on');   % Buffer for reusable subsystems
codegen_cs.set_param('SimCompilerOptimization', 'off');   % Compiler optimization level
codegen_cs.set_param('AccelVerboseBuild', 'off');   % Verbose accelerator builds
codegen_cs.set_param('UseRowMajorAlgorithm', 'off');   % Use algorithms optimized for row-major array layout
codegen_cs.set_param('LabelGuidedReuse', 'off');   % Use signal labels to guide buffer reuse
codegen_cs.set_param('DenormalBehavior', 'GradualUnderflow');   % In accelerated simulation modes, denormal numbers can be flushed to zero using the 'flush-to-zero' option.
codegen_cs.set_param('EfficientTunableParamExpr', 'on');   % Remove code from tunable parameter expressions that saturates out-of-range values

% Diagnostics
codegen_cs.set_param('RTPrefix', 'error');   % "rt" prefix for identifiers
codegen_cs.set_param('ConsistencyChecking', 'none');   % Solver data inconsistency
codegen_cs.set_param('ArrayBoundsChecking', 'none');   % Array bounds exceeded
codegen_cs.set_param('SignalInfNanChecking', 'none');   % Inf or NaN block output
codegen_cs.set_param('StringTruncationChecking', 'error');   % String truncation checking
codegen_cs.set_param('SignalRangeChecking', 'none');   % Simulation range checking
codegen_cs.set_param('ReadBeforeWriteMsg', 'UseLocalSettings');   % Detect read before write
codegen_cs.set_param('WriteAfterWriteMsg', 'UseLocalSettings');   % Detect write after write
codegen_cs.set_param('WriteAfterReadMsg', 'UseLocalSettings');   % Detect write after read
codegen_cs.set_param('AlgebraicLoopMsg', 'warning');   % Algebraic loop
codegen_cs.set_param('ArtificialAlgebraicLoopMsg', 'warning');   % Minimize algebraic loop
codegen_cs.set_param('SaveWithDisabledLinksMsg', 'warning');   % Block diagram contains disabled library links
codegen_cs.set_param('SaveWithParameterizedLinksMsg', 'warning');   % Block diagram contains parameterized library links
codegen_cs.set_param('UnderspecifiedInitializationDetection', 'Simplified');   % Underspecified initialization detection
codegen_cs.set_param('MergeDetectMultiDrivingBlocksExec', 'error');   % Detect multiple driving blocks executing at the same time step
codegen_cs.set_param('SignalResolutionControl', 'UseLocalSettings');   % Signal resolution
codegen_cs.set_param('BlockPriorityViolationMsg', 'warning');   % Block priority violation
codegen_cs.set_param('MinStepSizeMsg', 'warning');   % Min step size violation
codegen_cs.set_param('TimeAdjustmentMsg', 'none');   % Sample hit time adjusting
codegen_cs.set_param('MaxConsecutiveZCsMsg', 'error');   % Consecutive zero crossings violation
codegen_cs.set_param('MaskedZcDiagnostic', 'warning');   % Masked zero crossings
codegen_cs.set_param('IgnoredZcDiagnostic', 'warning');   % Ignored zero crossings
codegen_cs.set_param('SolverPrmCheckMsg', 'none');   % Automatic solver parameter selection
codegen_cs.set_param('InheritedTsInSrcMsg', 'warning');   % Source block specifies -1 sample time
codegen_cs.set_param('MultiTaskDSMMsg', 'error');   % Multitask data store
codegen_cs.set_param('MultiTaskCondExecSysMsg', 'error');   % Multitask conditionally executed subsystem
codegen_cs.set_param('MultiTaskRateTransMsg', 'error');   % Multitask rate transition
codegen_cs.set_param('SingleTaskRateTransMsg', 'none');   % Single task rate transition
codegen_cs.set_param('TasksWithSamePriorityMsg', 'warning');   % Tasks with equal priority
codegen_cs.set_param('SigSpecEnsureSampleTimeMsg', 'warning');   % Enforce sample times specified by Signal Specification blocks
codegen_cs.set_param('CheckMatrixSingularityMsg', 'none');   % Division by singular matrix
codegen_cs.set_param('IntegerOverflowMsg', 'warning');   % Wrap on overflow
codegen_cs.set_param('Int32ToFloatConvMsg', 'warning');   % 32-bit integer to single precision float conversion
codegen_cs.set_param('ParameterDowncastMsg', 'error');   % Detect downcast
codegen_cs.set_param('ParameterOverflowMsg', 'error');   % Detect overflow
codegen_cs.set_param('ParameterUnderflowMsg', 'none');   % Detect underflow
codegen_cs.set_param('ParameterPrecisionLossMsg', 'warning');   % Detect precision loss
codegen_cs.set_param('ParameterTunabilityLossMsg', 'error');   % Detect loss of tunability
codegen_cs.set_param('FixptConstUnderflowMsg', 'none');   % Detect underflow
codegen_cs.set_param('FixptConstOverflowMsg', 'none');   % Detect overflow
codegen_cs.set_param('FixptConstPrecisionLossMsg', 'none');   % Detect precision loss
codegen_cs.set_param('UnderSpecifiedDataTypeMsg', 'none');   % Underspecified data types
codegen_cs.set_param('UnnecessaryDatatypeConvMsg', 'none');   % Unnecessary type conversions
codegen_cs.set_param('VectorMatrixConversionMsg', 'none');   % Vector/matrix block input conversion
codegen_cs.set_param('FcnCallInpInsideContextMsg', 'error');   % Context-dependent inputs
codegen_cs.set_param('SignalLabelMismatchMsg', 'none');   % Signal label mismatch
codegen_cs.set_param('UnconnectedInputMsg', 'warning');   % Unconnected block input ports
codegen_cs.set_param('UnconnectedOutputMsg', 'warning');   % Unconnected block output ports
codegen_cs.set_param('UnconnectedLineMsg', 'warning');   % Unconnected line
codegen_cs.set_param('SFcnCompatibilityMsg', 'none');   % S-function upgrades needed
codegen_cs.set_param('FrameProcessingCompatibilityMsg', 'error');   % Block behavior depends on frame status of signal
codegen_cs.set_param('UniqueDataStoreMsg', 'none');   % Duplicate data store names
codegen_cs.set_param('BusObjectLabelMismatch', 'warning');   % Element name mismatch
codegen_cs.set_param('RootOutportRequireBusObject', 'warning');   % Unspecified bus object at root Outport block
codegen_cs.set_param('AssertControl', 'UseLocalSettings');   % Model Verification block enabling
codegen_cs.set_param('AllowSymbolicDim', 'on');   % Allow symbolic dimension specification
codegen_cs.set_param('ModelReferenceIOMsg', 'none');   % Invalid root Inport/Outport block connection
codegen_cs.set_param('ModelReferenceVersionMismatchMessage', 'none');   % Model block version mismatch
codegen_cs.set_param('ModelReferenceIOMismatchMessage', 'none');   % Port and parameter mismatch
codegen_cs.set_param('UnknownTsInhSupMsg', 'warning');   % Unspecified inheritability of sample time
codegen_cs.set_param('ModelReferenceDataLoggingMessage', 'warning');   % Unsupported data logging
codegen_cs.set_param('ModelReferenceSymbolNameMessage', 'warning');   % Insufficient maximum identifier length
codegen_cs.set_param('ModelReferenceExtraNoncontSigs', 'error');   % Extraneous discrete derivative signals
codegen_cs.set_param('StateNameClashWarn', 'none');   % State name clash
codegen_cs.set_param('OperatingPointInterfaceChecksumMismatchMsg', 'warning');   % Operating point restore interface checksum mismatch
codegen_cs.set_param('NonCurrentReleaseOperatingPointMsg', 'error');   % Operating point object from a different release
codegen_cs.set_param('PregeneratedLibrarySubsystemCodeDiagnostic', 'warning');   % Behavior when pregenerated library subsystem code is missing
codegen_cs.set_param('InitInArrayFormatMsg', 'warning');   % Initial state is array
codegen_cs.set_param('StrictBusMsg', 'ErrorLevel1');   % Bus signal treated as vector
codegen_cs.set_param('BusNameAdapt', 'WarnAndRepair');   % Repair bus selections
codegen_cs.set_param('NonBusSignalsTreatedAsBus', 'none');   % Non-bus signals treated as bus signals
codegen_cs.set_param('SFUnusedDataAndEventsDiag', 'warning');   % Unused data, events, messages and functions
codegen_cs.set_param('SFUnexpectedBacktrackingDiag', 'error');   % Unexpected backtracking
codegen_cs.set_param('SFInvalidInputDataAccessInChartInitDiag', 'warning');   % Invalid input data access in chart initialization
codegen_cs.set_param('SFNoUnconditionalDefaultTransitionDiag', 'error');   % No unconditional default transitions
codegen_cs.set_param('SFTransitionOutsideNaturalParentDiag', 'warning');   % Transition outside natural parent
codegen_cs.set_param('SFUnreachableExecutionPathDiag', 'warning');   % Unreachable execution path
codegen_cs.set_param('SFUndirectedBroadcastEventsDiag', 'warning');   % Undirected event broadcasts
codegen_cs.set_param('SFTransitionActionBeforeConditionDiag', 'warning');   % Transition action specified before condition action
codegen_cs.set_param('SFOutputUsedAsStateInMooreChartDiag', 'error');   % Read-before-write to output in Moore chart
codegen_cs.set_param('SFTemporalDelaySmallerThanSampleTimeDiag', 'warning');   % Absolute time temporal value shorter than sampling period
codegen_cs.set_param('SFSelfTransitionDiag', 'warning');   % Self-transition on leaf state
codegen_cs.set_param('SFExecutionAtInitializationDiag', 'warning');   % 'Execute-at-initialization' disabled in presence of input events
codegen_cs.set_param('SFMachineParentedDataDiag', 'warning');   % Use of machine-parented data instead of Data Store Memory
codegen_cs.set_param('IntegerSaturationMsg', 'warning');   % Saturate on overflow
codegen_cs.set_param('AllowedUnitSystems', 'all');   % Allowed unit systems
codegen_cs.set_param('UnitsInconsistencyMsg', 'warning');   % Units inconsistency messages
codegen_cs.set_param('AllowAutomaticUnitConversions', 'on');   % Allow automatic unit conversions
codegen_cs.set_param('RCSCRenamedMsg', 'warning');   % Detect non-reused custom storage classes
codegen_cs.set_param('RCSCObservableMsg', 'warning');   % Detect ambiguous custom storage class final values
codegen_cs.set_param('ForceCombineOutputUpdateInSim', 'off');   % Combine output and update methods for code generation and simulation
codegen_cs.set_param('UnderSpecifiedDimensionMsg', 'none');   % Underspecified dimensions
codegen_cs.set_param('DebugExecutionForFMUViaOutOfProcess', 'off');   % FMU Import blocks
codegen_cs.set_param('ArithmeticOperatorsInVariantConditions', 'error');   % Arithmetic operations in variant conditions

% Hardware Implementation
codegen_cs.set_param('ProdHWDeviceType', 'Intel->x86-64 (Windows64)');   % Production device vendor and type
codegen_cs.set_param('ProdLongLongMode', 'off');   % Support long long
codegen_cs.set_param('ProdEqTarget', 'on');   % Test hardware is the same as production hardware
codegen_cs.set_param('TargetPreprocMaxBitsSint', 32);   % Maximum bits for signed integer in C preprocessor
codegen_cs.set_param('TargetPreprocMaxBitsUint', 32);   % Maximum bits for unsigned integer in C preprocessor
codegen_cs.set_param('HardwareBoardFeatureSet', 'EmbeddedCoderHSP');   % Feature set for selected hardware board

% Model Referencing
codegen_cs.set_param('UpdateModelReferenceTargets', 'IfOutOfDateOrStructuralChange');   % Rebuild
codegen_cs.set_param('EnableRefExpFcnMdlSchedulingChecks', 'on');   % Enable strict scheduling checks for referenced models
codegen_cs.set_param('EnableParallelModelReferenceBuilds', 'off');   % Enable parallel model reference builds
codegen_cs.set_param('ParallelModelReferenceErrorOnInvalidPool', 'on');   % Perform consistency check on parallel pool
codegen_cs.set_param('ModelReferenceNumInstancesAllowed', 'Multi');   % Total number of instances allowed per top model
codegen_cs.set_param('PropagateVarSize', 'Infer from blocks in model');   % Propagate sizes of variable-size signals
codegen_cs.set_param('ModelDependencies', '');   % Model dependencies
codegen_cs.set_param('ModelReferencePassRootInputsByReference', 'on');   % Pass fixed-size scalar root inputs by value for code generation
codegen_cs.set_param('ModelReferenceMinAlgLoopOccurrences', 'off');   % Minimize algebraic loop occurrences
codegen_cs.set_param('PropagateSignalLabelsOutOfModel', 'on');   % Propagate all signal labels out of the model
codegen_cs.set_param('SupportModelReferenceSimTargetCustomCode', 'off');   % Include custom code for referenced models

% Simulation Target
codegen_cs.set_param('SimCustomSourceCode', '');   % Source file
codegen_cs.set_param('SimCustomHeaderCode', '');   % Header file
codegen_cs.set_param('SimCustomInitializer', '');   % Initialize function
codegen_cs.set_param('SimCustomTerminator', '');   % Terminate function
codegen_cs.set_param('SimReservedNameArray', []);   % Reserved names
codegen_cs.set_param('SimUserSources', '');   % Source files
codegen_cs.set_param('SimUserIncludeDirs', '');   % Include directories
codegen_cs.set_param('SimUserLibraries', '');   % Libraries
codegen_cs.set_param('SimUserDefines', '');   % Defines
codegen_cs.set_param('SFSimEnableDebug', 'off');   % Allow setting breakpoints during simulation
codegen_cs.set_param('SFSimEcho', 'on');   % Echo expressions without semicolons
codegen_cs.set_param('SimCtrlC', 'on');   % Ensure responsiveness
codegen_cs.set_param('SimIntegrity', 'on');   % Ensure memory integrity
codegen_cs.set_param('SimParseCustomCode', 'on');   % Import custom code
codegen_cs.set_param('SimAnalyzeCustomCode', 'off');   % Enable custom code analysis
codegen_cs.set_param('SimBuildMode', 'sf_incremental_build');   % Simulation target build mode
codegen_cs.set_param('SimGenImportedTypeDefs', 'off');   % Generate typedefs for imported bus and enumeration types
codegen_cs.set_param('CompileTimeRecursionLimit', 50);   % Compile-time recursion limit for MATLAB functions
codegen_cs.set_param('EnableRuntimeRecursion', 'on');   % Enable run-time recursion for MATLAB functions
codegen_cs.set_param('MATLABDynamicMemAlloc', 'off');   % Dynamic memory allocation in MATLAB functions
codegen_cs.set_param('CustomCodeFunctionArrayLayout', []);   % Specify by function...
codegen_cs.set_param('DefaultCustomCodeFunctionArrayLayout', 'NotSpecified');   % Default function array layout
codegen_cs.set_param('CustomCodeUndefinedFunction', 'FilterOut');   % Undefined function handling

% Code Generation
codegen_cs.set_param('RemoveResetFunc', 'off');   % Remove reset function
codegen_cs.set_param('ExistingSharedCode', '');   % Existing shared code
codegen_cs.set_param('TLCOptions', '');   % TLC command line options
codegen_cs.set_param('GenCodeOnly', 'off');   % Generate code only
codegen_cs.set_param('PackageGeneratedCodeAndArtifacts', 'off');   % Package code and artifacts
codegen_cs.set_param('PostCodeGenCommand', '');   % Post code generation command
codegen_cs.set_param('GenerateReport', 'on');   % Create code generation report
codegen_cs.set_param('RTWVerbose', 'on');   % Verbose build
codegen_cs.set_param('RetainRTWFile', 'off');   % Retain .rtw file
codegen_cs.set_param('ProfileTLC', 'off');   % Profile TLC
codegen_cs.set_param('TLCDebug', 'off');   % Start TLC debugger when generating code
codegen_cs.set_param('TLCCoverage', 'off');   % Start TLC coverage when generating code
codegen_cs.set_param('TLCAssert', 'off');   % Enable TLC assertion
codegen_cs.set_param('RTWUseSimCustomCode', 'on');   % Use the same custom code settings as Simulation Target
codegen_cs.set_param('CustomBLASCallback', '');   % Custom BLAS library callback
codegen_cs.set_param('CustomLAPACKCallback', '');   % Custom LAPACK library callback
codegen_cs.set_param('CustomFFTCallback', '');   % Custom FFT library callback
codegen_cs.set_param('Toolchain', 'Automatically locate an installed toolchain');   % Toolchain
codegen_cs.set_param('BuildConfiguration', 'Faster Runs');   % Build configuration
codegen_cs.set_param('IncludeHyperlinkInReport', 'on');   % Code-to-model
codegen_cs.set_param('LaunchReport', 'off');   % Open report automatically
codegen_cs.set_param('PortableWordSizes', 'off');   % Enable portable word sizes
codegen_cs.set_param('CreateSILPILBlock', 'None');   % Create block
codegen_cs.set_param('CodeExecutionProfiling', 'off');   % Measure task execution time
codegen_cs.set_param('CodeProfilingInstrumentation', 'off');   % Measure function execution times
codegen_cs.set_param('CodeCoverageSettings', coder.coverage.CodeCoverageSettings([],'off','off','None'));   % Third-party tool
codegen_cs.set_param('SILDebugging', 'off');   % Enable source-level debugging for SIL
codegen_cs.set_param('GenerateTraceInfo', 'on');   % Model-to-code
codegen_cs.set_param('GenerateTraceReport', 'on');   % Eliminated / virtual blocks
codegen_cs.set_param('GenerateTraceReportSl', 'on');   % Traceable Simulink blocks
codegen_cs.set_param('GenerateTraceReportSf', 'on');   % Traceable Stateflow objects
codegen_cs.set_param('GenerateTraceReportEml', 'on');   % Traceable MATLAB functions
codegen_cs.set_param('GenerateWebview', 'off');   % Generate model Web view
codegen_cs.set_param('GenerateCodeMetricsReport', 'on');   % Generate static code metrics
codegen_cs.set_param('GenerateCodeReplacementReport', 'on');   % Summarize which blocks triggered code replacements
codegen_cs.set_param('ObjectivePriorities', {'Execution efficiency','RAM efficiency'});   % Prioritized objectives
codegen_cs.set_param('CheckMdlBeforeBuild', 'Off');   % Check model before generating code
codegen_cs.set_param('GenerateComments', 'on');   % Include comments
codegen_cs.set_param('ForceParamTrailComments', 'on');   % Verbose comments for 'Model default' storage class
codegen_cs.set_param('CommentStyle', 'Auto');   % Comment style
codegen_cs.set_param('IgnoreCustomStorageClasses', 'off');   % Ignore custom storage classes
codegen_cs.set_param('IgnoreTestpoints', 'off');   % Ignore test point signals
codegen_cs.set_param('MaxIdLength', 255);   % Maximum identifier length
codegen_cs.set_param('ShowEliminatedStatement', 'on');   % Show eliminated blocks
codegen_cs.set_param('OperatorAnnotations', 'on');   % Operator annotations
codegen_cs.set_param('SimulinkDataObjDesc', 'on');   % Simulink data object descriptions
codegen_cs.set_param('SFDataObjDesc', 'on');   % Stateflow object descriptions
codegen_cs.set_param('MATLABFcnDesc', 'off');   % MATLAB user comments
codegen_cs.set_param('MangleLength', 1);   % Minimum mangle length
codegen_cs.set_param('SharedChecksumLength', 8);   % Shared checksum length
codegen_cs.set_param('CustomSymbolStrGlobalVar', '$R$N$M_FSM101');   % Global variables
codegen_cs.set_param('CustomSymbolStrType', '$N$R$M_FSM101_T');   % Global types
codegen_cs.set_param('CustomSymbolStrField', '$N$M');   % Field name of global types
codegen_cs.set_param('CustomSymbolStrFcn', '$R$N$M_FSM101_$F');   % Subsystem methods
codegen_cs.set_param('CustomSymbolStrFcnArg', 'rt$I$N$M_FSM101');   % Subsystem method arguments
codegen_cs.set_param('CustomSymbolStrBlkIO', 'rtb_$N$M_FSM101');   % Local block output variables
codegen_cs.set_param('CustomSymbolStrTmpVar', '$N$M_FSM101');   % Local temporary variables
codegen_cs.set_param('CustomSymbolStrMacro', '$R$N$M_FSM101');   % Constant macros
codegen_cs.set_param('CustomSymbolStrEmxType', 'emxArray_$M$N_FSM101');   % EMX array types identifier format
codegen_cs.set_param('CustomSymbolStrEmxFcn', 'emx$M$N_FSM101');   % EMX array utility functions identifier format

codegen_cs.set_param('CustomSymbolStrGlobalVar', ['$R$N$M_FSM' num2str(id)]);   % Global variables
codegen_cs.set_param('CustomSymbolStrType', ['$N$R$M_FSM' num2str(id) '_T']);   % Global types
codegen_cs.set_param('CustomSymbolStrField', '$N$M');   % Field name of global types //Keep this the standard name convention since the struct type is already in a namespace
codegen_cs.set_param('CustomSymbolStrFcn', ['$R$N$M_FSM' num2str(id) '_$F']);   % Subsystem methods
codegen_cs.set_param('CustomSymbolStrFcnArg', ['rt$I$N$M_FSM' num2str(id)]);   % Subsystem method arguments
codegen_cs.set_param('CustomSymbolStrTmpVar', ['$N$M_FSM' num2str(id)]);   % Local temporary variables
codegen_cs.set_param('CustomSymbolStrBlkIO', ['rtb_$N$M_FSM' num2str(id)]);   % Local block output variables
codegen_cs.set_param('CustomSymbolStrMacro', ['$R$N$M_FSM' num2str(id)]);   % Constant macros
codegen_cs.set_param('CustomSymbolStrUtil', ['$N$C_FSM' num2str(id)]);   % Shared utilities
codegen_cs.set_param('CustomSymbolStrEmxType', ['emxArray_$M$N_FSM' num2str(id)]);   % EMX array types identifier format
codegen_cs.set_param('CustomSymbolStrEmxFcn', ['emx$M$N_FSM' num2str(id)]);   % EMX array utility functions identifier format

codegen_cs.set_param('CustomUserTokenString', '');   % Custom token text
codegen_cs.set_param('EnableCustomComments', 'off');   % Custom comments (MPT objects only)
codegen_cs.set_param('DefineNamingRule', 'None');   % #define naming
codegen_cs.set_param('ParamNamingRule', 'None');   % Parameter naming
codegen_cs.set_param('SignalNamingRule', 'None');   % Signal naming
codegen_cs.set_param('InsertBlockDesc', 'on');   % Simulink block descriptions
codegen_cs.set_param('InsertPolySpaceComments', 'off');   % Insert Polyspace comments
codegen_cs.set_param('SimulinkBlockComments', 'on');   % Simulink block comments
codegen_cs.set_param('StateflowObjectComments', 'off');   % Stateflow object comments
codegen_cs.set_param('BlockCommentType', 'BlockPathComment');   % Trace to model using
codegen_cs.set_param('MATLABSourceComments', 'off');   % MATLAB source code as comments
codegen_cs.set_param('InternalIdentifier', 'Shortened');   % System-generated identifiers
codegen_cs.set_param('InlinedPrmAccess', 'Literals');   % Generate scalar inlined parameters as
codegen_cs.set_param('ReqsInCode', 'off');   % Requirements in block comments
codegen_cs.set_param('UseSimReservedNames', 'off');   % Use the same reserved names as Simulation Target
codegen_cs.set_param('ReservedNameArray', []);   % Reserved names
codegen_cs.set_param('EnumMemberNameClash', 'error');   % Duplicate enumeration member names
codegen_cs.set_param('TargetLangStandard', 'C99 (ISO)');   % Standard math library
codegen_cs.set_param('CodeReplacementLibrary', 'None');   % Code replacement library
codegen_cs.set_param('UtilityFuncGeneration', 'Auto');   % Shared code placement
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
codegen_cs.set_param('ArrayLayout', 'Column-major');   % Array layout
codegen_cs.set_param('GenerateSampleERTMain', 'on');   % Generate an example main program
codegen_cs.set_param('IncludeFileDelimiter', 'Auto');   % #include file delimiter
codegen_cs.set_param('ERTCustomFileBanners', 'on');   % Enable custom file banner
%Adding parameters for setting the filename
codegen_cs.set_param('ERTHeaderFileRootName', ['$R_FSM' num2str(id) '$E']);   % Header files
codegen_cs.set_param('ERTSourceFileRootName', ['$R_FSM' num2str(id) '$E']);   % Source files
codegen_cs.set_param('ERTFilePackagingFormat', 'Compact');   % File packaging format
codegen_cs.set_param('GenerateFullHeader', 'on');   % Generate full file banner
codegen_cs.set_param('InferredTypesCompatibility', 'off');   % Create preprocessor directive in rtwtypes.h
codegen_cs.set_param('TargetLibSuffix', '');   % Suffix applied to target library name
codegen_cs.set_param('TargetPreCompLibLocation', '');   % Precompiled library location
codegen_cs.set_param('RemoveDisableFunc', 'off');   % Remove disable function
codegen_cs.set_param('LUTObjectStructOrderExplicitValues', 'Size,Breakpoints,Table');   % LUT object struct order for explicit value specification
codegen_cs.set_param('LUTObjectStructOrderEvenSpacing', 'Size,Breakpoints,Table');   % LUT object struct order for even spacing specification
codegen_cs.set_param('DynamicStringBufferSize', 256);   % Buffer size of dynamically-sized string (bytes)
codegen_cs.set_param('GlobalDataDefinition', 'Auto');   % Data definition
codegen_cs.set_param('GlobalDataReference', 'Auto');   % Data declaration
codegen_cs.set_param('ExtMode', 'off');   % External mode
codegen_cs.set_param('ExtModeTransport', 0);   % Transport layer
codegen_cs.set_param('ExtModeMexFile', 'ext_comm');   % MEX-file name
codegen_cs.set_param('ExtModeStaticAlloc', 'off');   % Static memory allocation
codegen_cs.set_param('EnableUserReplacementTypes', 'off');   % Replace data type names in the generated code
codegen_cs.set_param('ConvertIfToSwitch', 'on');   % Convert if-elseif-else patterns to switch-case statements
codegen_cs.set_param('ERTCustomFileTemplate', 'example_file_process.tlc');   % File customization template
codegen_cs.set_param('ERTDataHdrFileTemplate', 'ert_code_template.cgt');   % Header file template
codegen_cs.set_param('ERTDataSrcFileTemplate', 'ert_code_template.cgt');   % Source file template
codegen_cs.set_param('RateTransitionBlockCode', 'Inline');   % Rate Transition block code
codegen_cs.set_param('ERTHdrFileBannerTemplate', 'ert_code_template.cgt');   % Header file template
codegen_cs.set_param('ERTSrcFileBannerTemplate', 'ert_code_template.cgt');   % Source file template
codegen_cs.set_param('EnableDataOwnership', 'off');   % Use owner from data object for data definition placement
codegen_cs.set_param('ExtModeIntrfLevel', 'Level1');   % External mode interface level
codegen_cs.set_param('ExtModeMexArgs', '');   % MEX-file arguments
codegen_cs.set_param('ExtModeTesting', 'off');   % External mode testing
codegen_cs.set_param('GenerateASAP2', 'off');   % ASAP2 interface
codegen_cs.set_param('IndentSize', '2');   % Indent size
codegen_cs.set_param('IndentStyle', 'K&R');   % Indent style
codegen_cs.set_param('NewlineStyle', 'Default');   % Newline style
codegen_cs.set_param('MaxLineWidth', 80);   % Maximum line width
codegen_cs.set_param('MultiInstanceErrorCode', 'Error');   % Multi-instance code error diagnostic
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
codegen_cs.set_param('RootIOFormat', 'Individual arguments');   % Pass root-level I/O as
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
codegen_cs.set_param('MaxIdInt64', 'MAX_int64_T');   % 64-bit integer maximum identifier
codegen_cs.set_param('MinIdInt64', 'MIN_int64_T');   % 64-bit integer minimum identifier
codegen_cs.set_param('MaxIdUint64', 'MAX_uint64_T');   % 64-bit unsigned integer maximum identifier
codegen_cs.set_param('TypeLimitIdReplacementHeaderFile', '');   % Type limit identifier replacement header file
codegen_cs.set_param('DSAsUniqueAccess', 'off');   % Implement each data store block as a unique access point

% Simulink Coverage
codegen_cs.set_param('CovModelRefEnable', 'off');   % Record coverage for referenced models
codegen_cs.set_param('RecordCoverage', 'off');   % Record coverage for this model
codegen_cs.set_param('CovEnable', 'off');   % Enable coverage analysis

% HDL Coder
try 
	cs_componentCC = hdlcoderui.hdlcc;
	cs_componentCC.createCLI();
	codegen_cs.attachComponent(cs_componentCC);
catch ME
	warning('Simulink:ConfigSet:AttachComponentError', ME.message);
end
end

