
function RESULT____ = EvalWithVars(EXPR____, VARS____)
%EvalWithVars Evaluates an expresion with a set of variables
%   It is possible for name collisions to exist here.  Ideally, a new
%   workspace could be created and evalin could be used.  However, the
%   closest we can get to this is exeuting within a function
%   workspace.  However, local variables may still have a name collision.
%   As a result, all local variables are in all caps and are appended with
%   4 underscores.  This should help avoid name collisions.
%   It is worth noting that evalin('base', expr) could have worked if masks
%   were not used.
%
%   Variables are given as an array of structures.  They are applied to the
%   workspace in order

for I____ = 1:length(VARS____)
    VAR____ = VARS____(I____);
    
    %Check for illegal names
    if contains(VAR____.Name, '____')
        error(['Variable (' VAR____.Name ') contains ____.  This is used to namespace internal script variables and should not be used.']);
    end
    
    assigninHere(VAR____.Name, VAR____.Value);
end

RESULT____ = eval(EXPR____);

end

