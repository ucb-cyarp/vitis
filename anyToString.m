function param_val_str = anyToString(param_val)
%Converts any type (we can reasonably think of) to a string
    if isempty(param_val)
        if iscell(param_val)
            param_val_str = '{}';
        else
            param_val_str = '[]';
        end
    elseif ischar(param_val)
        param_val_str = param_val;
    elseif isstruct(param_val)
        param_val_str = 'Struct(';
        
        field_names = fieldnames(param_val);
        
        if ~isempty(field_names)
            field_name = field_names{1};
            param_val_str = [param_val_str field_name '=' anyToString(getfield(param_val, field_name))];
        end
        
        for i = 2:length(field_names)
            field_name = field_names{i};
            param_val_str = [param_val_str ', ' field_name '=' anyToString(getfield(param_val, field_name))];
        end
        
        param_val_str = [param_val_str, ')'];
        
    elseif iscell(param_val)
        %Check for cell arrays.  Note that cell arrays of 1 will cause
        %isscalar to return true.  Therefore, check for cell array should
        %occur first
        
        %Already protected for empty cell array above
        
        [rows, cols] = size(param_val);
        param_val_str = '{';
        
        %first row
            %first col
        param_val_str = [param_val_str, anyToString(param_val{1, 1})];
            %other cols
        for j = 2:cols
            param_val_str = [param_val_str, ', ' anyToString(param_val{1, j})];
        end
        
        %other rows
        for i = 2:rows
            param_val_str = [param_val_str, '; '];
            %first col
            param_val_str = [param_val_str, anyToString(param_val{i, 1})];
            
            %other cols
            for j = 2:cols
                param_val_str = [param_val_str, ', ' anyToString(param_val{i, j})];
            end
        end
        
        param_val_str = [param_val_str, '}'];
            
        
    elseif ~isscalar(param_val)
        %Already protected for empty array array above
        
        [rows, cols] = size(param_val);
        param_val_str = '[';
        
        %first row
            %first col
        param_val_str = [param_val_str, anyToString(param_val(1, 1))];
            %other cols
        for j = 2:cols
            param_val_str = [param_val_str, ', ' anyToString(param_val(1, j))];
        end
        
        %other rows
        for i = 2:rows
            param_val_str = [param_val_str, '; '];
            %first col
            param_val_str = [param_val_str, anyToString(param_val(i, 1))];
            
            %other cols
            for j = 2:cols
                param_val_str = [param_val_str, ', ' anyToString(param_val(i, j))];
            end
        end
        
        param_val_str = [param_val_str, ']'];
        
    elseif isnumeric(param_val)
        param_val_str = num2str(param_val);
    elseif islogical(param_val)
        if(param_val)
            param_val_str = 'true';
        else
            param_val_str = 'false';
        end
    else
        %Not a type we know how to convert
            param_val_str = 'Error';
            warning(['Type for which no toString method is available ... Replaced with ''Error''']);
    end
    
    %XML Escape the entry
    param_val_str = xmlEscape(param_val_str);
end

