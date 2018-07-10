//
// Created by Christopher Yarp on 7/6/18.
//

#ifndef VITIS_DELAY_H
#define VITIS_DELAY_H

#include <vector>
#include <memory>
#include <map>
#include <string>
#include <complex>

#include "GraphCore/Node.h"
#include "GraphCore/NumericValue.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 */
/*@{*/

/**
 * @brief Represents a Delay (z^-1) Block
 */
class Delay : public Node{
    friend NodeFactory;
private:
    int delayValue; ///<The amount of delay in this node
    std::vector<NumericValue> initCondition; ///<The Initial condition of this delay.  Length must match the delay value.
    //TODO: Re-evaluate if numeric value should be stored as double/complex double (like Matlab does).  An advantage to providing a class that can contain both is that there is less risk of an int being store improperly and full 64 bit integers can be represented.
    //TODO: Advantage of storing std::complex is that each element is smaller (does not need to allocate both a double and int version)

    //==== Constructors ====
    /**
     * @brief Constructs an empty delay node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    Delay();

    /**
     * @brief Constructs an empty delay node with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit Delay(std::shared_ptr<SubSystem> parent);

public:
    //====Getters/Setters====
    int getDelayValue() const;
    void setDelayValue(int delayValue);
    std::vector<NumericValue> getInitCondition() const;
    void setInitCondition(const std::vector<NumericValue> &initCondition);

    //====Factories====
    /**
     * @brief Creates a delay node from a Simulink GraphML Description
     *
     * @note This function does not add the node to the design or to the nodeID/pointer map
     *
     * @param id the ID number of the node
     * @param dataKeyValueMap A map of property keys and values extracted from the data nodes in the GraphML
     * @param parent The parent of this node in the hierarchy
     * @return a pointer to the new delay node
     */
    static std::shared_ptr<Delay> createFromSimulinkGraphML(int id, std::map<std::string, std::string> dataKeyValueMap, std::shared_ptr<SubSystem> parent);
};

/*@}*/


#endif //VITIS_DELAY_H
