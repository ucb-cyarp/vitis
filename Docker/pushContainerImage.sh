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

cd $scriptSrc 

if [[ -f lastBuiltTag.log ]]; then
    tag=$(cat lastBuiltTag.log)
    if [[ ! -z ${tag} ]]; then
        echo "docker tag laminar-remote-cpp-env-local:${tag} cyarp/laminar-clion-dev:${tag}"
        docker tag laminar-remote-cpp-env-local:${tag} cyarp/laminar-clion-dev:${tag}
    fi
fi

echo "docker tag laminar-remote-cpp-env-local:latest cyarp/laminar-clion-dev:latest"
docker tag laminar-remote-cpp-env-local:latest cyarp/laminar-clion-dev:latest

echo "docker push -a cyarp/laminar-clion-dev"
docker push -a cyarp/laminar-clion-dev

cd ${oldDir}