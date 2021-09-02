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

#Clean old container and images 
echo "docker stop clion_remote_env"
docker stop clion_remote_env

echo "docker rm clion_remote_env"
docker rm clion_remote_env

# echo "docker rmi laminar-remote-cpp-env-local"
# docker rmi laminar-remote-cpp-env-local

timestamp=$(date +%F_%H-%M-%S)

#Build the container image
cd $scriptSrc
echo "docker build -t laminar-remote-cpp-env-local:${timestamp} -f Dockerfile ."
docker build --progress=plain -t "laminar-remote-cpp-env-local:${timestamp}" -f Dockerfile . 2>&1 | tee dockerBuild.log

echo "docker tag laminar-remote-cpp-env-local:${timestamp} laminar-remote-cpp-env-local:latest"
docker tag "laminar-remote-cpp-env-local:${timestamp}" laminar-remote-cpp-env-local:latest

echo "${timestamp}" > lastBuiltTag.log

cd ${oldDir}