//
// Created by Christopher Yarp on 9/3/19.
//

#include "LocklessThreadCrossingFIFO.h"
#include "General/GeneralHelper.h"
#include "General/ErrorHelpers.h"

LocklessThreadCrossingFIFO::LocklessThreadCrossingFIFO() : cWriteOffsetPtrInitialized(false), cReadOffsetPtrInitialized(false),
cArrayPtrInitialized(false) {

}

LocklessThreadCrossingFIFO::LocklessThreadCrossingFIFO(std::shared_ptr<SubSystem> parent) : ThreadCrossingFIFO(parent),
cWriteOffsetPtrInitialized(false), cReadOffsetPtrInitialized(false), cArrayPtrInitialized(false) {

}

LocklessThreadCrossingFIFO::LocklessThreadCrossingFIFO(std::shared_ptr<SubSystem> parent, LocklessThreadCrossingFIFO *orig)
        : ThreadCrossingFIFO(parent, orig), cWriteOffsetPtr(orig->cWriteOffsetPtr), cReadOffsetPtr(orig->cReadOffsetPtr), cArrayPtr(orig->cArrayPtr),
        cWriteOffsetPtrInitialized(orig->cWriteOffsetPtrInitialized), cReadOffsetPtrInitialized(orig->cReadOffsetPtrInitialized), cArrayPtrInitialized(orig->cArrayPtrInitialized) {

}

void LocklessThreadCrossingFIFO::initializeVarIfNotAlready(std::shared_ptr<Node> node, Variable &var, bool &init, std::string suffix){
    ThreadCrossingFIFO::initializeVarIfNotAlready(node, var, init, suffix);
    var.setVolatileVar(true);
}

Variable LocklessThreadCrossingFIFO::getCWriteOffsetPtr() {
    if(!cWriteOffsetPtrInitialized) {
        int elementLength = fifoLength*blockSize;

        DataType newDT = DataType(false, false, false, std::ceil(std::log2(elementLength+1)), 0, 1);
        newDT = newDT.getCPUStorageType();
        cWriteOffsetPtr.setDataType(newDT);
    }

    LocklessThreadCrossingFIFO::initializeVarIfNotAlready(getSharedPointer(), cWriteOffsetPtr, cWriteOffsetPtrInitialized, "writeOffsetPtr");
    return cWriteOffsetPtr;
}

Variable LocklessThreadCrossingFIFO::getCReadOffsetPtr() {
    if(!cReadOffsetPtrInitialized){
        int elementLength = fifoLength*blockSize;

        DataType newDT = DataType(false, false, false, std::ceil(std::log2(elementLength+1)), 0, 1);
        newDT = newDT.getCPUStorageType();
        cReadOffsetPtr.setDataType(newDT);
    }

    LocklessThreadCrossingFIFO::initializeVarIfNotAlready(getSharedPointer(), cReadOffsetPtr, cReadOffsetPtrInitialized, "readOffsetPtr");
    return cReadOffsetPtr;
}

Variable LocklessThreadCrossingFIFO::getCArrayPtr() {
    if (!cArrayPtrInitialized){
        if (getOutputPorts().size() == 1 && getOutputPort(0)->getArcs().size() >= 1) {
            cArrayPtr.setDataType((*(getOutputPort(0)->getArcs().begin()))->getDataType());
        } else {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Should Have >= 1 Output Arc", getSharedPointer()));
        }
    }

    LocklessThreadCrossingFIFO::initializeVarIfNotAlready(getSharedPointer(), cArrayPtr, cArrayPtrInitialized, "arrayPtr");

    return cArrayPtr;
}

std::shared_ptr<LocklessThreadCrossingFIFO> LocklessThreadCrossingFIFO::createFromGraphML(int id, std::string name,
                                                                                          std::map<std::string, std::string> dataKeyValueMap,
                                                                                          std::shared_ptr<SubSystem> parent,
                                                                                          GraphMLDialect dialect) {
    std::shared_ptr<LocklessThreadCrossingFIFO> newNode = NodeFactory::createNode<LocklessThreadCrossingFIFO>(parent);
    newNode->setId(id);
    newNode->setName(name);

    if (dialect != GraphMLDialect::VITIS) {
            throw std::runtime_error(ErrorHelpers::genErrorStr("LocklessThreadCrossingFIFO is only supported in the vitis GraphML dialect", newNode));
    }

    //Populate properties from ThreadCrossingFIFO superclass
    newNode->populatePropertiesFromGraphML(dataKeyValueMap, parent, dialect);

    //==== Import important properties for this class ====
    //The variables are based on the name.  No additional parameters needed for import

    return newNode;
}

std::set<GraphMLParameter> LocklessThreadCrossingFIFO::graphMLParameters() {
    return ThreadCrossingFIFO::graphMLParameters();

    //No additional parameters that need to be stored in XML (variables based on node name)
}

xercesc::DOMElement *LocklessThreadCrossingFIFO::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode,
                                                             bool include_block_node_type) {
    xercesc::DOMElement* thisNode = emitGraphMLBasics(doc, graphNode);
    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "ThreadCrossingFIFO");
    }

    GraphMLHelper::addDataNode(doc, thisNode, "block_function", "LocklessThreadCrossingFIFO");

    //Emit properties from the superclass
    emitPropertiesToGraphML(doc, graphNode);

    //No additional parameters that need to be stored in XML (variables based on node name)

    return thisNode;
}

std::shared_ptr<Node> LocklessThreadCrossingFIFO::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<LocklessThreadCrossingFIFO>(parent, this);
}

std::string LocklessThreadCrossingFIFO::labelStr() {
    //No additional paramerters to add
    return ThreadCrossingFIFO::labelStr();
}

void LocklessThreadCrossingFIFO::validate() {
    //No new validation
    ThreadCrossingFIFO::validate();
}

std::string LocklessThreadCrossingFIFO::emitCIsNotEmpty() {
    //Empty = (writeOffset-readOffset == 1) || (writeOffset-readOffset==-(arrayLength-1))
    //Note: This is !Empty

    int arrayLength = (fifoLength+1);
    int checkPoint = -(arrayLength-1);

    std::string derefWriteOffset = "*" + cWriteOffsetPtr.getCVarName(false);
    std::string derefReadOffset = "*" + cReadOffsetPtr.getCVarName(false);

    return "(!((" + derefWriteOffset + " - " + derefReadOffset + " == 1) || (" + derefWriteOffset + " - " + derefReadOffset + " == " + GeneralHelper::to_string(checkPoint) + ")))";
}

std::string LocklessThreadCrossingFIFO::emitCIsNotFull() {
    //Full = (readOffset == writeOffset)
    //Note: This is !Full

    std::string derefWriteOffset = "*" + cWriteOffsetPtr.getCVarName(false);
    std::string derefReadOffset = "*" + cReadOffsetPtr.getCVarName(false);

    return "(" + derefReadOffset + " != " + derefWriteOffset + ")";
}

std::string LocklessThreadCrossingFIFO::emitCNumBlocksAvailToRead() {
    //Avail to read: ((readOffset < writeOffset) ? writeOffset - readOffset - 1 : arrayLength - readOffset + writeOffset - 1)

    std::string derefWriteOffset = "*" + cWriteOffsetPtr.getCVarName(false);
    std::string derefReadOffset = "*" + cReadOffsetPtr.getCVarName(false);

    int arrayLength = fifoLength+1;

    return "((" + derefReadOffset + " < " + derefWriteOffset + ") ? " + derefWriteOffset + " - " + derefReadOffset + " - 1 : " + GeneralHelper::to_string(arrayLength) + " - " + derefReadOffset + " + " + derefWriteOffset + " - 1)";
}

std::string LocklessThreadCrossingFIFO::emitCNumBlocksAvailToWrite() {
    //Space Left: ((readOffset < writeOffset) ? arrayLength - writeOffset + readOffset : readOffset - writeOffset)
    std::string derefWriteOffset = "*" + cWriteOffsetPtr.getCVarName(false);
    std::string derefReadOffset = "*" + cReadOffsetPtr.getCVarName(false);

    int arrayLength = fifoLength+1;

    return "((" + derefReadOffset + " < " + derefWriteOffset + ") ? " + GeneralHelper::to_string(arrayLength) + " - " + derefWriteOffset + " + " + derefReadOffset + " : " + derefReadOffset + " - " + derefWriteOffset + ")";
}

void
LocklessThreadCrossingFIFO::emitCWriteToFIFO(std::vector<std::string> &cStatementQueue, std::string src, int numBlocks) {
    //TODO: Consider optimizing if this becomes the bottleneck
    //It appears that memcpy drops the volatile designations which is an issue for this use case

    int arrayLengthBlocks = (fifoLength+1);

    std::string localWriteOffsetBlocks = cWriteOffsetPtr.getCVarName(false)+"_local";
    std::string derefSharedWriteOffsetBlocks = "*" + cWriteOffsetPtr.getCVarName(false);
    std::string arrayName = cArrayPtr.getCVarName(false);
    std::string srcName = src;

    if(numBlocks == 1){
        //Read 1 full block
        //Open a block for the write to prevent scoping issues with declared temp
        cStatementQueue.push_back("{");
        cStatementQueue.push_back("//Load Write Ptr");
        cStatementQueue.push_back("int " + localWriteOffsetBlocks + " = " + derefSharedWriteOffsetBlocks + ";"); //Elements and blocks are the same
        cStatementQueue.push_back("");
        cStatementQueue.push_back("//Write into array");
        cStatementQueue.push_back(arrayName + "[" + localWriteOffsetBlocks + "] = " + srcName + ";");
        cStatementQueue.push_back("if (" + localWriteOffsetBlocks + " >= " + GeneralHelper::to_string(arrayLengthBlocks - 1) + ") {"); //Elements and blocks are the same
        cStatementQueue.push_back(localWriteOffsetBlocks + " = 0;");
        cStatementQueue.push_back("} else {");
        cStatementQueue.push_back(localWriteOffsetBlocks + "++;");
        cStatementQueue.push_back("}");
        cStatementQueue.push_back("//Update Write Ptr");
        cStatementQueue.push_back(derefSharedWriteOffsetBlocks + " = " + localWriteOffsetBlocks + ";"); //Elements and blocks are the same
        cStatementQueue.push_back("}");
    }else{
        //Open a block for the write to prevent scoping issues with declared temp
        cStatementQueue.push_back("{");
        cStatementQueue.push_back("//Load Write Ptr");
        cStatementQueue.push_back("int " + localWriteOffsetBlocks + " = " + derefSharedWriteOffsetBlocks + ";");
        cStatementQueue.push_back("");
        cStatementQueue.push_back("//Write into array");
        cStatementQueue.push_back("for (int32_t i = 0; i < " + GeneralHelper::to_string(numBlocks) + "; i++){");
        cStatementQueue.push_back(arrayName + "[" + localWriteOffsetBlocks +"] = " + srcName + "[i];");
        //Handle the mod with a branch as it should be cheaper
        cStatementQueue.push_back("if (" + localWriteOffsetBlocks + " >= " + GeneralHelper::to_string(arrayLengthBlocks - 1) + ") {");
        cStatementQueue.push_back(localWriteOffsetBlocks + " = 0;");
        cStatementQueue.push_back("} else {");
        cStatementQueue.push_back(localWriteOffsetBlocks + "++;");
        cStatementQueue.push_back("}");
        cStatementQueue.push_back("}");
        cStatementQueue.push_back("");
        cStatementQueue.push_back("//Update Write Ptr");
        cStatementQueue.push_back(derefSharedWriteOffsetBlocks + " = " + localWriteOffsetBlocks + ";");
        cStatementQueue.push_back("}");
    }
}

void
LocklessThreadCrossingFIFO::emitCReadFromFIFO(std::vector<std::string> &cStatementQueue, std::string dst, int numBlocks) {
    //TODO: Consider optimizing if this becomes the bottleneck
    //It appears that memcpy drops the volatile designations which is an issue for this use case

    //TODO: Consider optimizing if this becomes the bottleneck
    //It appears that memcpy drops the volatile designations which is an issue for this use case

    int arrayLengthBlocks = (fifoLength+1);

    std::string localReadOffsetBlocks = cReadOffsetPtr.getCVarName(false)+"_local";
    std::string derefSharedReadOffsetBlocks = "*" + cReadOffsetPtr.getCVarName(false);
    std::string arrayName = cArrayPtr.getCVarName(false);
    std::string dstName = dst;

    if(numBlocks == 1){
        //Read an entire blocks
        //Open a block for the write to prevent scoping issues with declared temp
        cStatementQueue.push_back("{");
        cStatementQueue.push_back("//Load Read Ptr");
        cStatementQueue.push_back("int " + localReadOffsetBlocks + " = " + derefSharedReadOffsetBlocks + ";"); //Elements and blocks are the same
        //Read pointer is at the position of the last read value. Needs to be incremented before read
        cStatementQueue.push_back("if (" + localReadOffsetBlocks + " >= " + GeneralHelper::to_string(arrayLengthBlocks - 1) + ") {"); //Elements and blocks are the same
        cStatementQueue.push_back(localReadOffsetBlocks + " = 0;");
        cStatementQueue.push_back("} else {");
        cStatementQueue.push_back(localReadOffsetBlocks + "++;");
        cStatementQueue.push_back("}");
        cStatementQueue.push_back("");
        cStatementQueue.push_back("//Read from array");
        cStatementQueue.push_back(dstName + " = " + arrayName + "[" + localReadOffsetBlocks + "];");
        cStatementQueue.push_back("//Update Read Ptr");
        cStatementQueue.push_back(derefSharedReadOffsetBlocks + " = " + localReadOffsetBlocks + ";"); //Elements and blocks are the same
        cStatementQueue.push_back("}");
    }else{
        //Open a block for the write to prevent scoping issues with declared temp
        cStatementQueue.push_back("{");
        cStatementQueue.push_back("//Load Read Ptr");
        cStatementQueue.push_back("int " + localReadOffsetBlocks + " = " + derefSharedReadOffsetBlocks + ";");
        cStatementQueue.push_back("");
        cStatementQueue.push_back("//Read from array");
        cStatementQueue.push_back("for (int32_t i = 0; i < " + GeneralHelper::to_string(numBlocks) + "; i++){");
        //Read pointer is at the position of the last read value. Needs to be incremented before read
        //Handle the mod with a branch as it should be cheaper
        cStatementQueue.push_back("if (" + localReadOffsetBlocks + " >= " + GeneralHelper::to_string(arrayLengthBlocks - 1) + ") {");
        cStatementQueue.push_back(localReadOffsetBlocks + " = 0;");
        cStatementQueue.push_back("} else {");
        cStatementQueue.push_back(localReadOffsetBlocks + "++;");
        cStatementQueue.push_back("}");
        cStatementQueue.push_back(dstName + "[i] = " + arrayName + "[" + localReadOffsetBlocks + "];");
        cStatementQueue.push_back("}");
        cStatementQueue.push_back("");
        cStatementQueue.push_back("//Update Read Ptr");
        cStatementQueue.push_back(derefSharedReadOffsetBlocks + " = " + localReadOffsetBlocks + ";");
        cStatementQueue.push_back("}");
    }
}

std::vector<Variable> LocklessThreadCrossingFIFO::getFIFOSharedVariables() {
    std::vector<Variable> vars;
    vars.push_back(cReadOffsetPtr);
    vars.push_back(cWriteOffsetPtr);
    vars.push_back(cArrayPtr);

    return vars;
}

void LocklessThreadCrossingFIFO::createSharedVariables(std::vector<std::string> &cStatementQueue) {
    //Will declare the shared vars.  References to these should be passed (using & for the pointers and directly for the )

    std::string cReadOffsetDT = cReadOffsetPtr.getDataType().getCPUStorageType().toString(DataType::StringStyle::C, false, false);
    cStatementQueue.push_back(cReadOffsetDT + "* " + cReadOffsetPtr.getCVarName(false) + " = new " + cReadOffsetDT + ";");

    std::string cWriteOffsetDT = cWriteOffsetPtr.getDataType().getCPUStorageType().toString(DataType::StringStyle::C, false, false);
    cStatementQueue.push_back(cWriteOffsetDT + "* " + cWriteOffsetPtr.getCVarName(false) + " = new " + cWriteOffsetDT + ";");

    std::string cArrayDT;
    std::string cArrayDTWithSize;

    cArrayDT = getFIFOStructTypeName();
    cArrayDTWithSize = cArrayDT + "[" + GeneralHelper::to_string(fifoLength) + "]";
    cStatementQueue.push_back(cArrayDT + "* " + cArrayPtr.getCVarName(false) + " = new " + cArrayDTWithSize + ";");
}

void LocklessThreadCrossingFIFO::cleanupSharedVariables(std::vector<std::string> &cStatementQueue) {
    cStatementQueue.push_back("delete " + cReadOffsetPtr.getCVarName(false) + ";");
    cStatementQueue.push_back("delete " + cWriteOffsetPtr.getCVarName(false) + ";");
    cStatementQueue.push_back("delete[] " + cArrayPtr.getCVarName(false) + ";");
}

