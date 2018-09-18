//
// Created by Christopher Yarp on 7/19/18.
//

#include "LUT.h"
#include "General/GeneralHelper.h"

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
    if(str == "No_Check"){
        return ExtrapMethod::NO_CHECK;
    }else if(str == "Clip"){
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
    if(extrapMethod == ExtrapMethod::NO_CHECK){
        return "No_Check";
    }else if(extrapMethod == ExtrapMethod::CLIP){
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

LUT::LUT() : interpMethod(InterpMethod::FLAT), extrapMethod(ExtrapMethod::CLIP), searchMethod(SearchMethod::EVENLY_SPACED_POINTS), emittedIndexCalculation(false){

}

LUT::LUT(std::shared_ptr<SubSystem> parent) : PrimitiveNode(parent), interpMethod(InterpMethod::FLAT), extrapMethod(ExtrapMethod::CLIP), searchMethod(SearchMethod::EVENLY_SPACED_POINTS), emittedIndexCalculation(false) {

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

        //Check if interp method is set to CLIP and the "RemoveProtectionInput" checkbox is set to "on"
        if(extrapMethod == ExtrapMethod::CLIP && dataKeyValueMap.at("InterpMethod") == "on"){
            extrapMethod = ExtrapMethod::NO_CHECK;
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
                    searchMethod = SearchMethod::LINEAR_SEARCH_NO_MEMORY;
                }else if(searchMethodStr == "Binary search"){
                    searchMethod = SearchMethod::BINARY_SEARCH_NO_MEMORY;
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

    //TODO: Implement complex input (related to 2D)
    unsigned long breakpointLen = breakpoints[0].size();
    for(unsigned long i = 0; i<breakpointLen; i++){
        if((breakpoints[0])[i].isComplex()){
            throw std::runtime_error("Validation Failed - LUT - Currently only supports real input");
        }
    }

    double firstBreakpoint = (breakpoints[0])[0].isFractional() ? (breakpoints[0])[0].getComplexDouble().real() : (breakpoints[0])[0].getRealInt();
    double lastBreakpoint = (breakpoints[0])[breakpointLen-1].isFractional() ? (breakpoints[0])[breakpointLen-1].getComplexDouble().real() : (breakpoints[0])[breakpointLen-1].getRealInt();
    double range = lastBreakpoint - firstBreakpoint;

    DataType inputType = getInputPort(0)->getDataType();

    if((!inputType.isFloatingPt()) && inputType.getFractionalBits() == 0){
        //Check for Integer Input Conditions
        //Check if the first input is an integer
        if((breakpoints[0])[0].isFractional()){
            throw std::runtime_error("Validation Failed - LUT - Currently only supports integer inputs when the starting breakpoint is an integer");
        }

        //Check if the step size is an integer type or if its reciprocal is an integer type
        if(breakpointLen > 1) {
            //TODO: using fmod here to check.  Relying on double having enough precision to return 0 if exactly divides.  Check Assumption
            if ((fmod(range, breakpointLen - 1) != 0.0) && (fmod(breakpointLen - 1, range) != 0.0)) {
                throw std::runtime_error(
                        "Validation Failed - LUT - Currently only supports integer inputs when breakpoint step is an integer or the reciprocal of which is an integer");
            }
        }


    }else if((!inputType.isFloatingPt()) && inputType.getFractionalBits() > 0){
        //TODO: Implement Fixed Point Type Checks (can fit first input and step)
        throw std::runtime_error("Validation Failed - LUT - Currently does not support fixed point input");
    }

    //Floating point should not require a check

    //Check equidistant points
    if(breakpointLen > 1){
        double breakpointStep = range/(breakpointLen-1.0); //The range is divided by the number of intervals/steps

        for(unsigned long i = 1; i<breakpointStep; i++){
            double pt1 = (breakpoints[0])[i-1].isFractional() ? (breakpoints[0])[i-1].getComplexDouble().real() : (breakpoints[0])[i-1].getRealInt();
            double pt2 = (breakpoints[0])[i].isFractional() ? (breakpoints[0])[i].getComplexDouble().real() : (breakpoints[0])[i].getRealInt();

            double step = pt2 - pt1;

            //TODO: using double here.  Assuming there is enough precision to calculate the step exactly.  Check assumption
            if(step != breakpointStep){
                throw std::runtime_error("Validation Failed - LUT - Breakpoints step conflicts: " + std::to_string(step) + " != " + std::to_string(breakpointStep));
            }

            if(step < 0){
                throw std::runtime_error("Validation Failed - LUT - Currently does not support decending breakpoints");
            }
        }
    }

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

bool LUT::hasGlobalDecl(){
    return true;
}

std::string LUT::getGlobalDecl(){
    //==== Emit the table as a constant global array ====

    //TODO: Implement > 1D table output

    //get the datatype from the output arc of the LUT
    DataType tableType = getOutputPort(0)->getDataType();

    //Get the variable name
    std::string varName = name+"_n"+std::to_string(id)+"_table";

    Variable tableVar = Variable(varName, tableType);

    std::string tableDecl = "const " + tableVar.getCVarDecl(false, false, false, false) + "[" + std::to_string(tableData.size()) + "] = " +
                            NumericValue::toStringComponent(false, tableType, tableData, "{\n", "\n}", ",\n") + ";";

    //Emit an imagionary vector if the table is complex
    if(tableType.isComplex()){
         tableDecl += "const " + tableVar.getCVarDecl(true, false, false, false) + "[" + std::to_string(tableData.size()) + "] = " +
                      NumericValue::toStringComponent(true, tableType, tableData, "{\n", "\n}", ",\n") + ";";
    }

    return tableDecl;
}

CExpr LUT::emitCExpr(std::vector<std::string> &cStatementQueue, int outputPortNum, bool imag){
    //Emit the index calculation
    std::string indexName = name+"_n"+std::to_string(id)+"_index";
    Variable indexVariable = Variable(indexName, DataType()); //The correct type will be set durring index calculation.  Type is not required for de-reference

    if(!emittedIndexCalculation) {

        //TODO: support other search methods?
        if (searchMethod != SearchMethod::EVENLY_SPACED_POINTS) {
            throw std::runtime_error("Emit Failed - LUT - Currently only supports Evenly spaced search method");
        }

        //TODO: Support multiple inputs (N-D inputs), only 1D is currently supported
        unsigned long numInputPorts = inputPorts.size();
        if (numInputPorts > 1) {
            throw std::runtime_error("Emit Failed - LUT - Currently only supports 1-D tables");
        }

        std::shared_ptr<OutputPort> srcOutputPort = getInputPort(0)->getSrcOutputPort();
        int srcOutputPortNum = srcOutputPort->getPortNum();
        std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

        std::string inputExpr = srcNode->emitC(cStatementQueue, srcOutputPortNum, imag);


        //If the Input Datatype is a floating point type, calculating the index takes the form:
        //    index = truncateToInt((input value - first breakpoint)/(breakpoint step))
        //If rounding is desired, 0.5 is added before the truncation
        //    index = truncateToInt((input value - first breakpoint)/(breakpoint step) + 0.5)

        //Find the range of the breakpoints, the number of breakpoints/intervals, the breakpoint step

        //If the input type is an integer or a fixed point type, the indexing can be more complex

        //If the input is an integer type and the scale is an integer, and the starting breakpoint is integer, then, modifications to the above algorithm do not need to be made*
        //   Except 0.5 cannot be added to the final result, instead 0.5*breakpointScale is added to the numerator before the division.
        //Truncation is also not required because all of the arithmetic is integer arithmetic

        //If the input is an integer and the reciprocal of the breakpoint scale is an integer, and the starting breakpoint is an integer, then the modification to the above algorithm is that the
        //numerator should be multiplied by the reciprocal of the breakpoint scale rather than divided by the breakpoint scale directly.  This is because
        //all of the math here is using integer arithmtic and would involve dividing my a number < 1.
        //Also, note that in this case, rounding does not do anything since the each input should map exactly to one breakpoint (with some breakpoints having no
        //Also, note that this is rather inefficient because it results in a table where some values are never used.

        //If the above cases are not true, then the input needs to be treated as a fixed point type.  For now, this will be considered an error state
        //TODO: Handle LUTs with integer input and insert appropriate scalaing to fixed point type

        //For fixed point inputs, the type must be of sufficient resolution to be able to represent the breakpoint step as well as the first breakpoint.
        // It must also have enough range to encompas both the first and last breakpoint.  An intemediate may need to be used which has sufficient range to include
        // the number of breakpoints (since we are caclulating the index).  To accomplish this, an inteemediary fixed point variable may need to be declared with the appropriate shift.
        // The result will need to be shifted back to an integer to get the correct integer index.
        //The above math (for floating point) can be used except with fixed point operations.

        //Checking for out of range:
        //In any case, an if/elseif/else statement is used to check if the input is outside of the range of breakpoints.
        //This can be done by looking at the value of the input or by looking at the value of the input or by looking at the returned index
        //One benifit of checking the input is that it should be in the range of the input type while the
        //Note that the bounds checking causes internal fanout

        //Reusing array index
        //This method computes the index in the LUT and then returns an array de-reference.

        //Get the input datatype
        DataType inputType = getInputPort(0)->getDataType();

        unsigned long numBreakPoints = breakpoints[0].size();

        unsigned long bitsRequiredForIndex = GeneralHelper::numIntegerBits(numBreakPoints-1, false);

        DataType indexType;
        indexType.setComplex(false);
        indexType.setTotalBits(bitsRequiredForIndex);
        indexType.setFractionalBits(0);
        indexType.setSignedType(false);

        indexVariable.setDataType(indexType);

        double firstBreakpoint = (breakpoints[0])[0].isFractional() ? (breakpoints[0])[0].getComplexDouble().real() : (breakpoints[0])[0].getRealInt();
        double lastBreakpoint = (breakpoints[0])[numBreakPoints-1].isFractional() ? (breakpoints[0])[numBreakPoints-1].getComplexDouble().real() : (breakpoints[0])[numBreakPoints-1].getRealInt();
        double range = lastBreakpoint - firstBreakpoint;
        double breakpointStep = range/(numBreakPoints-1.0); //The range is divided by the number of intervals/steps

        std::string indexDecl = indexVariable.getCVarDecl(false, false, false, false) + ";"; //Will output a standard CPU type automatically

        cStatementQueue.push_back(indexDecl);

        std::string indexExpr;

        if(inputType.isFloatingPt()){
            //Note that the first breakpoint and breakpointStep are doubles and should force promotion (they should be outputted with .00 if an integer)
            indexExpr = "((" + inputExpr + ") - (" + std::to_string(firstBreakpoint) + "))/(" + std::to_string(breakpointStep) + ")";

            //Round if nessisary
            if(interpMethod == InterpMethod::NEAREST){
                indexExpr += "+0.5";
            }else if(interpMethod == InterpMethod::CUBIC_SPLINE || interpMethod == InterpMethod::LINEAR){
                throw std::runtime_error("Emit Failed - LUT - Currently do not support Cubic Spline or Linear Interpolation");
            }

            //Truncate
            indexExpr = "((" + indexType.getCPUStorageType().toString(DataType::StringStyle::C, false) + ")(" + indexExpr + "))";

        }else if((!inputType.isFloatingPt()) && (inputType.getFractionalBits()==0)){
            //This is an integer type
            //For now, we only support integer first breakpoints for integers

            indexExpr = "(" + inputExpr + ") - (" + std::to_string((breakpoints[0])[0].getRealInt()) + ")";

            if(breakpointStep<1){
                double breakpointStepRecip = (numBreakPoints-1.0)/range;
                breakpointStepRecip = round(breakpointStep);
                //TODO: Relying on exact integer value of double.  Check assumption
                int64_t breakpointStepRecipInt = (int64_t) breakpointStepRecip;

                indexExpr = "(" + indexExpr + ")*" + std::to_string(breakpointStepRecipInt);
            }else{
                int64_t breakpointStepInt = (int64_t) (round(breakpointStep));

                if(interpMethod == InterpMethod::NEAREST){
                    //Rounding only make sense when the step is greater than 1.  This is because we add 0.5*step to the numerator.  A step <1 will have no impact on the final result

                    indexExpr += " + " + std::to_string(breakpointStepInt/2); //Take the integer divide
                }

                indexExpr = "(" + indexExpr + ")/" + "(" + std::to_string(breakpointStepInt) + ")";
            }

            indexExpr = "((" + indexType.toString(DataType::StringStyle::C, false) + ")(" + indexExpr + "))";

        }else{
            throw std::runtime_error("Emit Failed - LUT - Currently does not support fixed point input");
        }

        if(extrapMethod == ExtrapMethod::CLIP){
            //Add bounds check logic
            std::string boundCheckStr = "if(("+inputExpr+") > ";
            if((breakpoints[0])[numBreakPoints-1].isFractional()){
                boundCheckStr += std::to_string((breakpoints[0])[numBreakPoints-1].getComplexDouble().real());
            }else{
                boundCheckStr += std::to_string((breakpoints[0])[numBreakPoints-1].getRealInt());
            }
            boundCheckStr += "){\n" + indexVariable.getCVarName(false) + " = ";
            if((breakpoints[0])[numBreakPoints-1].isFractional()){
                boundCheckStr += std::to_string((breakpoints[0])[numBreakPoints-1].getComplexDouble().real());
            }else{
                boundCheckStr += std::to_string((breakpoints[0])[numBreakPoints-1].getRealInt());
            }

            boundCheckStr += ";\n}else if(("+inputExpr+") < ";
            if((breakpoints[0])[numBreakPoints-1].isFractional()){
                boundCheckStr += std::to_string((breakpoints[0])[0].getComplexDouble().real());
            }else{
                boundCheckStr += std::to_string((breakpoints[0])[0].getRealInt());
            }
            boundCheckStr += "){\n" + indexVariable.getCVarName(false) + " = ";
            if((breakpoints[0])[numBreakPoints-1].isFractional()){
                boundCheckStr += std::to_string((breakpoints[0])[0].getComplexDouble().real());
            }else{
                boundCheckStr += std::to_string((breakpoints[0])[0].getRealInt());
            }
            boundCheckStr += ";\n}else{\n" + indexVariable.getCVarName(false) + " = " + indexExpr + ";\n}";

            cStatementQueue.push_back(boundCheckStr);

        }else if(extrapMethod != ExtrapMethod::NO_CHECK){
            throw std::runtime_error("Emit Failed - LUT - Currently only supports the clip and \"no check\" extrapolation methods");
        }else{
            cStatementQueue.push_back(indexVariable.getCVarName(false) + " = " + indexExpr + ";");
        }

        emittedIndexCalculation = true;
    }

    //Need to re-create

    //Emit the array dereference
    std::string tablePrefix = name+"_n"+std::to_string(id)+"_table";
    Variable tableVar = Variable(tablePrefix, DataType());

    if(imag){
        return CExpr(tableVar.getCVarName(true) + "[" + indexVariable.getCVarName(false) + "]", false);
    }
    else{
        return CExpr(tableVar.getCVarName(false) + "[" + indexVariable.getCVarName(false) + "]", false);
    }
}

bool LUT::hasInternalFanout(int inputPort, bool imag){
    //If the Extrap method is clip, there is internal fanout on the input as it is used more than once.
    //Otherwise, the input is only used once to calculate the index

    if(extrapMethod == ExtrapMethod::CLIP){
        return true;
    }else{
        return false;
    }
}