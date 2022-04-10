# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.231.5/containers/cpp/.devcontainer/base.Dockerfile

# [Choice] Debian / Ubuntu version (use Debian 11, Ubuntu 18.04/21.04 on local arm64/Apple Silicon): debian-11, debian-10, ubuntu-21.04, ubuntu-20.04, ubuntu-18.04
ARG VARIANT="bullseye"
FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-${VARIANT}

# [Optional] Install CMake version different from what base image has already installed. 
# CMake reinstall choices: none, 3.21.5, 3.22.2, or versions from https://cmake.org/download/
ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="none"

# Optionally install the cmake for vcpkg
COPY ./reinstall-cmake.sh /tmp/
RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
        chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
    fi \
    && rm -f /tmp/reinstall-cmake.sh

# [Optional] Uncomment this section to install additional vcpkg ports.
# RUN su vscode -c "${VCPKG_ROOT}/vcpkg install <your-port-name-here>"

# [Optional] Uncomment this section to install additional packages.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends clang \
#Vitis/Laminar Dependencies
      lcov \
#Need papi>5.6 which is the case with ubuntu 20.04
      papi-tools \
      libpapi-dev \
      libpfm4-dev \
      libboost-all-dev \
      libxerces-c-dev \
      graphviz \
#Used to install doxygen from apt but need a more modern version (see below) so I compile from src
#Doxygen build dependencies
      flex \
      bison \
#Other
      cloc \
  && apt-get clean

#TODO: Revisit if moving to a new Ubuntu base
#Install more modern doxygen 1.9.2 due to a regression in grouped classes (https://github.com/doxygen/doxygen/issues/7772)
RUN cd /tmp && \
    mkdir doxygen && \
    git clone https://github.com/doxygen/doxygen.git && \
    cd doxygen && \
    git checkout Release_1_9_2 && \
    mkdir build && \
    cd build && \
    cmake -G "Unix Makefiles" .. && \
    make && \
    make install && \
    #Cleanup Doxygen Build Dir
    cd /tmp && \
    rm -rf doxygen