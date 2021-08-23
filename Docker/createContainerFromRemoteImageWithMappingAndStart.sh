#!/bin/bash

oldDir=$(pwd)

#Get build dir
scriptSrc=$(dirname "${BASH_SOURCE[0]}")
cd $scriptSrc
scriptSrc=$(pwd)
if [[ $(basename $scriptSrc) == Docker ]]; then
    cd ..
    projectDir=$(pwd)
else
    echo "Error: Unable to determine location of project directory"
    cd $oldDir
    exit 1
fi

#Create and run the container.  Map the project directory
echo "docker run -d --cap-add sys_ptrace -p127.0.0.1:2222:22 -v $projectDir:/project: --name clion_remote_env cyarp/laminar-clion-dev:latest"
docker run -d --cap-add sys_ptrace -p127.0.0.1:2222:22 -v "$projectDir":/project: --name clion_remote_env cyarp/laminar-clion-dev:latest

#Set the mapped directiry to /project

#Reset the ssh known hosts entry
echo "ssh-keygen -f $HOME/.ssh/known_hosts -R [localhost]:2222"
ssh-keygen -f "$HOME/.ssh/known_hosts" -R "[localhost]:2222"

cd ${oldDir}