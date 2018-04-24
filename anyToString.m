function param_val_str = anyToString(param_val)
%Converts any type (we can reasonably think of) to a string
    if ischar(param_val)
        param_val_str = param_val;
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
            warning([obj.name ' parameter ' param_name ' has unknown type ... Replaced with ''Error''']);
    end
end

