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

formatStr = ['%-' num2str(maxBlockTypeLen) 's | Inputs: %3d - %5s'];
generalFormatStr = ['%-' num2str(maxBlockTypeLen) 's |                    '];

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
    elseif(node.nodeType ~= 0)
        key = sprintf(generalFormatStr, 'Special Node');
        if(isKey(countMap, 'Special Node'))
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
        
        for arcIter = 1:length(node.in_arcs)
            arc = node.in_arcs(arcIter);
            
            if(arc.complex)
                foundComplex = true;
            else
                foundReal = true;
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
        
        key = sprintf(formatStr, blockType, numInputs, complexity);
        
        if(isKey(countMap, key))
            countMap(key) = countMap(key) + 1;
        else
            countMap(key) = 1;
        end
    end
end

