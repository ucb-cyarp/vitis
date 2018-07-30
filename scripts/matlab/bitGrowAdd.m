function newType = bitGrowAdd(typeA, typeB, numAdds, mode)
%bitGrowAdd Logic for bit growth in adder

%Currently, the only mode supported is "typical".  This is a conservative
%form of bit growth for integer and fixed point. It should be noted that 
%Simulink/Matlab do not always use conservative bit growth.  When the 
%hardware target is set to Intel, there are actually some cases of bit 
%shrinking. 

%"typical grows "
%Assuming fixed point types add ceil(log2(Nadds))

%"typical" does not grow floating point types

if ~strcmp(mode, 'typical')
    error('Only ''typical'' bit growth supported');
end

if isFloatingPointType(typeA) && isFloatingPointType(typeB)
    %Both types are floating point, take the largest one
    if strcmp(typeA, 'double') || strcmp(typeB, 'double')
        newType = 'double';
    elseif strcmp(typeA, 'single') || strcmp(typeB, 'single')
        newType = 'single';
    else
        error('Type error: cannot find suitable floating point type');
    end
elseif isFloatingPointType(typeA)
    newType = typeA;
elseif isFloatingPointType(typeB)
    newType = typeB;
else
    %Both typeA and typeB are fixed point (or integer) types
    
    %Get the smallest type that encompases both
    
    
    %number of added bits is ceil(log2(Nadds))
    
    

outputArg1 = inputArg1;
outputArg2 = inputArg2;
end

