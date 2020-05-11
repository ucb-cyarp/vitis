function CleanupFunctionTemplate(simulink_file, origDataDictionaryName, dictionaryTmpName, dataDictionary)
%UNTITLED2 Summary of this function goes here
%   Detailed explanation goes here

%Close the temporary data dictionary
close(dataDictionary);

%Restore the orig dictionary
set_param(simulink_file, 'DataDictionary', origDataDictionaryName);

%Delete the temporary Simulink Dictionary
delete(dictionaryTmpName);
end

