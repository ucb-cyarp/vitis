function [origDataDictionaryName, dictionaryTmpName, dataDictionary] = ConfigFunctionTemplate(simulink_file, fsmID)
%UNTITLED Summary of this function goes here
%   Detailed explanation goes here
%Create a temporary Simulink Dictionary for the Model

% See https://www.mathworks.com/help/ecoder/ug/config_code_defs_programmatically_example.html
% https://www.mathworks.com/help/ecoder/ref/embeddedcoderdictionary.html

    origDataDictionaryName = get_param(simulink_file, 'DataDictionary');

    curTime = datetime;
    dictionaryTmpName = [simulink_file '_dict_tmp' num2str(round(posixtime(curTime))) '.sldd'];
    dataDictionary = Simulink.data.dictionary.create(dictionaryTmpName);
    if ~isempty(origDataDictionaryName)
        error('Do not currently support models with Simulink Dictionaries');
    %     origDataDictionary = Simulink.data.dictionary.open(origDataDictionaryName);
    %     dataDictionary=copy(origDataDictionary);
    %     Simulink.data.dictionary.close(origDataDictionary);
    end

    %Create a Coder Dictionary
    codeDictionary = coder.dictionary.create(dictionaryTmpName);
    
    %Create Storage Template to Change Header Filename
%     storageClasses = getSection(codeDictionary,'StorageClasses');
%     exportToNamespaceH = addEntry(storageClasses,'ExportToNamespaceHeader');
%     set(exportToNamespaceH,'HeaderFile', ['$R_FSM' num2str(fsmID) '.h'] ,'DataScope' ,'Exported');
    
    %Create Function Name Template
    functionTemplates = getSection(codeDictionary,'FunctionCustomizationTemplates');
    fcGlobal = addEntry(functionTemplates,'GlobalFunctions');
    set(fcGlobal,'FunctionName', ['$R$N_FSM' num2str(fsmID)]);
    fcShared = addEntry(functionTemplates,'GlobalSharedFunctions');
    set(fcShared,'FunctionName', ['$R$N$C_FSM' num2str(fsmID)]);
    saveChanges(dataDictionary);
    
    %Set Dictionary and set function 
    set_param(simulink_file, 'DataDictionary', dictionaryTmpName);
    coder.mapping.create(simulink_file);
%     coder.mapping.defaults.set(dictionaryTmpName,'InternalData','StorageClass','ExportToNamespaceHeader');
    coder.mapping.defaults.set(dictionaryTmpName, 'Execution', 'FunctionCustomizationTemplate', 'GlobalFunctions');
    coder.mapping.defaults.set(dictionaryTmpName, 'InitializeTerminate', 'FunctionCustomizationTemplate', 'GlobalFunctions');
    coder.mapping.defaults.set(dictionaryTmpName, 'SharedUtility', 'FunctionCustomizationTemplate', 'GlobalSharedFunctions');
end

