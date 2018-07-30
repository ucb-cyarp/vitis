//
// Created by Christopher Yarp on 7/19/18.
//

#ifndef VITIS_LUT_H
#define VITIS_LUT_H

#include "PrimitiveNode.h"
#include "GraphCore/NodeFactory.h"
#include "GraphCore/NumericValue.h"
#include "GraphMLTools/GraphMLDialect.h"

/**
 * \addtogroup PrimitiveNodes Primitives
 */
/*@{*/

/**
 * @brief Represents a Lookup table
 *
 * @note Currently only supports 1D LUTs
 */
class LUT : public PrimitiveNode{
    friend NodeFactory;

public:
    /**
     * @brief Methods for dealing with inputs between breakpoints
     */
    enum class InterpMethod{
        FLAT, ///< take the breakpoint below
        NEAREST, ///< take the nearest breakpoint, if equidistant, round up
        LINEAR, ///< linear interpolation between breakpoints
        CUBIC_SPLINE ///< cubic spline interpolation between breakpoints
    };

    /**
     * @brief Methods for dealing with inputs outside of breakpoint range
     */
    enum class ExtrapMethod{
        CLIP, ///< no extrapolation, returns end of range
        LINEAR, ///< linear extrapolation between outer pair of breakpoints
        CUBIC_SPLINE ///< cubic spline extrapolation between outer pair of breakpoints
    };

    /**
     * @brief Method for searching the LUT table for the desired value
     */
    enum class SearchMethod{
        EVENLY_SPACED_POINTS, ///<Relies on breakpoints being evenly spaced.  Can scale / truncate input and turn into an array index.
        LINEAR_SEARCH_NO_MEMORY, ///<Search using linear search.  Restarts search for each lookup.
        LINEAR_SEARCH_MEMORY, ///<Search using linear search.  Use previous value as starting point.
        BINARY_SEARCH_NO_MEMORY, ///<Search using binary search.  Restarts search for each lookup.
        BINARY_SEARCH_MEMORY ///<Search using binary search.  Use previous value as starting point.
    };

    static InterpMethod parseInterpMethodStr(std::string str);
    static ExtrapMethod parseExtrapMethodStr(std::string str);
    static SearchMethod parseSearchMethodStr(std::string str);

    static std::string interpMethodToString(InterpMethod interpMethod);
    static std::string extrapMethodToString(ExtrapMethod extrapMethod);
    static std::string searchMethodToString(SearchMethod searchMethod);

private:
    std::vector<std::vector<NumericValue>> breakpoints; ///< An array of breakpoint vectors.  breakpoints[0] is the vector of breakpoints for the 1st dimension
    std::vector<NumericValue> tableData;///<The table data of the LUT presented as a 1D vector.  Multidimensional array is stored in row major order.

    InterpMethod interpMethod; ///<The method for dealing with inputs between breakpoints
    ExtrapMethod extrapMethod; ///<The method for dealing with inputs outside of breakpoint range
    SearchMethod searchMethod; ///<The method to search through the LUT

    //TODO:implement N-D LUTs, currently only support 1D.  Need to modify Matlab script to output array as 1D row major

    //==== Constructors ====
    /**
     * @brief Constructs an empty LUT node
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     */
    LUT();

    /**
     * @brief Constructs an empty delay LUT with a given parent.  This node is not added to the children list of the parent.
     *
     * @note To construct from outside of hierarchy, use factories in @ref NodeFactory
     *
     * @param parent parent node
     */
    explicit LUT(std::shared_ptr<SubSystem> parent);

public:
    //====Getters/Setters====
    std::vector<std::vector<NumericValue>> getBreakpoints() const;
    void setBreakpoints(const std::vector<std::vector<NumericValue>> &breakpoints);
    std::vector<NumericValue> getTableData() const;
    void setTableData(const std::vector<NumericValue> &tableData);
    InterpMethod getInterpMethod() const;
    void setInterpMethod(InterpMethod interpMethod);
    ExtrapMethod getExtrapMethod() const;
    void setExtrapMethod(ExtrapMethod extrapMethod);
    SearchMethod getSearchMethod() const;
    void setSearchMethod(SearchMethod searchMethod);

    //====Factories====
    /**
     * @brief Creates a LUT node from a GraphML Description
     *
     * @note This function does not add the node to the design or to the nodeID/pointer map
     *
     * @param id the ID number of the node
     * @param name the human readable name of a node
     * @param dataKeyValueMap A map of property keys and values extracted from the data nodes in the GraphML
     * @param parent The parent of this node in the hierarchy
     * @param dialect The dialect of the GraphML file being imported
     * @return a pointer to the new delay node
     */
    static std::shared_ptr<LUT> createFromGraphML(int id, std::string name,
                                                    std::map<std::string, std::string> dataKeyValueMap,
                                                    std::shared_ptr<SubSystem> parent, GraphMLDialect dialect);

    //==== Emit Functions ====
    std::set<GraphMLParameter> graphMLParameters() override;

    xercesc::DOMElement* emitGraphML(xercesc::DOMDocument* doc, xercesc::DOMElement* graphNode, bool include_block_node_type = true) override ;

    std::string labelStr() override ;

    void validate() override;
};

/*@}*/

#endif //VITIS_LUT_H
