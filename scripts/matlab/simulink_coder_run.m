function simulink_coder_run(simulink_file, subsys, dstDir)

%Open system
open_system(simulink_file);

%Change to dstDir
current_dir = pwd;
contains_path = contains(path, current_dir);
if(~contains_path)
    addpath(current_dir);
end

mkdir(dstDir)
cd(dstDir);

%Create coder config
orig_model_config = getActiveConfigSet(simulink_file);

curTime = datetime;
new_cfg_name = [orig_model_config.Name '-tmp' num2str(round(posixtime(curTime)))];

stateflow_codegen_model_config = CreateStateFlowCodeGenCS_Standalone(orig_model_config);
stateflow_codegen_model_config.Name = new_cfg_name;
attachConfigSet(simulink_file, stateflow_codegen_model_config);
setActiveConfigSet(simulink_file, new_cfg_name);

%Generate
rtwbuild(subsys, 'generateCodeOnly', true);

%Restore orig model config set
setActiveConfigSet(simulink_file, orig_model_config.Name);
%Remove stateflow codegen config set
detachConfigSet(simulink_file, new_cfg_name);

cd(current_dir);
if(~contains_path)
    rmpath(current_dir);
end

end