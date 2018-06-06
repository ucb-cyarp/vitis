classdef SimulinkType
    %SimulinkFixedType A class to represent a simulink data type
    
    properties
        bits
        fractionalBits
        signed
        floating
    end
    
    methods
        function obj = SimulinkType()
            %SimulinkType Construct an instance of this class
            %   Only set default values
            obj.bits = 0;
            obj.fractionalBits = 0;
            obj.signed = false;
            obj.floating = false;
        end
        
        function str = toString(obj)
            %toString Output a string representation of the type

            %Output Floating Point Types
            if obj.floating
                if obj.bits == 32
                    str = 'single';
                elseif obj.bits == 64
                    str = 'double';
                else
                    error('Floating point type which is not a ''single'' or ''double''')
                end
            else
                %Output integer or fixed point types
                if obj.fractionalBits == 0 && (obj.bits == 1 || obj.bits == 8 || obj.bits == 16 || obj.bits == 32 || obj.bits == 64)
                    %Integer type
                    if obj.signed
                        %Signed
                        if obj.bits == 1
                            %Special case of fixed (really dosn't make any
                            %sense but can be delared)
                            str = 'sfix1_En0';
                        else
                            str = ['int' num2str(obj.bits)];
                        end
                    else
                        %Unsigned
                        if obj.bits == 1
                            str = 'boolean';
                        else
                            str = ['uint' num2str(obj.bits)];
                        end
                    end
                else
                    %fixed point type
                    if obj.signed
                        signed_str = 's';
                    else
                        signed_str = 'u';
                    end
                    
                    str = sprintf('%sfix%d_En%d', signed_str, obj.bits, obj.fractionalBits);
                end
            end
        end
    end
    
    methods(Static)
        function obj = FromStr(str)
            %FromStr Construct a SimulinkFixedType object
            %by parsing a type spec string
            
            %Check for Float Types
            if strcmp(str, 'single')
                obj = SimulinkType();
                obj.bits = 32;
                obj.fractionalBits = 0;
                obj.signed = true;
                obj.floating = true;
            elseif strcmp(str, 'double')
                obj = SimulinkType();
                obj.bits = 64;
                obj.fractionalBits = 0;
                obj.signed = true;
                obj.floating = true;
            
            %Check for Standard Bool & Int Types
            elseif strcmp(str, 'boolean')
                obj = SimulinkType();
                obj.bits = 1;
                obj.fractionalBits = 0;
                obj.signed = false;
                obj.floating = false;
            elseif strcmp(str, 'logical')
                %A synonym for boolean in matlab
                obj = SimulinkType();
                obj.bits = 1;
                obj.fractionalBits = 0;
                obj.signed = false;
                obj.floating = false;
            elseif strcmp(str, 'uint8')
                obj = SimulinkType();
                obj.bits = 8;
                obj.fractionalBits = 0;
                obj.signed = false;
                obj.floating = false;
            elseif strcmp(str, 'uint16')
                obj = SimulinkType();
                obj.bits = 16;
                obj.fractionalBits = 0;
                obj.signed = false;
                obj.floating = false;
            elseif strcmp(str, 'uint32')
                obj = SimulinkType();
                obj.bits = 32;
                obj.fractionalBits = 0;
                obj.signed = false;
                obj.floating = false;
            elseif strcmp(str, 'uint64')
                obj = SimulinkType();
                obj.bits = 64;
                obj.fractionalBits = 0;
                obj.signed = false;
                obj.floating = false;
            elseif strcmp(str, 'int8')
                obj = SimulinkType();
                obj.bits = 8;
                obj.fractionalBits = 0;
                obj.signed = true;
                obj.floating = false;
            elseif strcmp(str, 'int16')
                obj = SimulinkType();
                obj.bits = 16;
                obj.fractionalBits = 0;
                obj.signed = true;
                obj.floating = false;
            elseif strcmp(str, 'int32')
                obj = SimulinkType();
                obj.bits = 32;
                obj.fractionalBits = 0;
                obj.signed = true;
                obj.floating = false;
            elseif strcmp(str, 'int64')
                obj = SimulinkType();
                obj.bits = 64;
                obj.fractionalBits = 0;
                obj.signed = true;
                obj.floating = false;
            else
                %This is a fixed point type, parse the type string
                
                %2 basic forms we may see:
                %fixdt(1,16,13) or sfix16_En13
                %fixdt(0,16,13) or ufix16_En13
                %fixdt(signed, bits, fractionalBits) or [s,u]fix{bits}_En{fractionalBits}
                
                %The first form is common when declaring types while the
                %second is more commonly reported by simulink.
                
                fixdt_regex = 'fixdt[(]([0-9]+),([0-9]+),([0-9]+)[)]';
                fix_regex = '([su])fix([0-9]+)_En([0-9]+)';
                
                [matches,tokens] = regexp(str, fixdt_regex, 'match', 'tokens');
                if isempty(matches)
                    %Is the other format
                    [matches,tokens] = regexp(str, fix_regex, 'match', 'tokens');
                    
                    if isempty(matches)
                        error(['Fixed point type did not match expected format: ', str]);
                    end
                end
                
                obj = SimulinkType();
                obj.floating = false;
                if strcmp(tokens{1}, 's') || strcmp(tokens{1}, '1')
                    obj.signed = true;
                elseif strcmp(tokens{1}, 'u') || strcmp(tokens{1}, '0')
                    obj.signed = false;
                else
                    error(['Fixed point type did not match expected format: ', str]);
                end
                
                obj.bits = str2double(tokens{2});
                obj.fractionalBits = str2double(tokens{3});
                
            end
        end
        
        function obj = ConservativeType(typeA, typeB)
            %ConservativeType Construct a SimulinkFixedType object
            %which can fit all the values specified by typeA and typeB
            %without loss of precision (unless floating point).
            
            %If either of the types are floating point, the result is
            %floating point.  If both types are floating point, take the
            %larger bit width type
            
            %It should be noted that, while matlab states in its
            %documentation that doubles are promoted to fixed point rather
            %than the other way around (as would occur in C), Simulink
            %appears to follow the C like convention of converting to
            %floating point.
            
            obj = SimulinkType();
            
            if typeA.floating || typeB.floating
                %If either type is floating point
                obj.floating = true;
                obj.fractionalBits = 0;
                obj.signed = true; % All floating points are signed
                
                if typeA.floating && ~typeB.floating
                    obj.bits = typeA.bits;
                elseif ~typeA.floating && typeB.floating
                    obj.bits = typeB.bits;
                else
                    %Both are floating
                    obj.bits = max(typeA.bits, typeB.bits);
                end
            else
                %Types are integer or fixed point
                obj.floating = false;
                
                %Extract bit widths and promote to signed if required
                typeA_fractional = typeA.fractionalBits;
                typeA_integer = typeA.bits - typeA_fractional;
                if typeA_integer < 0
                    error('Integer type with negative number of integer bits');
                end
                if ~typeA.signed && typeB.signed
                    %Need to promote typeA to be signed by adding a bit
                    typeA_integer = typeA_integer + 1;
                end
                
                %Extract bit widths and promote to signed if required
                typeB_fractional = typeB.fractionalBits;
                typeB_integer = typeB.bits - typeB_fractional;
                if typeB_integer < 0
                    error('Integer type with negative number of integer bits');
                end
                if ~typeB.signed && typeA.signed
                    %Need to promote typeA to be signed by adding a bit
                    typeB_integer = typeB_integer + 1;
                end
                
                %Set Calculate new type
                %We need to take the maximum of the number of integer bits
                %and fractional bits.  Note that integer bits must be >=0
                %and fractional bits must grow in order to avoid loosing
                %resolution
                integer_bits = max(typeA_integer, typeB_integer);
                fractional_bits = max(typeA_fractional, typeB_fractional);
                
                bits = integer_bits + fractional_bits;
                
                obj.bits = bits;
                obj.fractionalBits = fractional_bits;
                obj.signed = typeA.signed || typeB.signed; %If either typeA or typeB are signed, the resulting type is also signed
            end
        end
    
        function obj = bitGrowAdd(typeA, typeB, numAdds, mode)
            %bitGrowAdd Logic for bit growth in adder

            %Currently, the only mode supported is "typical".  This is a conservative
            %form of bit growth for integer and fixed point. It should be noted that 
            %Simulink/Matlab do not always use conservative bit growth.  When the 
            %hardware target is set to Intel, there are actually some cases of bit 
            %shrinking. 

            %"typical" first selects the conservative type which fits
            %both typeA and typeB
            %Assuming fixed point types, "typical" grows the number of integer bits
            %(which is assumed to be >- 0) by ceil(log2(Nadds))
            %https://www.mathworks.com/help/fixedpoint/examples/perform-fixed-point-arithmetic.html

            %"typical" does not grow floating point types
            
            if ~strcmp(mode, 'typical')
                error('Only ''typical'' bit growth supported');
            end

            %Get the conservative type that fits both typeA and typeB
            %This handles promotion to floating point and signed if
            %nessisary.
            obj = ConservativeType(typeA, typeB);
            
            if ~obj.floating
                %this is an integer or fixed point type, grow bits
                
                %growing the number of integer bits is equivalent to
                %growing the number of bits and leaving the number of
                %fractional bits unchanged
                
                obj.bits = obj.bits + ceil(log2(numAdds));
            end
            %Else, this is floating point, do not grow
        end
    
        function obj = bitGrowMult(typeA, typeB, mode)

            % Note that simulink does not appear to promote the multiplication
            % of 2 singles to a double.  The product of 2 singles remains a
            % single.

            %Simulink will promote a single to a double if a single and double
            %are both used as arguments.

            % It is worth noting that fixed point multiplication, preserving
            % precision requires the word width of the result to equal the sum
            % of the word widths of the operands.  Likewise, the fractional
            % width must be the sum of the fractional widths of the operands.
            % This is detailed in the Matlab documentation
            % https://www.mathworks.com/help/fixedpoint/examples/perform-fixed-point-arithmetic.html

            % The actual bit growth experienced depends on the mode Simulink is
            % operating in.  If the target is set to FPGA/ASIC, full precision
            % is usually preserved.  If a CPU is set as the target, matlab
            % tries to keep the word widths standard CPU widths such as 8, 16,
            % 32, or 64

            if ~strcmp(mode, 'typical')
                error('Only ''typical'' bit growth supported');
            end

            %"typical" will grow the bit widths according to the ASIC target.
            %Promotion to floating point will occur if one of the operands is a
            %floating point.

            if typeA.floating || typeB.floating
                %One or both of the operands is a floating point number.
                %Find the conservative type which will be the floating point
                %number.  If both are floating point numbers, it will return
                %the type of the largest width.
                obj = SimulinkType.ConservativeType(typeA, typeB);
            else
                %The operands are integers or fixed point numbers.

                %The new width will be the sum of the widths of the 2 operands
                %and the new fractional width will be the sum of the fractional
                %width of each of the 2 operands
                new_bits = typeA.bits + typeB.bits;
                new_fractionalBits = typeA.fractionalBits + typeB.fractionalBits;

                %Now, need to check if one of the operands needed to be
                %promoted to signed, adding one more bit
                if (typeA.signed && ~typeB.signed) || (~typeA.signed && typeB.signed) % XOR
                    new_bits = new_bits + 1;
                end

                obj = SimulinkType();
                obj.bits = new_bits;
                obj.fractionalBits = new_fractionalBits;
                obj.signed = (typeA.signed || typeB.signed); %The result is signed if either of the operands are signed
                obj.floating = false;
            end
        end
    end
end
