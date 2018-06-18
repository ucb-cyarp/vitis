function [expansion_occured, expanded_nodes, vector_fans, new_arcs] = ExpandTappedDelay(opBlock)
%ExpandTappedDelay Expands a tapped delay line

%Currently, only single wire inputs are supported

%Need to handle the degenerate case of a delay of 1.
%In that case, a single delay is created and the wires are simply conected

expansion_occured = false;
expanded_nodes = [];
vector_fans = [];
new_arcs = [];

%There should only be 1 in_arc
if length(opBlock.in_arcs) > 1
    error('Tapped delay block expansion only supports one input');
end

in_arc = opBlock.in_arcs(1);
if in_arc.width > 1
    error('Expansion of tapped delay block with vector input is currently not supported');
end

%% Apply Expansion

numDelays = opBlock.dialogPropertiesNumeric('NumDelays');
vinit = opBlock.dialogPropertiesNumeric('vinit');

if numDelays == 1
    %This is the degenerate case of a single delay.  Reduces to a single
    %delay which is re-wired.
    node = GraphNode.createExpandNodeNoSimulinkParams(opBlock, 'Standard', 'Delay', 1); %Sums are 'Standard' nodes.  This function adds the node as a child of the parent.
    node.simulinkBlockType = 'Delay';
    node.dialogPropertiesNumeric('NumDelays') = numDelays;
    node.dialogPropertiesNumeric('vinit') = vinit;
    node.dialogPropertiesNumeric('samptime') = opBlock.dialogProperties('samptime');
    node.dialogProperties('DelayLengthSource') = 'Dialog';
    node.dialogProperties('InitialConditionSource') = 'Dialog';
    expanded_nodes = [expanded_nodes, node];
    
    %Reconnect in arc (should only be 1)
    in_arc = opBlock.in_arcs(1);
    in_arc.dstNode = node;
    opBlock.removeIn_arc(in_arc);
    node.addIn_arc(in_arc);
    in_arc.appendIntermediateNodeEntry(opBlock, 1, 1, 'Standard', 'In');
    
    %Reconnect out arc (may be multiple)
    arcs_to_remove_from_orig = [];
    for i = 1:length(opBlock.out_arcs)
        out_arc = opBlock.out_arcs(i);
        out_arc.srcNode = node;
        out_arc.prependIntermediateNodeEntry(opBlock, 1, 1, 'Standard', 'Out');
        
        arcs_to_remove_from_orig = [arcs_to_remove_from_orig, out_arc];
        node.addOut_arc(out_arc);
    end
    
    for i = 1:length(arcs_to_remove_from_orig)
        out_arc = arcs_to_remove_from_orig(i);
        opBlock.removeOut_arc(out_arc);
    end
    
else
    %In this case, there are multiple delay blocks
    
    out_arc = opBlock.out_arcs(1);
    
    %Get the init values
    if length(vinit) > 1 
        delay_init_vals = vinit;
    else
        delay_init_vals = vinit .* ones(1, numDelays);
    end
    
    oldest_first = strcmp(opBlock.dialogProperties('DelayOrder'), 'Oldest');
    
    %There should only be a vectorfan (fan-in) for the output

    %Create Vector_Fan object

    %Should only be 1 output port (but potentially with many output arcs)
    vector_fan_output = VectorFan('Fan-In', opBlock); %Indevidual Arcs will be fanning into the bus

    vector_fans = [vector_fan_output];
    
    %----Treat first block specially----
    node = GraphNode.createExpandNodeNoSimulinkParams(opBlock, 'Standard', 'Delay', 1); %Sums are 'Standard' nodes.  This function adds the node as a child of the parent.
    node.simulinkBlockType = 'Delay';
    node.dialogPropertiesNumeric('NumDelays') = numDelays;
    node.dialogPropertiesNumeric('vinit') = delay_init_vals(1);
    node.dialogPropertiesNumeric('samptime') = opBlock.dialogProperties('samptime');
    node.dialogProperties('DelayLengthSource') = 'Dialog';
    node.dialogProperties('InitialConditionSource') = 'Dialog';
    expanded_nodes = [expanded_nodes, node];
    
    %Connect old in_arc to this block (should only be 1)
    in_arc = opBlock.in_arcs(1);
    in_arc.dstNode = node;
    opBlock.removeIn_arc(in_arc);
    node.addIn_arc(in_arc);
    in_arc.appendIntermediateNodeEntry(opBlock, 1, 1, 'Standard', 'In');
    
    %Create Arc from delay to vectorfan
    %    Copy out_arc to get parameters
    tap_arc = copy(out_arc);
    tap_arc.srcNode = node;
    tap_arc.srcPortNumber = 1;
    %Dst will be filled in durring bus cleanup
    tap_arc.width = 1;
    tap_arc.dimension = [1, 1];
    new_arcs = [new_arcs, tap_arc];
    node.addOut_arc(tap_arc);
    
    if oldest_first
        out_index = numDelays;
    else
        out_index = 1;
    end
    
    tap_arc.prependIntermediateNodeEntry(opBlock, 1, out_index, 'Standard', 'Out');
    vector_fan_output.addArc(tap_arc, out_index);
    
    prev_delay = node;
    template_arc = in_arc;

    %----Handle the rest of the delay blocks
    for i = 2:numDelays
        %Create new delay
        node = GraphNode.createExpandNodeNoSimulinkParams(opBlock, 'Standard', 'Delay', i); %Sums are 'Standard' nodes.  This function adds the node as a child of the parent.
        node.simulinkBlockType = 'Delay';
        node.dialogPropertiesNumeric('NumDelays') = numDelays;
        node.dialogPropertiesNumeric('vinit') = delay_init_vals(1);
        node.dialogPropertiesNumeric('samptime') = opBlock.dialogProperties('samptime');
        node.dialogProperties('DelayLengthSource') = 'Dialog';
        node.dialogProperties('InitialConditionSource') = 'Dialog';
        expanded_nodes = [expanded_nodes, node];
        
        %Connect to old delay
        chain_arc = copy(template_arc);
        chain_arc.srcNode = prev_delay;
        chain_arc.srcPortNumber = 1;
        chain_arc.dstNode = node;
        chain_arc.dstPortNumber = 1;
        prev_delay.addOut_arc(chain_arc);
        new_arcs = [new_arcs, chain_arc];
        node.addIn_arc(chain_arc);
        
        %Connect to vectorfan
        if oldest_first
            out_index = numDelays-i+1;
        else
            out_index = i;
        end
        
        tap_arc = copy(out_arc);
        tap_arc.srcNode = node;
        tap_arc.srcPortNumber = 1;
        %Dst will be filled in durring bus cleanup
        tap_arc.width = 1;
        tap_arc.dimension = [1, 1];
        new_arcs = [new_arcs, tap_arc];
        node.addOut_arc(tap_arc);
        
        tap_arc.prependIntermediateNodeEntry(opBlock, 1, out_index, 'Standard', 'Out');
        vector_fan_output.addArc(tap_arc, out_index);
        
        prev_delay = node;
    end

    %---Attach the old out arcs (maybe multiple) to the VectorFan output and remove it from the out_arcs list
    %of the orig object

    arcs_to_remove_from_opBlock = [];

    for i = 1:length(opBlock.out_arcs)
        out_arc = opBlock.out_arcs(i);
        arcs_to_remove_from_opBlock = [arcs_to_remove_from_opBlock, out_arc];

        out_arc.srcNode = vector_fan_output;
        vector_fan_output.addBusArc(out_arc);
    end

    for i = 1:length(arcs_to_remove_from_opBlock)
        opBlock.removeOut_arc(arcs_to_remove_from_opBlock(i));
    end
    
end

%Set orig node type to "Expanded" 
opBlock.setNodeTypeFromText('Expanded');

end

