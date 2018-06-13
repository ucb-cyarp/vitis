classdef VectorFan < GraphNode
    %VectorFan A node which represents the fan-in/fan-out of a vector/bus
    %   This is used durring block expansion and contains an array of arc
    %   objects which represent indevidual wires of the bus.  The bus arc
    %   is connected to the VectorFan object durring the 
    
    %Is a subclass of GraphNode to support emitting.  Normally, pairs of
    %VectorFans are deleted as part of bus cleanup.  The exception is when
    %the bus sink or source is a Master node,  
    
    properties
        arcList
        wireIndexList
        busDirection %One of the following:
                     % 0 = Fan-In:  The indevidual arcs are fanning into the
                     %              VectorFan object.  The Bus has the
                     %              VectorFan as the SRC and the remote
                     %              block as the DST
                     % 1 = Fan-Out: The indevidual arcs are fanning out from 
                     %              the VectorFan object.  The Bus has the
                     %              VectorFan as the DST and the remote
                     %              block as the SRC
        busArc %The handle of the arc representing the bus
        markedForDeletion %Indicates if the node should be deleted.  Is set to true once all arcs have been 
    end
    
    methods
        function obj = VectorFan(busDirectionStr, parent)
            %VectorFan Construct an instance of the VectorFan class
            %   Starts with an empty list of arcs
            obj = obj@GraphNode('VectorFan', 'VectorFan', parent);
            parent.addChild(obj);
            obj.arcList = [];
            obj.wireIndexList = [];
            obj.busDirection = VectorFan.busDirFromStr(busDirectionStr);
            obj.busArc = [];
            obj.parent = parent;
            obj.markedForDeletion = false;
        end
        
        function addArc(obj, arc, wireIndex)
            %addArc Adds a fan-in/fan-out arc to the VectorFan object
            obj.arcList = [obj.arcList, arc];
            obj.wireIndexList = [obj.wireIndexList, wireIndex];
        end
        
        function addBusArc(obj, arc)
            %addBusArc Adds a bus arc to the VectorFan object
            obj.busArc = [obj.busArc, arc];
        end
        
        function result = isFanIn(obj)
            %isFanIn returns true if the VectorFan represents a fan-in to
            %the bus
            result = obj.busDirection == 0;
        end
        
        function result = isFanOut(obj)
            %isFanOut returns true if the VectorFan represents a fan-out 
            %from the bus
            result = obj.busDirection == 1;
        end
        
        function str = busDirStr(obj)
            %busDirStr Returns the string representation of the
            %busDirection
            
            if obj.busDirection == 0
                str = 'Fan-in';
            elseif obj.busDirection == 1
                str = 'Fan-out';
            else
                error(['''', num2str(obj.busDirection), ''' is not a recognized bus direction']);
            end
        end
        
        function delete(obj)
            %delete Removes the VectorFan from its parent's child list
            %before deleting the object
            parent = obj.parent;
            parent.removeChild(obj);
        end
        
        %NOTE: Only need to implement one direction.  Because pairs need to
        %be able to reach each other, traversael from either of the ends
        %needs to work.  There should be no case where a connection can
        %only be made by traversing in a specific direction.  Therefore,
        %only the implementation in one direction should be required.
        %NOTE: This should be called on each VectorFan object which is a
        %Fan-In EXCEPT thoes which are directly connected to master nodes
        %(deposited durring the expansion stage).
        function [arcs_to_delete] = reconnectArcs(obj, UnconnectedMasterNode)
            %reconnectArcs Reconnects the indevidual arcs that were fanned
            %from the VectorFan object durring expansion.  This involves
            %traversing the bus arc until it either reaches another VectorFan
            %in the oposite direction or reaches a select node where it is
            %trimmed away
            
            %If it reaches
            %another VectorFan, the matching arc is located and the arc is
            %connected to the block that the matching arc is connected to.
            %The origional arc is removed from the arc list of the
            %block it is connected to, and is placed in a list of arcs to
            %be deleted.
            
            %While traversing, the wire index may change if Vector
            %Concatenate or Select blocks are encountered.  No other block
            %type except Concatinate, Select, or VectorFans in the oposite direction
            %should be encountered.
            
            arcs_to_delete = [];
            
            if obj.isFanIn()
                %Need to do loop outside traversal because of the possibility
                %of select blocks changing the routing of the signal
                for i = 1:length(obj.arcList)
                    intermediateNodes = [];
                    intermediatePortNumbers = [];
                    intermediateWireNumbers = [];
                    intermediatePortTypes = [];
                    intermediatePortDirections = [];

                    arc = obj.arcList(i);
                    VectorFan_arc_ind = i;
                    cur_wire_number = obj.wireIndexList(i);

                    %Call recursive function on each of the of the output
                    %arcs of the VectorFan
                    connected = false;
                    for j = 1:length(obj.busArc)
                        cur_arc = obj.busArc(j);
                        [more_arcs_to_delete, was_connected] = reconnectArcs_helper_inner(arc, VectorFan_arc_ind, cur_arc, cur_wire_number, intermediateNodes, intermediatePortNumbers, intermediateWireNumbers, intermediatePortTypes, intermediatePortDirections);
                        arcs_to_delete = [arcs_to_delete, more_arcs_to_delete];
                        connected = connected || was_connected;
                    end
                    
                    %If this arc was not able to be connected,
                    %connect it to the "Unconnected" master node
                    %Note, it would not have been deleted as part of the
                    %process because the origional arc in the VectorFan
                    %(Fan-In) object is only deleted when a new vector
                    %(from the dst) is created to replace it
                    
                    if ~connected
                        arc.dstNode = UnconnectedMasterNode;
                    end
                    
                end
            elseif obj.isFanOut()
                %Trace in reverse (dst->src) along the bus arc
                error('reconnectArcs is not implemented for VectorArcs in FanOut mode, run on the FanIn pairs to reconnect.');
            else
                error('Unknown VectorFan fan direction');
            end
        end
        
        
        function [arcs_to_delete, connected] = reconnectArcs_helper_inner(arc, VectorFan_arc_ind, cur_arc, cur_wire_number, intermediateNodes, intermediatePortNumbers, intermediateWireNumbers, intermediatePortTypes, intermediatePortDirections)
            arcs_to_delete = [];
            connected = false;
            
            %Trace forward (src->dst) along the bus arc
            %Copy intermediate node entries from current
            %arc
            for intermediate_ind = 1:length(cur_arc.intermediateNodes)
                intermediateNodes = [intermediateNodes, cur_arc.intermediateNodes(intermediate_ind)];
                intermediatePortNumbers = [intermediatePortNumbers, cur_arc.intermediatePortNumbers(intermediate_ind)];
                intermediateWireNumbers = [intermediateWireNumbers, cur_arc.intermediateWireNumbers(intermediate_ind)];
                intermediatePortTypes = [intermediatePortTypes, cur_arc.intermediatePortTypes(intermediate_ind)];
                intermediatePortDirections = [intermediatePortDirections, cur_arc.intermediatePortDirections(intermediate_ind)];
            end

            cursor = cur_arc.dstNode;

            if isa(cursor, 'VectorFan')
                %This is a vector fan
                %Check that it is the oposite direction
                if cursor.busDirection == obj.busDirection
                    error('Encountered VectorFan object in the same direction');
                end

                %Lookup other arc (arc with same wire
                %number)
                inds = find(cursor.wireIndexList == cur_wire_number);
                if isempty(inds)
                    error('Could not find wire in VectorFan object');
                end
                
                %It is possible for there to be multiple outputs for a
                %single wire from a Fan-Out VectorFan
                for ind_counter = 1:length(inds)
                    ind = inds(ind_counter);
                
                    arc_pair = cursor.arcList(ind);

                    %Copy intermedate node entries encountered
                    %durring traversal
                    for j = length(intermediateNodes):-1:1
                        arc_pair.prependIntermediateNodeEntry(intermediateNodes(j), intermediatePortNumbers(j), intermediateWireNumbers(j), intermediatePortTypes(j), intermediatePortDirections(j));
                    end

                    %Copy Parameters (Intermediate Node
                    %Parameters)
                    for j = length(arc.intermediateNodes):-1:1
                        arc_pair.prependIntermediateNodeEntry(arc.intermediateNodes(j), arc.intermediatePortNumbers(j), arc.intermediateWireNumbers(j), arc.intermediatePortTypes(j), arc.intermediatePortDirections(j));
                    end

                    %Update src of arc
                    srcNode = arc.srcNode;

                    arc_pair.srcNode = arc.srcNode;
                    arc_pair.srcPortNumber = arc.srcPortNumber;
                    arc_pair.srcPortType = arc.srcPortType;

                    %Add arc to out_arcs list
                    srcNode.addOut_arc(arc_pair);

                    %Add pair arc to the list of arcs to
                    %delete
                    
                end
                %Remove arc from out_arcs list (Was replaced by arc_pairs
                %above)
                
                srcNode.removeOutArc(arc);
                arcs_to_delete = [arcs_to_delete, arc];

                %Return that connection was successful
                connected = true;

            elseif cursor.isMaster()
                %Should not occur in new refactored version.  Should only
                %encounter other VectorFan objects
                error('VectorFan re-connection encountered master node.  This should not occur.');

            elseif cursor.isStandard() && strcmp(cursor.simulinkBlockType, 'Concatenate')
                %Got to a concatenate block

                %Concat block is saved as an
                %intermediate node itself - is not deleted
                intermediateNodes = [intermediateNodes, cursor];
                intermediatePortNumbers = [intermediatePortNumbers, cur_arc.dstPortNumber];
                intermediateWireNumbers = [intermediateWireNumbers, cur_wire_number];
                intermediatePortTypes = [intermediatePortTypes, cur_arc.dstPortType];
                intermediatePortDirections = [intermediatePortDirections, portDirFromStr('In')];

                %---Calculate new wire number---

                %Offset is the sum of the widths of the
                %ports before this one
                offset = 0;

                for port = 1:(cur_arc.dstPortNumber - 1) %Do not include this port
                    %Find the in_arc with port 
                    for in_arc_iter = 1:length(cursor.in_arcs)
                        in_arc = cursor.in_arcs(in_arc_iter);
                        if in_arc.dstPortNumber == port
                            %Add the width of this port to
                            %the offset
                            offset = offset + in_arc.width;
                            break; %Found this port, can move to the next one
                        end
                    end
                end

                cur_wire_number = cur_wire_number + offset;

                %---Update cur_arc to new outgoing arc---

                %There should only be 1 outgoing arc from
                %concatenate
                if isempty(cursor.out_arcs)
                    error('There is not an output arc from the concatenate block.');
                elseif length(cursor.out_arcs)>2
                    error('There is more than one output arc from the concatenate block.');
                end

                for out_ind = 1:length(cursor.out_arcs)
                    cur_arc = cursor.out_arcs(out_ind);
                    [new_arcs_to_delete, new_connected] = reconnectArcs_helper_inner(arc, VectorFan_arc_ind, cur_arc, cur_wire_number, intermediateNodes, intermediatePortNumbers, intermediateWireNumbers, intermediatePortTypes, intermediatePortDirections);
                    arcs_to_delete = [arcs_to_delete, new_arcs_to_delete];
                    connected = connected || new_connected;
                end
                    
            elseif cursor.isStandard() && strcmp(cursor.simulinkBlockType, 'Selector')
                %Got to a selector

                %select block is saved as an
                %intermediate node itself - is not deleted
                intermediateNodes = [intermediateNodes, cursor];
                intermediatePortNumbers = [intermediatePortNumbers, cur_arc.dstPortNumber];
                intermediateWireNumbers = [intermediateWireNumbers, cur_wire_number];
                intermediatePortTypes = [intermediatePortTypes, cur_arc.dstPortType];
                intermediatePortDirections = [intermediatePortDirections, portDirFromStr('In')];

                %---Calculate new wire number or determine
                %that propogation ends here---

                selected_wires = node.dialogPropertiesNumeric('IndexParamArray');
                if node.dialogPropertiesNumeric('index_mode') == 0
                    %Increment selected_wires if "zero based"
                    %to be in line with matlab indexing
                    selected_wires = selected_wires + ones(size(selected_wires));
                end

                %The new wire number is the index of the
                %selected_wires array of the current wire
                %number
                new_wire_number = find(selected_wires == cur_wire_number);

                if isempty(new_wire_number)
                    %The current wire number was not found
                    %in the selected wires.  This is the
                    %end of this part of the traversal
                    
                elseif length(new_wire_number)>2
                    error('Select block includes wire more than once.  This is not allowed at this time.');
                else
                    cur_wire_number = new_wire_number;
                end

                for out_ind = 1:length(cursor.out_arcs)
                    cur_arc = cursor.out_arcs(out_ind);
                    [new_arcs_to_delete, new_connected] = reconnectArcs_helper_inner(arc, VectorFan_arc_ind, cur_arc, cur_wire_number, intermediateNodes, intermediatePortNumbers, intermediateWireNumbers, intermediatePortTypes, intermediatePortDirections);
                    arcs_to_delete = [arcs_to_delete, new_arcs_to_delete];
                    connected = connected || new_connected;
                end

            else
                %Found a different node type.  This should
                %not happen.
                error('Found an unexpected node type durring VectorFan traversal');
            end
        end
    end
        
    
    methods (Static)
        function num = busDirFromStr(busDir)
            %busDirFromStr Returns the numeric representation of the
            %bus direction from the string

            if strcmp(busDir, 'Fan-In')
                num = 0;
            elseif strcmp(busDir, 'Fan-Out')
                num = 1;
            else
                error(['''', busDir, ''' is not a recognized bus direction']);
            end

        end
    end
end

