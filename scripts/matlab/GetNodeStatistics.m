function [countMap] = GetNodeStatistics(nodes)
%GetNodeCountStatistics Gets Statistics on the Different Nodes in the

countMap = containers.Map('KeyType','char','ValueType','int64');

maxBlockTypeLen = 0;

%Find Max Block Type Len
for iter = 1:length(nodes)
    node = nodes(iter);
    
    blockTypeLen = length(node.simulinkBlockType);
    if(blockTypeLen > maxBlockTypeLen)
        maxBlockTypeLen = blockTypeLen;
    end
end

formatStr = ['%-' num2str(maxBlockTypeLen) 's | Inputs: %3d - %5s - %3s'];
generalFormatStr = ['%-' num2str(maxBlockTypeLen) 's |                          '];

for iter = 1:length(nodes)
    node = nodes(iter);
    
    if(node.nodeType == 1)
        key = sprintf(generalFormatStr, 'Subsystem');
        if(isKey(countMap, key))
            countMap(key) = countMap(key) + 1;
        else
            countMap(key) = 1;
        end
    elseif(node.nodeType == 7)
        key = sprintf(generalFormatStr, 'Expanded Node');
        if(isKey(countMap, key))
            countMap(key) = countMap(key) + 1;
        else
            countMap(key) = 1;
        end
    elseif(node.nodeType == 2)
        key = sprintf(generalFormatStr, 'Enabled Subsystem');
        if(isKey(countMap, key))
            countMap(key) = countMap(key) + 1;
        else
            countMap(key) = 1;
        end
    elseif(node.nodeType == 6)
        key = sprintf(generalFormatStr, 'Master Node');
        if(isKey(countMap, key))
            countMap(key) = countMap(key) + 1;
        else
            countMap(key) = 1;
        end
    elseif(node.nodeType == 9)
        key = sprintf(generalFormatStr, 'Stateflow');
        if(isKey(countMap, key))
            countMap(key) = countMap(key) + 1;
        else
            countMap(key) = 1;
        end
    elseif(node.nodeType == 10)
        key = sprintf(generalFormatStr, 'RateChange');
        if(isKey(countMap, key))
            countMap(key) = countMap(key) + 1;
        else
            countMap(key) = 1;
        end
    elseif(node.nodeType ~= 0)
        key = sprintf(generalFormatStr, 'Special Node');
        if(isKey(countMap, key))
            countMap(key) = countMap(key) + 1;
        else
            countMap(key) = 1;
        end
    else
        %Get the node block type
        blockType = node.simulinkBlockType;
        
        numInputs = length(node.in_arcs); %num in_arcs = num in ports since we do not allow multiple driver nets
        
        %Check for complexity
        foundComplex = false;
        foundReal = false;
        
        %Check for scalar / matrix
        foundScalar = false;
        foundVector = false;
        foundMatrix = false;
        
        for arcIter = 1:length(node.in_arcs)
            arc = node.in_arcs(arcIter);
            
            if(arc.complex)
                foundComplex = true;
            else
                foundReal = true;
            end
            
            if(arc.dimension(1) == 1)
                if(arc.dimension(2) == 1)
                    foundScalar = true;
                elseif(arc.dimension(2) > 1)
                    foundVector = true;
                end
            elseif(arc.dimension(1) > 1)
                foundDir = false;
                %It is possible for a scalar value to be expressed as
                %having a larger dimension (NCO appears to do this for ex).
                % Catch this when reporting.
                for dir = 2:length(arc.dimension)
                    if arc.dimension(dir)>1
                        foundDir = true;
                    end
                end
                if foundDir
                    foundMatrix = true;
                else
                    foundScalar = true;
                end
            end
            
        end
        
        complexity = 'N/A';
        if(foundComplex && foundReal)
            complexity = 'Mixed';
        elseif(foundComplex)
            complexity = 'Cmplx';
        elseif(foundReal)
            complexity = 'Real';
        end
        
        mat = '';
        if(foundMatrix)
            mat = [mat, 'M'];
        else
            mat = [mat, ' '];
        end
        if(foundVector)
            mat = [mat, 'V'];
        else
            mat = [mat, ' '];
        end
        if(foundScalar)
            mat = [mat, 'S'];
        else
            mat = [mat, ' '];
        end
        
        
        key = sprintf(formatStr, blockType, numInputs, complexity, mat);
        
        if(isKey(countMap, key))
            countMap(key) = countMap(key) + 1;
        else
            countMap(key) = 1;
        end
    end
end

