# Simulink Support

## Laminar Specific Simulink
* Partitions: A subsystem can be denoted as a partition by placing a constant block inside the subsystem and naming it
  ```LAMINAR_PARTITION``` or ```VITIS_PARTITION```.  The name of the block indicates the partition number.  Note that 
  the I/O partition is always -2.
  - The constant can be attached to a terminator block and will be pruned away by vitis before C code generation.
* SubBlocking: A subsystem can be noted to indicate that nodes contained within them should be sub-blocked by a 
  specified length.  This is accomplished by placing a constant block inside the subsystem and naming it
  ```LAMINAR_SUBBLOCKING``` or ```VITIS_SUBBLOCKING```.  This is the **base** sub-blocking length, outside of any 
  clock domain.
  - Limitations:
    - Context Expansion optimization passes will be stopped by nodes with a different specified sub-blocking
      length.  
    - Contextually executed domains (such as Enabled Subsystems) must be composed of nodes of a single sub-blocking
      length.  Clock Domains are an exception and may be composed of nodes with different sub-blocking lengths
    - FIFO Merging will not occur for nodes in different blocking domains (FIFO ports currently only support a single
      blocking domain at the input and output)
* Clock Domains: A clock domain can be specified by naming a subsystem with the prefix ```LAMINAR_CLOCK_DOMAIN``` or 
  ```VITIS_CLOCK_DOMAIN```.
  - Currently, only Downsampled clock domains are supported
  - All of the input ports need to share the same rate change (ex. upsample by 2, repeat by 2) and all of the output 
    ports need to be the inverse rate change (ex. downsample by 2).
       - Rate changes blocks can be nested within subsystems but must be between logic external to the clock domain and 
         any logic within the clock domain.
  - The one exception is for system inputs/outputs which are allowed to be connected without a rate change.  If a rate
    change is supplied for an I/O line, it needs to be consistent with the other inputs and outputs to the domain.
       - I/O which is directly connected to a node in a clock domain (ie. is not connected to a rate change block at 
         the boundary of the clock domain) is defined to operate at the rate of that clock domain
       - Fanout of an input to different clock domains operating at the same rate is permitted.  Fanout to domains 
         operating at different rates is not permitted.
  - Upsample and repeat blocks can be mixed so long as they represent the same rate change.
  - All clock domains in the design must be an integer upsample or downsample factor from the base rate (input rate).
    This allows the block size and vector lengths to be set statically.  This requirement may be relaxed in a future 
    version.

## Simulink Export Only Support
* Fixed Point numbers are only supported by the simulink export script.  Fixed point operations are not yet implemented 
  vitis.

## Laminar/Vitis Specific Library Blocks
There are some constructs which can be expressed in Simulink but in such a way that it can be convoluted for the 
compiler to handle.  The following are a list of library blocks that should be used in place of more complex simulink
structures.  They are extracted in the Simulink export scripts and map to configurations of native Laminar/Vitis nodes.
Even though these library blocks may appear as subsystems, the exported design will treat them as standard nodes.

Laminar/Vitis Block Name             | Library(s)                   | Implements                                               | Limitation Addressed
------------------------------------ | ---------------------------- | -------------------------------------------------------- | -------------------
VectorTappedDelay_startingWithOldest | rev1CyclopsLib or laminarLib | Vector Tapped Delay (Starting with Oldest Vector)        | Simulink Tapped Delay only supports scalars
TappedDelayWithReset_oldestFirst     | rev1CyclopsLib or laminarLib | Tapped Delay (with Reset Port)                           | Laminar/Vitis does not Implement Simulink Reset Subsystems
ComplexDotProductNoConj              | rev1CyclopsLib or laminarLib | Complex Dot Product without Complex Conjugation of Input | Simulink complex conjugates the first input before taking the dot product

## Supported Simulink Blocks:
*\*Note: Parameter restrictions for each node have not yet been documented below.*

### Primitives
Simulink Block Type         | Vitis Block Type        | Vitis Restrictions
--------------------------- | ----------------------- | ------------------
Subsystem                   | SubSystem               | *
Enabled Subsystem           | EnabledSubSystem        | Input & Output Ports Converted to Enable Input & Output Ports
Constant                    | Constant                | Also used for partition declaration with name ```LAMINAR_PARTITION``` or ```VITIS_PARTITION```.  Also used for Sub-Blocking declaration with name ```LAMINAR_SUBBLOCKING``` or ```VITIS_SUBBLOCKING```.
Input Port                  | MasterInput             | There is a single master shared by all system level inputs.  Converted to EnableInput if part of an enabled subsystem.  If at subsystem level, not converted
Output Port                 | MasterOutput            | There is a single master shared by all system level outputs.  Converted to EnableOutput if part of an enabled subsystem.  If at subsystem level, not converted
Terminator                  | MasterOutput            | There is a single master shared by all terminators
Relational Operator         | Compare                 | *
Lookup_n-D                  | LUT                     | *
Delay                       | Delay                   | *
Tapped Delay                | TappedDelay             | *
Complex to Real-Imag        | ComplexToRealImag       | *
Real-Imag to Complex        | RealImagToComplex       | *
Sum                         | Sum                     | *
Product                     | Product                 | *
Selector                    | (Expanded by Export)    | *
Logic                       | LogicalOperator         | *
Data Type Propagation       | UnsupportedSink         | Exports only but has no effect
Data Type Conversion        | DataTypeConversion      | *
Data Type Duplicate         | DataTypeDuplicate       | Only enforces the constraint that the inputs have the same type
Stateflow FSM               | SimulinkCoderFSM        | Implemented as a BlackBox from C code generated by Matlab/Simulink for the FSM
Bitwise Operator            | BitwiseOperator         | *
Trigonometry Operator       | Sin, Cos, Atan, Atan2   | *
Math                        | Ln, Exp                 | *
Dot Product                 | InnerProduct            | *
Concatenate                 | Concatenate             | *
Reshape                     | Reshape                 | *

### Medium Level
Simulink Block Type         | Vitis Block Type        | Vitis Restrictions
--------------------------- | ----------------------- | ------------------
Gain                        | Gain                    | *
Compare to Constant         | CompareToConstant       | *
Threshold Switch            | ThresholdSwitch         | *
Multiport Switch            | SimulinkMultiPortSwitch | *
Saturate                    | Saturate                | *
BitShift, ArithShift        | SimulinkBitShift        | *
Bitwise Operator            | SimulinkBitwiseOperator | *
NCO                         | NCO                     | *
BPSK Modulator              | DigitalModulator        | *
QPSK Modulator              | DigitalModulator        | *
Rectangular QAM Modulator   | DigitalModulator        | *
BPSK Demodulator            | DigitalDemodulator      | *
QPSK Demodulator            | DigitalDemodulator      | *
Rectangular QAM Demodulator | DigitalDemodulator      | *

### High Level
Simulink Block Type         | Vitis Block Type        | Vitis Restrictions
--------------------------- | ----------------------- | ------------------
Discrete FIR                | DiscreteFIR             | Expanded by Simulink Export


