function output_port_handle = findOuterOutputPort(simuling_outport_block, simulink_subsystem_handle)
%findInnerInputPortBlock Given a port handle to an input port on a given
%subsystem, find the Inport block within the subsystem that corresponds to
%this port.  This correspondence is established by comparing the
%'PortNumber' property of the port to the 'Port' property of the Inport
%block.  Returns a handle to the corresponding Inport.

port_block_num_str = get_param(simuling_outport_block, 'Port');
%The port is returned as a string, convert it to a number for
%comparison
port_block_num = str2double(port_block_num_str);

port_num = get_param(simulink_port_handle, 'PortNumber');

output_port_handle = [];

block_port_handles = get_param(simulink_subsystem_handle, 'PortHandles');
block_output_port_handles = block_port_handles.Outport;
%Get the specified output port based on the number
%TODO: Appears that the array index is the same as the port number.
%However, this was not specified in the help page.  Do a search just to be
%safe.

for i = 1:length(block_output_port_handles)
    %Check the port number
    candidate_port_num = get_param(block_output_port_handles(i), 'PortNumber');
    if port_num == candidate_port_num
        output_port_handle = block_output_port_handles(i);
        break;
    end
end

if isempty(output_port_handle)
    error('Could not find output port associated to outport block')
end

end

