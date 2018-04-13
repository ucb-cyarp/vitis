function enabled_subsystem = is_system_enabled(system)
%is_system_enabled Check if system is an enabled subsystem system

enable_block_list = find_system(system,  'FollowLinks', 'on', 'LoadFullyIfNeeded', 'on', 'LookUnderMasks', 'on', 'SearchDepth', 1, 'BlockType', 'EnablePort');
enabled_subsystem = ~isempty(enable_block_list);

if(length(enable_block_list) > 1)
    error(['[Simulink2GraphML] Error: ', system, ' has more than one enable block']);
elseif((strcmp(system_type, 'block_diagram')==1) && ~isempty(enable_block_list))
    error(['[Simulink2GraphML] Error: ', system, ' is a block diagram and cannot be enabled']);
end
end

