//
// Created by Christopher Yarp on 9/3/21.
//

#include "SingleThreadGenerator.h"

#include "Emitter/SingleThreadEmit.h"
#include "Passes/DesignPasses.h"
#include "Passes/DomainPasses.h"
#include "Passes/ContextPasses.h"
#include "Scheduling/IntraPartitionScheduling.h"
#include "GraphMLTools/GraphMLExporter.h"
#include "General/ErrorHelpers.h"
#include "MultiRate/MultiRateHelpers.h"
#include "MasterNodes/MasterOutput.h"
#include "MasterNodes/MasterUnconnected.h"
#include "MasterNodes/MasterInput.h"
#include "PrimitiveNodes/BlackBox.h"

#include <iostream>
#include <fstream>

void SingleThreadGenerator::generateSingleThreadedC(Design &design, std::string outputDir, std::string designName, SchedParams::SchedType schedType, TopologicalSortParameters topoSortParams, unsigned long blockSize, bool emitGraphMLSched, bool printNodeSched){
    if(schedType == SchedParams::SchedType::BOTTOM_UP)
        emitSingleThreadedC(design, outputDir, designName, designName, schedType, blockSize);
    else if(schedType == SchedParams::SchedType::TOPOLOGICAL) {
        DesignPasses::prune(design, true);
        DesignPasses::pruneUnconnectedArcs(design, true);

        IntraPartitionScheduling::scheduleTopologicalStort(design, topoSortParams, false, false, designName, outputDir, printNodeSched, false);
        design.verifyTopologicalOrder(true, schedType);

        if(emitGraphMLSched) {
            //Export GraphML (for debugging)
            std::cout << "Emitting GraphML Schedule File: " << outputDir << "/" << designName
                      << "_scheduleGraph.graphml" << std::endl;
            GraphMLExporter::exportGraphML(outputDir + "/" + designName + "_scheduleGraph.graphml", design);
        }

        emitSingleThreadedC(design, outputDir, designName, designName, schedType, blockSize);
    }else if(schedType == SchedParams::SchedType::TOPOLOGICAL_CONTEXT){
        DesignPasses::prune(design, true);
        DesignPasses::pruneUnconnectedArcs(design, true);

        std::vector<std::shared_ptr<ClockDomain>> clockDomains = DomainPasses::findClockDomains(design);
        //TODO Optimize this so a second pass is not needed
        //After pruning, re-discover clock domain parameters since rate change nodes and links to MasterIO ports may have been removed
        //Before doing that, reset the ClockDomain links for master nodes as ports may have become disconnected
        DomainPasses::resetMasterNodeClockDomainLinks(design);
        MultiRateHelpers::rediscoverClockDomainParameters(clockDomains);

        //ClockDomain Rate Change Specialization.  Convert ClockDomains to UpsampleClockDomains or DownsampleClockDomains
        clockDomains = DomainPasses::specializeClockDomains(design, clockDomains);
        design.assignNodeIDs();
        design.assignArcIDs();
        //This also converts RateChangeNodes to Input or Output implementations.
        //This is done before other operations since these operations replace the objects because the class is changed to a subclass

        //AfterSpecialization, create support nodes for clockdomains, particularly DownsampleClockDomains
        DomainPasses::createClockDomainSupportNodes(design, clockDomains, false, true); //Have not done context discovery & marking yet so do not request context inclusion
        design.assignNodeIDs();
        design.assignArcIDs();

        //Check the ClockDomain rates are appropriate
        MultiRateHelpers::validateClockDomainRates(clockDomains);

        ContextPasses::expandEnabledSubsystemContexts(design);
        //Quick check to make sure vis node has no outgoing arcs (required for createEnabledOutputsForEnabledSubsysVisualization).
        //Is ok for them to exist (specifically order constraint arcs) after state update node and context encapsulation
        if(design.getVisMaster()->outDegree() > 0){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Visualization Master is Not Expected to Have an Out Degree > 0, has " + GeneralHelper::to_string(design.getVisMaster()->outDegree())));
        }
        ContextPasses::createEnabledOutputsForEnabledSubsysVisualization(design);
        design.assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
        design.assignArcIDs();

        ContextPasses::createContextVariableUpdateNodes(design); //Create after expanding the subcontext so that any movement of EnableInput and EnableOutput nodes
        design.assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
        design.assignArcIDs();

        ContextPasses::discoverAndMarkContexts(design);
//        assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
//        assignArcIDs();

        //Order constraining zero input nodes in enabled subsystems is not nessisary as rewireArcsToContexts can wire the enable
        //line as a depedency for the enable context to be emitted.  This is currently done in the scheduleTopoloicalSort method called below
        //TODO: re-introduce orderConstrainZeroInputNodes if the entire enable context is not scheduled hierarchically
        //orderConstrainZeroInputNodes(); //Do this after the contexts being marked since this constraint should not have an impact on contexts˚

        ContextPasses::encapsulateContexts(design);
        design.assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
        design.assignArcIDs();

        //Moved from before discoverAndMarkContexts to afterEncapsulate contexts to better match the multithreaded emitter
        //Since this is occuring after contexts are discovered and marked, the include context options should be set to true
        DesignPasses::createStateUpdateNodes(design, true); //Done after EnabledSubsystem Contexts are expanded to avoid issues with deleting and re-wiring EnableOutputs
        design.assignNodeIDs(); //Need to assign node IDs since the node ID is used for set ordering and new nodes may have been added
        design.assignArcIDs();

        //Export GraphML (for debugging)
//        std::cout << "Emitting GraphML pre-Schedule File: " << outputDir << "/" << designName << "_preSortGraph.graphml" << std::endl;
//        GraphMLExporter::exportGraphML(outputDir+"/"+designName+"_preSchedGraph.graphml", *this);

        IntraPartitionScheduling::scheduleTopologicalStort(design, topoSortParams, false, true, designName, outputDir, printNodeSched, false); //Pruned before inserting state update nodes
        design.verifyTopologicalOrder(true, schedType);

        //TODO: ValidateIO Block

        //TODO: Create variables for clock domains

        if(emitGraphMLSched) {
            //Export GraphML (for debugging)
            std::cout << "Emitting GraphML Schedule File: " << outputDir << "/" << designName
                      << "_scheduleGraph.graphml" << std::endl;
            GraphMLExporter::exportGraphML(outputDir + "/" + designName + "_scheduleGraph.graphml", design);
        }

        emitSingleThreadedC(design, outputDir, designName, designName, schedType, blockSize);
    }else{
        throw std::runtime_error("Unknown SCHED Type");
    }

    EmitterHelpers::emitParametersHeader(outputDir, designName, blockSize);
}

void SingleThreadGenerator::emitSingleThreadedOpsBottomUp(Design &design, std::ofstream &cFile, std::vector<std::shared_ptr<Node>> &nodesWithState, SchedParams::SchedType schedType){
    //Emit compute next states
    cFile << std::endl << "//==== Compute Next States ====" << std::endl;
    unsigned long numNodesWithState = nodesWithState.size();
    for(unsigned long i = 0; i<numNodesWithState; i++){
        std::vector<std::string> nextStateExprs;
        nodesWithState[i]->emitCExprNextState(nextStateExprs, schedType);
        cFile << std::endl << "//---- Compute Next States " << nodesWithState[i]->getFullyQualifiedName() <<" ----" << std::endl;
        cFile << "//~~~~ Orig Path: " << nodesWithState[i]->getFullyQualifiedOrigName()  << "~~~~" << std::endl;


        unsigned long numNextStateExprs = nextStateExprs.size();
        for(unsigned long j = 0; j<numNextStateExprs; j++){
            cFile << nextStateExprs[j] << std::endl;
        }
    }

    //Assign each of the outputs (and emit any expressions that preceed it)
    cFile << std::endl << "//==== Compute Outputs ====" << std::endl;
    unsigned long numOutputs = design.getOutputMaster()->getInputPorts().size();
    for(unsigned long i = 0; i<numOutputs; i++){
        std::shared_ptr<InputPort> output = design.getOutputMaster()->getInputPort(i);
        cFile << std::endl << "//---- Compute Output " << i << ": " << output->getName() <<" ----" << std::endl;

        //Get the arc connected to the output
        std::shared_ptr<Arc> outputArc = *(output->getArcs().begin());

        DataType outputDataType = outputArc->getDataType();

        std::shared_ptr<OutputPort> srcOutputPort = outputArc->getSrcPort();
        int srcNodeOutputPortNum = srcOutputPort->getPortNum();

        std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

        cFile << "//-- Compute Real Component --" << std::endl;
        std::vector<std::string> cStatements_re;
        CExpr expr_re = srcNode->emitC(cStatements_re, schedType, srcNodeOutputPortNum, false);
        //emit the expressions
        unsigned long numStatements_re = cStatements_re.size();
        for(unsigned long j = 0; j<numStatements_re; j++){
            cFile << cStatements_re[j] << std::endl;
        }

        //emit the assignment
        Variable outputVar = Variable(design.getOutputMaster()->getCOutputName(i), outputDataType);
        //TODO: Emit Vector Output Support for Single Threaded Emit
        if(!outputDataType.isScalar()){
            throw std::runtime_error(ErrorHelpers::genErrorStr("Vector/Matrix Output Not Currently Supported For Single Threaded Generator"));
        }
        cFile << "output[0]." << outputVar.getCVarName(false) << " = " << expr_re.getExpr() << ";" << std::endl;

        //Emit Imag if Datatype is complex
        if(outputDataType.isComplex()){
            cFile << std::endl << "//-- Compute Imag Component --" << std::endl;
            std::vector<std::string> cStatements_im;
            CExpr expr_im = srcNode->emitC(cStatements_im, schedType, srcNodeOutputPortNum, true);
            //emit the expressions
            unsigned long numStatements_im = cStatements_im.size();
            for(unsigned long j = 0; j<numStatements_im; j++){
                cFile << cStatements_im[j] << std::endl;
            }

            //emit the assignment
            cFile << "output[0]." << outputVar.getCVarName(true) << " = " << expr_im.getExpr() << ";" << std::endl;
        }
    }
}

void SingleThreadGenerator::emitSingleThreadedOpsSched(Design &design, std::ofstream &cFile, SchedParams::SchedType schedType){

    cFile << std::endl << "//==== Compute Operators ====" << std::endl;

    //Sort nodes by schedOrder.
    std::vector<std::shared_ptr<Node>> orderedNodes = design.getNodes();
    //Add the output master to the scheduled node list
    orderedNodes.push_back(design.getOutputMaster());
    std::sort(orderedNodes.begin(), orderedNodes.end(), Node::lessThanSchedOrder);

    std::shared_ptr<Node> zeroSchedNodeCmp = NodeFactory::createNode<MasterUnconnected>(); //Need a node to compare to
    zeroSchedNodeCmp->setSchedOrder(0);

    auto schedIt = std::lower_bound(orderedNodes.begin(), orderedNodes.end(), zeroSchedNodeCmp, Node::lessThanSchedOrder); //Binary search for the first node to be emitted (schedOrder 0)

    //Itterate through the schedule and emit
    for(auto it = schedIt; it != orderedNodes.end(); it++){

//        std::cout << "Writing " << (*it)->getFullyQualifiedName() << std::endl;

        if(*it == design.getOutputMaster()){
            //Emit output (using same basic code as bottom up except forcing fanout - all results will be availible as temp vars)
            unsigned long numOutputs = design.getOutputMaster()->getInputPorts().size();
            for(unsigned long i = 0; i<numOutputs; i++){
                std::shared_ptr<InputPort> output = design.getOutputMaster()->getInputPort(i);
                cFile << std::endl << "//---- Assign Output " << i << ": " << output->getName() <<" ----" << std::endl;

                //Get the arc connected to the output
                std::shared_ptr<Arc> outputArc = *(output->getArcs().begin());

                DataType outputDataType = outputArc->getDataType();

                std::shared_ptr<OutputPort> srcOutputPort = outputArc->getSrcPort();
                int srcNodeOutputPortNum = srcOutputPort->getPortNum();

                std::shared_ptr<Node> srcNode = srcOutputPort->getParent();

                cFile << "//-- Assign Real Component --" << std::endl;
                std::vector<std::string> cStatements_re;
                CExpr expr_re = srcNode->emitC(cStatements_re, schedType, srcNodeOutputPortNum, false, true, true);
                //emit the expressions
                unsigned long numStatements_re = cStatements_re.size();
                for(unsigned long j = 0; j<numStatements_re; j++){
                    cFile << cStatements_re[j] << std::endl;
                }

                //emit the assignment
                Variable outputVar = Variable(design.getOutputMaster()->getCOutputName(i), outputDataType);
                //TODO: Emit Vector Output Support for Single Threaded Emit
                if(!outputDataType.isScalar()){
                    throw std::runtime_error(ErrorHelpers::genErrorStr("Vector/Matrix Output Not Currently Supported For Single Threaded Generator"));
                }
                cFile << "output[0]." << outputVar.getCVarName(false) << " = " << expr_re.getExpr() << ";" << std::endl;

                //Emit Imag if Datatype is complex
                if(outputDataType.isComplex()){
                    cFile << std::endl << "//-- Assign Imag Component --" << std::endl;
                    std::vector<std::string> cStatements_im;
                    CExpr expr_im = srcNode->emitC(cStatements_im, schedType, srcNodeOutputPortNum, true, true, true);
                    //emit the expressions
                    unsigned long numStatements_im = cStatements_im.size();
                    for(unsigned long j = 0; j<numStatements_im; j++){
                        cFile << cStatements_im[j] << std::endl;
                    }

                    //emit the assignment
                    cFile << "output[0]." << outputVar.getCVarName(true) << " = " << expr_im.getExpr() << ";" << std::endl;
                }
            }
        }else if((*it)->hasState()){
            //Call emit for state element input
            //The state element output is treateded similarly to a constant and a variable name is always returned
            //The output has no dependencies within the cycle
            //The input can have dependencies

            //Emit comment
            cFile << std::endl << "//---- Calculate " << (*it)->getFullyQualifiedName() << " Inputs ----" << std::endl;
            cFile << "//~~~~ Orig Path: " << (*it)->getFullyQualifiedOrigName()  << "~~~~" << std::endl;

            std::vector<std::string> nextStateExprs;
            (*it)->emitCExprNextState(nextStateExprs, schedType);

            for(unsigned long j = 0; j<nextStateExprs.size(); j++){
                cFile << nextStateExprs[j] << std::endl;
            }

        }else{
            //Emit standard node

            //Emit comment
            cFile << std::endl << "//---- Calculate " << (*it)->getFullyQualifiedName() << " ----" << std::endl;
            cFile << "//~~~~ Orig Path: " << (*it)->getFullyQualifiedOrigName()  << "~~~~" << std::endl;

            unsigned long numOutputPorts = (*it)->getOutputPorts().size();
            //Emit each output port
            //TODO: check for unconnected output ports and do not emit them
            for(unsigned long i = 0; i<numOutputPorts; i++){
                std::vector<std::string> cStatementsRe;
                //Emit real component (force fanout)
                (*it)->emitC(cStatementsRe, schedType, i, false, true, true); //We actually do not use the returned expression.  Dependent nodes will get this by calling the emit function of this block again.

                for(unsigned long j = 0; j<cStatementsRe.size(); j++){
                    cFile << cStatementsRe[j] << std::endl;
                }

                if((*it)->getOutputPort(i)->getDataType().isComplex()) {
                    //Emit imag component (force fanout)
                    std::vector<std::string> cStatementsIm;
                    //Emit real component (force fanout)
                    (*it)->emitC(cStatementsIm, schedType, i, true, true, true); //We actually do not use the returned expression.  Dependent nodes will get this by calling the emit function of this block again.

                    for(unsigned long j = 0; j<cStatementsIm.size(); j++){
                        cFile << cStatementsIm[j] << std::endl;
                    }
                }
            }
        }

    }
}

void SingleThreadGenerator::emitSingleThreadedOpsSchedStateUpdateContext(Design &design, std::ofstream &cFile, SchedParams::SchedType schedType, int blockSize, std::string indVarName){

    cFile << std::endl << "//==== Compute Operators ====" << std::endl;

    //Sort nodes by schedOrder.
    std::vector<std::shared_ptr<Node>> orderedNodes = design.getNodes();
    //Add the output master to the scheduled node list
    orderedNodes.push_back(design.getOutputMaster());
    std::sort(orderedNodes.begin(), orderedNodes.end(), Node::lessThanSchedOrder);

    std::shared_ptr<Node> zeroSchedNodeCmp = NodeFactory::createNode<MasterUnconnected>(); //Need a node to compare to
    zeroSchedNodeCmp->setSchedOrder(0);

    auto schedIt = std::lower_bound(orderedNodes.begin(), orderedNodes.end(), zeroSchedNodeCmp, Node::lessThanSchedOrder); //Binary search for the first node to be emitted (schedOrder 0)

    std::vector<std::shared_ptr<Node>> toBeEmittedInThisOrder;
    std::copy(schedIt, orderedNodes.end(), std::back_inserter(toBeEmittedInThisOrder));

    EmitterHelpers::emitOpsStateUpdateContext(cFile, schedType, toBeEmittedInThisOrder, design.getOutputMaster(), blockSize);
}

void SingleThreadGenerator::emitSingleThreadedC(Design &design, std::string path, std::string fileName, std::string designName, SchedParams::SchedType sched, unsigned long blockSize) {
    EmitterHelpers::stringEmitTypeHeader(path);

    std::string blockIndVar = "";

    //Set the block size and indVarName in the master nodes.  This is used in the emit, specifically in the inputMaster
    design.getInputMaster()->setBlockSize(blockSize);
    design.getOutputMaster()->setBlockSize(blockSize);
    design.getTerminatorMaster()->setBlockSize(blockSize);
    design.getUnconnectedMaster()->setBlockSize(blockSize);
    design.getVisMaster()->setBlockSize(blockSize);

    if(blockSize > 1) {
        blockIndVar = "blkInd";
        design.getInputMaster()->setIndVarName(blockIndVar);
        design.getOutputMaster()->setIndVarName(blockIndVar);
        design.getTerminatorMaster()->setIndVarName(blockIndVar);
        design.getUnconnectedMaster()->setIndVarName(blockIndVar);
        design.getVisMaster()->setIndVarName(blockIndVar);
    }

    //Note: If the blockSize == 1, the function prototype can include scalar arguments.  If blockSize > 1, only pointer
    //types are allowed since multiple values are being passed

    //Get the OutputType struct defn and function prototype

    std::string outputTypeDefn = design.getCOutputStructDefn(blockSize);
    bool forceArrayArgs = blockSize > 1;
    std::string fctnProtoArgs = design.getCFunctionArgPrototype(forceArrayArgs);

    std::string fctnProto = "void " + designName + "(" + fctnProtoArgs + ")";

    std::cout << "Emitting C File: " << path << "/" << fileName << ".h" << std::endl;
    //#### Emit .h file ####
    std::ofstream headerFile;
    headerFile.open(path+"/"+fileName+".h", std::ofstream::out | std::ofstream::trunc);

    std::string fileNameUpper =  GeneralHelper::toUpper(fileName);
    headerFile << "#ifndef " << fileNameUpper << "_H" << std::endl;
    headerFile << "#define " << fileNameUpper << "_H" << std::endl;
    headerFile << "#include <stdint.h>" << std::endl;
    headerFile << "#include <stdbool.h>" << std::endl;
    headerFile << "#include <math.h>" << std::endl;
    headerFile << "#include \"" << VITIS_TYPE_NAME << ".h\"" << std::endl;
    //headerFile << "#include <thread.h>" << std::endl;
    headerFile << std::endl;

    //Output the Function Definition
    headerFile << outputTypeDefn << std::endl;
    headerFile << std::endl;
    headerFile << fctnProto << ";" << std::endl;

    //Output the reset function definition
    headerFile << "void " << designName << "_reset();" << std::endl;
    headerFile << std::endl;

    //Find nodes with state & global decls
    std::vector<std::shared_ptr<Node>> nodesWithState = design.findNodesWithState();
    std::vector<std::shared_ptr<Node>> nodesWithGlobalDecl = design.findNodesWithGlobalDecl();
    unsigned long numNodes = design.getNodes().size(); //Iterate through all un-pruned nodes in the design since this is a single threaded emit

    headerFile << "//==== State Variable Definitions ====" << std::endl;
    //We also need to declare the state variables here as extern;

    //Emit Definition
    unsigned long nodesWithStateCount = nodesWithState.size();
    for(unsigned long i = 0; i<nodesWithStateCount; i++){
        std::vector<Variable> stateVars = nodesWithState[i]->getCStateVars();
        //Emit State Vars
        unsigned long numStateVars = stateVars.size();
        for(unsigned long j = 0; j<numStateVars; j++){
            //cFile << "_Thread_local static " << stateVars[j].getCVarDecl(false, true, true) << ";" << std::endl;
            headerFile << "extern " << stateVars[j].getCVarDecl(false, true, false, true) << ";" << std::endl;

            if(stateVars[j].getDataType().isComplex()){
                headerFile << "extern " << stateVars[j].getCVarDecl(true, true, false, true) << ";" << std::endl;
            }
        }
    }

    headerFile << std::endl;

    //Insert BlackBox Headers
    std::vector<std::shared_ptr<BlackBox>> blackBoxes = design.findBlackBoxes();
    if(blackBoxes.size() > 0) {
        headerFile << "//==== BlackBox Headers ====" << std::endl;

        for(unsigned long i = 0; i<blackBoxes.size(); i++){
            headerFile << "//**** BEGIN BlackBox " << blackBoxes[i]->getFullyQualifiedName() << " Header ****" << std::endl;
            headerFile << blackBoxes[i]->getCppHeaderContent() << std::endl;
            headerFile << "//**** END BlackBox " << blackBoxes[i]->getFullyQualifiedName() << " Header ****" << std::endl;
        }
    }

    headerFile << "#endif" << std::endl;

    headerFile.close();

    std::cout << "Emitting C File: " << path << "/" << fileName << ".c" << std::endl;
    //#### Emit .c file ####
    std::ofstream cFile;
    cFile.open(path+"/"+fileName+".c", std::ofstream::out | std::ofstream::trunc);

    cFile << "#include \"" << fileName << ".h" << "\"" << std::endl;

    //Include any external include statements required by nodes in the design
    std::set<std::string> extIncludes;
    {
        std::vector<std::shared_ptr<Node>> nodes = design.getNodes();
        for (int i = 0; i < nodes.size(); i++) {
            std::set<std::string> nodeIncludes = nodes[i]->getExternalIncludes();
            extIncludes.insert(nodeIncludes.begin(), nodeIncludes.end());
        }
    }

    for(auto it = extIncludes.begin(); it != extIncludes.end(); it++){
        cFile << *it << std::endl;
    }

    cFile << std::endl;

    //Find nodes with state & Emit state variable declarations
    cFile << "//==== Init State Vars ====" << std::endl;
    for(unsigned long i = 0; i<nodesWithStateCount; i++){
        std::vector<Variable> stateVars = nodesWithState[i]->getCStateVars();
        //Emit State Vars
        unsigned long numStateVars = stateVars.size();
        for(unsigned long j = 0; j<numStateVars; j++){
            //cFile << "_Thread_local static " << stateVars[j].getCVarDecl(false, true, true) << ";" << std::endl;
            cFile << stateVars[j].getCVarDecl(false, true, true, true) << ";" << std::endl;

            if(stateVars[j].getDataType().isComplex()){
                cFile << stateVars[j].getCVarDecl(true, true, true, true) << ";" << std::endl;
            }
        }
    }

    //Emit the reset constants
    cFile << "//==== State Var Reset Vals ====" << std::endl;
    for(unsigned long i = 0; i<nodesWithStateCount; i++) {
        std::vector<Variable> stateVars = nodesWithState[i]->getCStateVars();

        unsigned long numStateVars = stateVars.size();
        for(unsigned long j = 0; j<numStateVars; j++) {
            std::vector<std::string> rstConsts = stateVars[j].getResetConst(true);
            for(const std::string &constant : rstConsts){
                cFile << constant << std::endl;
            }
        }
    }

    cFile << std::endl;

    cFile << "//==== Global Declarations ====" << std::endl;
    unsigned long nodesWithGlobalDeclCount = nodesWithGlobalDecl.size();
    for(unsigned long i = 0; i<nodesWithGlobalDeclCount; i++){
        cFile << nodesWithGlobalDecl[i]->getGlobalDecl() << std::endl;
    }

    cFile << std::endl;

    //Emit BlackBox C++ functions
    if(blackBoxes.size() > 0) {
        cFile << "//==== BlackBox Functions ====" << std::endl;

        for(unsigned long i = 0; i<blackBoxes.size(); i++){
            cFile << "//**** BEGIN BlackBox " << blackBoxes[i]->getFullyQualifiedName() << " Functions ****" << std::endl;
            cFile << blackBoxes[i]->getCppBodyContent() << std::endl;
            cFile << "//**** END BlackBox " << blackBoxes[i]->getFullyQualifiedName() << " Functions ****" << std::endl;
        }
    }

    cFile << std::endl;

    cFile << "//==== Functions ====" << std::endl;

    cFile << fctnProto << "{" << std::endl;

    //emit inner loop
    DataType blockDT = DataType(false, false, false, (int) std::ceil(std::log2(blockSize)+1), 0, {1});
    if(blockSize > 1) {
        //TODO: Creatr other variables for clock domains
        cFile << "for(" + blockDT.getCPUStorageType().toString(DataType::StringStyle::C, false, false) + " " + blockIndVar + " = 0; " + blockIndVar + "<" + GeneralHelper::to_string(blockSize) + "; " + blockIndVar + "++){" << std::endl;
    }

    //Emit operators
    //TODO: Implement other scheduler types
    if(sched == SchedParams::SchedType::BOTTOM_UP){
        emitSingleThreadedOpsBottomUp(design, cFile, nodesWithState, sched);
    }else if(sched == SchedParams::SchedType::TOPOLOGICAL){
        emitSingleThreadedOpsSched(design, cFile, sched);
    }else if(sched == SchedParams::SchedType::TOPOLOGICAL_CONTEXT){
        emitSingleThreadedOpsSchedStateUpdateContext(design, cFile, sched, blockSize, blockIndVar);
    }else{
        throw std::runtime_error("Unknown schedule type");
    }

    //Emit state variable updates (only if state update nodes are not included)
    if(sched != SchedParams::SchedType::TOPOLOGICAL_CONTEXT){
        cFile << std::endl << "//==== Update State Vars ====" << std::endl;
        for(unsigned long i = 0; i<nodesWithState.size(); i++){
            std::vector<std::string> stateUpdateExprs;
            nodesWithState[i]->emitCStateUpdate(stateUpdateExprs, sched, nullptr);

            unsigned long numStateUpdateExprs = stateUpdateExprs.size();
            for(unsigned long j = 0; j<numStateUpdateExprs; j++){
                cFile << stateUpdateExprs[j] << std::endl;
            }
        }
    }

    if(blockSize > 1) {
        //TODO: Increment other variables here
        cFile << "}" << std::endl;
    }

    cFile << std::endl << "//==== Return Number Outputs Generated ====" << std::endl;
    cFile << "*outputCount = 1;" << std::endl;

    cFile << "}" << std::endl;

    cFile << std::endl;

    cFile << "void " << designName << "_reset(){" << std::endl;
    cFile << "//==== Reset State Vars ====" << std::endl;
    for(unsigned long i = 0; i<nodesWithStateCount; i++){
        std::vector<Variable> stateVars = nodesWithState[i]->getCStateVars();
        //Emit State Vars
        unsigned long numStateVars = stateVars.size();
        for(unsigned long j = 0; j<numStateVars; j++){
            std::vector<std::string> rstStatements = stateVars[j].genReset();
            for(const std::string &rstStatement : rstStatements){
                cFile << rstStatement << std::endl;
            }
        }
    }

    //Call the reset functions of blackboxes if they have state
    if(blackBoxes.size() > 0) {
        cFile << "//==== Reset BlackBoxes ====" << std::endl;

        for(unsigned long i = 0; i<blackBoxes.size(); i++){
            cFile << blackBoxes[i]->getResetFunctionCall() << ";" << std::endl;
        }
    }

    cFile << "}" << std::endl;

    cFile.close();
}
