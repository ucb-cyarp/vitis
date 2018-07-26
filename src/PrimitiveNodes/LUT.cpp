//
// Created by Christopher Yarp on 7/19/18.
//

#include "LUT.h"

LUT::InterpMethod LUT::parseInterpMethodStr(std::string str) {
    if(str == "Flat"){
        return InterpMethod::FLAT;
    }else if(str == "Nearest"){
        return InterpMethod::NEAREST;
    }else if(str == "Linear"){
        return InterpMethod::LINEAR;
    }else if(str == "Cubic spline"){
        return InterpMethod::CUBIC_SPLINE;
    }else{
        throw std::runtime_error("Unknown InterpMethod: " + str);
    }
}

LUT::ExtrapMethod LUT::parseExtrapMethodStr(std::string str) {
    if(str == "Clip"){
        return ExtrapMethod::CLIP;
    }else if(str == "Linear"){
        return ExtrapMethod::LINEAR;
    }else if(str == "Cubic spline"){
        return ExtrapMethod::CUBIC_SPLINE;
    }else{
        throw std::runtime_error("Unknown ExtrapMethod: " + str);
    }
}

LUT::SearchMethod LUT::parseSearchMethodStr(std::string str) {
    if(str == "Evenly spaced points"){
        return SearchMethod::EVENLY_SPACED_POINTS;
    }else if(str == "Linear search (no memory)"){
        return SearchMethod::LINEAR_SEARCH_NO_MEMORY;
    }else if(str == "Linear search (memory)"){
        return SearchMethod::LINEAR_SEARCH_MEMORY;
    }else if(str == "Binary search (no memory)"){
        return SearchMethod::BINARY_SEARCH_NO_MEMORY;
    }else if(str == "Binary search (memory)"){
        return SearchMethod::BINARY_SEARCH_MEMORY;
    }else{
        throw std::runtime_error("Unknown SearchMethod: " + str);
    }
}

std::string LUT::interpMethodToString(LUT::InterpMethod interpMethod) {
    if(interpMethod == InterpMethod::FLAT){
        return "Flat";
    }else if(interpMethod == InterpMethod::NEAREST){
        return "Nearest";
    }else if(interpMethod == InterpMethod::LINEAR){
        return "Linear";
    }else if(interpMethod == InterpMethod::CUBIC_SPLINE){
        return "Cubic spline";
    }else{
        throw std::runtime_error("Unknown InterpMethod");
    }
}

std::string LUT::extrapMethodToString(LUT::ExtrapMethod extrapMethod) {
    if(extrapMethod == ExtrapMethod::CLIP){
        return "Clip";
    }else if(extrapMethod == ExtrapMethod::LINEAR){
        return "Linear";
    }else if(extrapMethod == ExtrapMethod::CUBIC_SPLINE){
        return "Cubic spline";
    }else{
        throw std::runtime_error("Unknown ExtrapMethod");
    }
}

std::string LUT::searchMethodToString(LUT::SearchMethod searchMethod) {
    if(searchMethod == SearchMethod::EVENLY_SPACED_POINTS){
        return "Evenly spaced points";
    }else if(searchMethod == SearchMethod::LINEAR_SEARCH_NO_MEMORY){
        return "Linear search (no memory)";
    }else if(searchMethod == SearchMethod::LINEAR_SEARCH_MEMORY){
        return "Linear search (memory)";
    }else if(searchMethod == SearchMethod::BINARY_SEARCH_NO_MEMORY){
        return "Binary search (no memory)";
    }else if(searchMethod == SearchMethod::BINARY_SEARCH_MEMORY){
        return "Binary search (memory)";
    }else{
        throw std::runtime_error("Unknown SearchMethod");
    }
}

LUT::LUT() : interpMethod(InterpMethod::FLAT), extrapMethod(ExtrapMethod::CLIP), searchMethod(SearchMethod::EVENLY_SPACED_POINTS){

}

LUT::LUT(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), interpMethod(InterpMethod::FLAT), extrapMethod(ExtrapMethod::CLIP), searchMethod(SearchMethod::EVENLY_SPACED_POINTS) {

}


std::vector<std::vector<NumericValue>> LUT::getBreakpoints() const {
    return breakpoints;
}

void LUT::setBreakpoints(const std::vector<std::vector<NumericValue>> &breakpoints) {
    LUT::breakpoints = breakpoints;
}

std::vector<NumericValue> LUT::getTableData() const {
    return tableData;
}

void LUT::setTableData(const std::vector<NumericValue> &tableData) {
    LUT::tableData = tableData;
}

LUT::InterpMethod LUT::getInterpMethod() const {
    return interpMethod;
}

void LUT::setInterpMethod(LUT::InterpMethod interpMethod) {
    LUT::interpMethod = interpMethod;
}

LUT::ExtrapMethod LUT::getExtrapMethod() const {
    return extrapMethod;
}

void LUT::setExtrapMethod(LUT::ExtrapMethod extrapMethod) {
    LUT::extrapMethod = extrapMethod;
}

LUT::SearchMethod LUT::getSearchMethod() const {
    return searchMethod;
}

void LUT::setSearchMethod(LUT::SearchMethod searchMethod) {
    LUT::searchMethod = searchMethod;
}

std::shared_ptr<LUT>
LUT::createFromGraphML(int id, std::string name, std::map<std::string, std::string> dataKeyValueMap,
                       std::shared_ptr<SubSystem> parent, GraphMLDialect dialect) {
    std::shared_ptr<LUT> newNode = NodeFactory::createNode<LUT>(parent);
    newNode->setId(id);
    newNode->setName(name);

    if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //==== Check Supported Config (Only if Simulink Import)====
        std::string dataSpecification = dataKeyValueMap.at("DataSpecification");
        if (dataSpecification != "Table and breakpoints") {
            throw std::runtime_error("LUT must use \"Table and breakpoints\" specification");
        }

        std::string breakpointSpecification = dataKeyValueMap.at("BreakpointsSpecification");
        if (breakpointSpecification != "Explicit values") {
            throw std::runtime_error("LUT must use \"Explicit values\" specification");
        }
    }

    //==== Import important properties ====
    std::string dimensionStr;
    std::string tableStr;

    InterpMethod interpMethod;
    ExtrapMethod extrapMethod;
    SearchMethod searchMethod;

    //Get the dimension, table (as a vector), interp/extrap/search methods
    //For simulink, extrap is clip for interp flat or nearest but will return linear
    //For simulink, search method, check for memory

    if (dialect == GraphMLDialect::VITIS) {
        //Vitis Names -- Dimensions, Table, InterpMethod, ExtrapMethod, SearchMethod
        dimensionStr = dataKeyValueMap.at("Dimensions");
        tableStr = dataKeyValueMap.at("Table");
        interpMethod = parseInterpMethodStr(dataKeyValueMap.at("InterpMethod"));
        extrapMethod = parseExtrapMethodStr(dataKeyValueMap.at("ExtrapMethod"));
        searchMethod = parseSearchMethodStr(dataKeyValueMap.at("SearchMethod"));
    } else if (dialect == GraphMLDialect::SIMULINK_EXPORT) {
        //Simulink Names -- Numeric.NumberOfTableDimensions, Numeric.Table, InterpMethod, ExtrapMethod, IndexSearchMethod, BeginIndexSearchUsingPreviousIndexResult
        dimensionStr = dataKeyValueMap.at("Numeric.NumberOfTableDimensions");
        tableStr = dataKeyValueMap.at("Numeric.Table");
        interpMethod = parseInterpMethodStr(dataKeyValueMap.at("InterpMethod"));

        if(interpMethod == InterpMethod::FLAT || interpMethod == InterpMethod::NEAREST){
            extrapMethod = ExtrapMethod::CLIP;
        } else{
            extrapMethod = parseExtrapMethodStr(dataKeyValueMap.at("ExtrapMethod"));
        }

        std::string searchMethodStr = dataKeyValueMap.at("IndexSearchMethod");
        if(searchMethodStr == "Evenly spaced points"){
            searchMethod = SearchMethod::EVENLY_SPACED_POINTS;
        }else{
            std::string memoryStr = dataKeyValueMap.at("BeginIndexSearchUsingPreviousIndexResult");

            if(memoryStr == "on"){
                if(searchMethodStr == "Linear search"){
                    searchMethod = SearchMethod::LINEAR_SEARCH_MEMORY;
                }else if(searchMethodStr == "Binary search"){
                    searchMethod = SearchMethod::BINARY_SEARCH_MEMORY;
                }else{
                    throw std::runtime_error("Unknown search type: " + searchMethodStr);
                }
            }else{
                if(searchMethodStr == "Linear search"){
                    searchMethod = SearchMethod::LINEAR_SEARCH_MEMORY;
                }else if(searchMethodStr == "Binary search"){
                    searchMethod = SearchMethod::BINARY_SEARCH_MEMORY;
                }else{
                    throw std::runtime_error("Unknown search type: " + searchMethodStr);
                }
            }
        }
    } else
    {
        throw std::runtime_error("Unsupported Dialect when parsing XML - LUT");
    }

    //Temporary error check for dimension != 1
    int dimension = std::stoi(dimensionStr);
    if(dimension != 1){
        throw std::runtime_error("LUT - Currently only supports 1-D tables");
    }

    //iterate and import breakpoints
    std::vector<std::vector<NumericValue>> breakpoints;

    for(int i = 1; i <= dimension; i++){
        std::string breakpointsForDimensionStr;
        if(dialect == GraphMLDialect::VITIS) {
            breakpointsForDimensionStr = "BreakpointsForDimension" + std::to_string(i);
        } else if(dialect == GraphMLDialect::SIMULINK_EXPORT) {
            breakpointsForDimensionStr = "Numeric.BreakpointsForDimension" + std::to_string(i);
        } else {
            throw std::runtime_error("Unsupported Dialect when parsing XML - LUT");
        }

        std::string breakpointStr = dataKeyValueMap.at(breakpointsForDimensionStr);
        breakpoints.push_back(NumericValue::parseXMLString(breakpointStr));
    }

    //import table data
    std::vector<NumericValue> table = NumericValue::parseXMLString(tableStr);

    newNode->setInterpMethod(interpMethod);
    newNode->setExtrapMethod(extrapMethod);
    newNode->setSearchMethod(searchMethod);

    newNode->setBreakpoints(breakpoints);
    newNode->setTableData(table);

    return newNode;
}

//Add dimension parameter
//Add parameters based on dimension

std::set<GraphMLParameter> LUT::graphMLParameters() {
    std::set<GraphMLParameter> parameters;

    parameters.insert(GraphMLParameter("Dimensions", "int", true));
    parameters.insert(GraphMLParameter("InterpMethod", "string", true));
    parameters.insert(GraphMLParameter("ExtrapMethod", "string", true));
    parameters.insert(GraphMLParameter("SearchMethod", "string", true));

    //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
    parameters.insert(GraphMLParameter("Table", "string", true));

    unsigned long dimension = breakpoints.size();
    for(unsigned long i = 1; i<=dimension; i++){
        //TODO: Declaring types as string so that complex can be stored.  Re-evaluate this
        std::string breakpointStr = "BreakpointsForDimension" + std::to_string(i);
        parameters.insert(GraphMLParameter(breakpointStr, "string", true));
    }

    return parameters;
}

void LUT::validate() {
    Node::validate();

    //TODO: implement n-D lookup table
    if(breakpoints.size()!=1){
        throw std::runtime_error("Validation Failed - LUT - Currently only supports 1-D tables, requested " + std::to_string(breakpoints.size()));
    }

    //Should have n input ports and 1 output port
    if(inputPorts.size() != breakpoints.size()){
        throw std::runtime_error("Validation Failed - LUT - Should Have Exactly n Input Ports for n-D Table");
    }

    if(outputPorts.size() != 1){
        throw std::runtime_error("Validation Failed - LUT - Should Have Exactly 1 Output Port");
    }

    //TODO: support other search methods?
    if(searchMethod != SearchMethod::EVENLY_SPACED_POINTS){
        throw std::runtime_error("Validation Failed - LUT - Currently only supports Evenly spaced search method");
    }

    //TODO: check for equidistant points
}

std::string LUT::labelStr() {
    std::string label = Node::labelStr();

    label += "\nFunction: LUT\nInterpMethod:" + interpMethodToString(interpMethod) + "\nExtrapMethod: " + extrapMethodToString(extrapMethod) + "\nSearchMethod: " + searchMethodToString(searchMethod)+ "\nTable: " + NumericValue::toString(tableData);

    unsigned long dimension = breakpoints.size();
    for(unsigned long i = 1; i<=dimension; i++){
        label += "\nBreakpoints Dimension(" + std::to_string(i) + "): " + NumericValue::toString(breakpoints[i-1]);
    }


    return label;
}

xercesc::DOMElement *
LUT::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode, bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Standard");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "LUT");

    unsigned long dimension = breakpoints.size();
    GraphMLHelper::addDataNode(doc, thisNode, "Dimensions", std::to_string(dimension));

    GraphMLHelper::addDataNode(doc, thisNode, "InterpMethod", interpMethodToString(interpMethod));
    GraphMLHelper::addDataNode(doc, thisNode, "ExtrapMethod", extrapMethodToString(extrapMethod));
    GraphMLHelper::addDataNode(doc, thisNode, "SearchMethod", searchMethodToString(searchMethod));

    GraphMLHelper::addDataNode(doc, thisNode, "Table", NumericValue::toString(tableData));

    for(unsigned long i = 1; i<=dimension; i++){
        std::string breakpointStr = "BreakpointsForDimension" + std::to_string(i);
        GraphMLHelper::addDataNode(doc, thisNode, breakpointStr, NumericValue::toString(breakpoints[i-1]));
    }

    return thisNode;
}
