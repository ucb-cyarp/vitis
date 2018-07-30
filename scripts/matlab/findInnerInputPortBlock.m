function port_block_handle = findInnerInputPortBlock(simulink_subsystem_block_handle, simulink_port_handle)
%findInnerInputPortBlock Given a port handle to an input port on a given
%subsystem, find the Inport block within the subsystem that corresponds to
%this port.  This correspondence is established by comparing the
%'PortNumber' property of the port to the 'Port' property of the Inport
%block.  Returns a handle to the corresponding Inport.

port_num = get_param(simulink_port_handle, 'PortNumber');

inports = find_system(simulink_subsystem_block_handle,  'FollowLinks', 'on', 'LoadFullyIfNeeded', 'on', 'LookUnderMasks', 'on', 'SearchDepth', 1, 'BlockType', 'Inport');

port_block_handle = [];

for i = 1:length(inports)
    inport = inports(i);
    port_block_num_str = get_param(inport, 'Port');
    %The port is returned as a string, convert it to a number for
    %comparison
    port_block_num = str2double(port_block_num_str);
    
    if port_num == port_block_num
        port_block_handle = get_param(inport, 'Handle');
        break;
    end 
end

if isempty(port_block_handle)
    error('Could not find Inport block associated to input port')
end

end

