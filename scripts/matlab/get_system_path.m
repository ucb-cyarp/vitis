function system_path = get_system_path(system_stack, delimitor)
%get_system_path Get the system path from the system stack (a cell array)
%   Assumes stack has its "top" at the last address
%   The path is constructed from the bottom of the stack, up

system_path = '';

if length(system_stack) > 0 
    system_path = system_stack{1};
    
    for i = 2:length(system_stack)
        system_path = [system_path, delimitor, system_stack{i}];
    end
end

end

