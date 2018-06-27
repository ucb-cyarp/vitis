//
// Created by Christopher Yarp on 6/26/18.
//

#ifndef VITIS_GRAPHMLPARAMETER_H
#define VITIS_GRAPHMLPARAMETER_H

#include <string>

/**
 * @brief Represents a GraphML parameter which includes a key and type.
 */
class GraphMLParameter {
private:
    std::string key; ///<The key used to represent the parameter in GraphML
    std::string type; ///<The data type used in GraphML to store this parameter
    bool nodeParam; ///<If true, this is a node parameter - if false, this is an edge parameter

public:
    //==== Constructors ====
    /**
     * @brief Construct an empty GraphML Parameter object
     */
    GraphMLParameter();

    /**
     * @brief Constructs a GraphML Parameter object with the given instance values
     *
     * @param key The key name of the parameter
     * @param type The datatype of the parameter
     * @param nodeParam True if the parameter is used with nodes, false if the parameter is used with edges
     */
    GraphMLParameter(const std::string &key, const std::string &type, bool nodeParam);

    //==== Functions ====
    /**
     * @brief Deep equivalence check of GraphMLParameter objects
     * @param rhs The right hand side of the equivalence check
     * @return true if the contents of both nodes are equivalent
     */
    bool operator==(const GraphMLParameter &rhs) const;

    /**
     * @brief Deep equivalence check of GraphMLParameter objects
     * @param rhs The right hand side of the equivalence check
     * @return true if the contents of both nodes are not equivalent
     */
    bool operator!=(const GraphMLParameter &rhs) const;

    /**
     * @brief Compares GraphMLParameter by key names, then types, then edege/node
     * @param a
     * @param b
     * @return true if the parameter key in a precedes the parameter key in b.  If names are equivalent, types are
     * compared.  If types are equivalent, nodeParam is used as the tie breaker with "edge" proceeding "node"
     */
    bool operator<(const GraphMLParameter &rhs) const;
    bool operator>(const GraphMLParameter &rhs) const;
    bool operator<=(const GraphMLParameter &rhs) const;
    bool operator>=(const GraphMLParameter &rhs) const;

    //====Getters/Setters====
    std::string getKey() const;
    void setKey(const std::string &key);
};


#endif //VITIS_GRAPHMLPARAMETER_H
