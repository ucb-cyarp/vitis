//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_GRAPHMLPARAMETER_H
#define VITIS_GRAPHMLPARAMETER_H

#include <string>

/**
 * Represents a GraphML parameter which includes a key and type.
 */
class GraphMLParameter {
private:
    ///The key used to represent the parameter in GraphML
    std::string key;

    ///The datatype used in GraphML to store this parameter
    std::string type;

    ///If true, this is a node parameter - if false, this is an edge parameter
    bool nodeParam;

public:
    /**
     * Construct an empty GraphML Parameter object
     */
    GraphMLParameter();

    GraphMLParameter(std::string key, std::string type, bool nodeParam);

    //Getters/Setters
    std::string getKey() const;
    void setKey(const std::string &key);
};


#endif //VITIS_GRAPHMLPARAMETER_H
