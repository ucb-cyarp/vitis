function port_handle = getSimulinkOutputPortHandle(simulink_node, simulink_port_number)
%getSimulinkOutputPortHandle Get Simulink output port handle given a
%Simulink node and port number

block_port_handles = get_param(simulink_node, 'PortHandles');
if iscell(block_port_handles)
    block_port_handles = block_port_handles{1};
end
block_output_port_handles = block_port_handles.Outport;
%Get the specified output port based on the number
%TODO: Appears that the array index is the same as the port number.
%However, this was not specified in the help page.  Do a search just to be
%safe.
port_handle = [];
for i = 1:length(block_output_port_handles)
    %Check the port number
    port_num = get_param(block_output_port_handles(i), 'PortNumber');
    if port_num == simulink_port_number
        port_handle = block_output_port_handles(i);
        break;
    end
end

%Check for the case the port was not found
if isempty(port_handle)
    error(['Could not find specified simulink port']);
end

end

