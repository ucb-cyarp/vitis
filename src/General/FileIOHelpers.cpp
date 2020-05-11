//
// Created by Christopher Yarp on 5/11/20.
//

#include "FileIOHelpers.h"

#include <iostream>

#include <boost/filesystem.hpp>

bool FileIOHelpers::createDirectoryIfDoesNotExist(std::string path, bool reportCreating) {
    boost::filesystem::path boostPath(path);

    //Check if the path already exists (see https://www.boost.org/doc/libs/1_72_0/libs/filesystem/doc/tutorial.html)
    if(boost::filesystem::exists(boostPath)) {

        //Check if the path is to a directory (see https://www.boost.org/doc/libs/1_72_0/libs/filesystem/doc/tutorial.html)
        if (!boost::filesystem::is_directory(boostPath)) {
            throw std::runtime_error("Tried to create directory: " + path + " but a file already exists with the same name");
        }else{
            return false; //Directory already exists
        }
    }else{
        //Need to create the directory
        bool dirCreated = boost::filesystem::create_directory(boostPath);

        if(!dirCreated){
            throw std::runtime_error("Encountered an error when creating directory: " + path);
        }

        if(reportCreating){
            std::cout << "Created directory: " << path << std::endl;
        }

        return true;
    }
}
