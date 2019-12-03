//
// Created by Christopher Yarp on 9/3/19.
//

#include "LocklessThreadCrossingFIFO.h"
#include "General/GeneralHelper.h"
#include "General/ErrorHelpers.h"

LocklessThreadCrossingFIFO::LocklessThreadCrossingFIFO() : cWriteOffsetPtrInitialized(false), cReadOffsetPtrInitialized(false),
cArrayPtrInitialized(false), cReadOffsetCachedInitialized(false), cWriteOffsetCachedInitialized(false) {

}

LocklessThreadCrossingFIFO::LocklessThreadCrossingFIFO(std::shared_ptr<SubSystem> parent) : ThreadCrossingFIFO(parent),
cWriteOffsetPtrInitialized(false), cReadOffsetPtrInitialized(false), cArrayPtrInitialized(false), cReadOffsetCachedInitialized(false), cWriteOffsetCachedInitialized(false) {

}

LocklessThreadCrossingFIFO::LocklessThreadCrossingFIFO(std::shared_ptr<SubSystem> parent, LocklessThreadCrossingFIFO *orig)
        : ThreadCrossingFIFO(parent, orig), cWriteOffsetPtr(orig->cWriteOffsetPtr), cReadOffsetPtr(orig->cReadOffsetPtr), cArrayPtr(orig->cArrayPtr),
        cWriteOffsetPtrInitialized(orig->cWriteOffsetPtrInitialized), cReadOffsetPtrInitialized(orig->cReadOffsetPtrInitialized), cArrayPtrInitialized(orig->cArrayPtrInitialized),
        cReadOffsetCachedInitialized(orig->cReadOffsetCachedInitialized), cWriteOffsetCachedInitialized(orig->cWriteOffsetCachedInitialized){

}

Variable LocklessThreadCrossingFIFO::getCWriteOffsetPtr() {
    bool initialized = cWriteOffsetPtrInitialized;
    LocklessThreadCrossingFIFO::initializeVarIfNotAlready(getSharedPointer(), cWriteOffsetPtr, cWriteOffsetPtrInitialized, "writeOffsetPtr");
    cWriteOffsetPtr.setAtomicVar(true);

    if(!initialized) {
        //Offset is in blocks
        DataType newDT = DataType(false, true, false, std::ceil(std::log2(2*(fifoLength+1))), 0, 1);
        newDT = newDT.getCPUStorageType();
        cWriteOffsetPtr.setDataType(newDT);
        cWriteOffsetPtr.setAtomicVar(true);
    }

    return cWriteOffsetPtr;
}

Variable LocklessThreadCrossingFIFO::getCReadOffsetPtr() {
    bool initialized = cReadOffsetPtrInitialized;
    LocklessThreadCrossingFIFO::initializeVarIfNotAlready(getSharedPointer(), cReadOffsetPtr, cReadOffsetPtrInitialized, "readOffsetPtr");
    cReadOffsetPtr.setAtomicVar(true);

    if(!initialized){
        //Offset is in blocks
        DataType newDT = DataType(false, true, false, std::ceil(std::log2(2*(fifoLength+1))), 0, 1);
        newDT = newDT.getCPUStorageType();
        cReadOffsetPtr.setDataType(newDT);
        cReadOffsetPtr.setAtomicVar(true);
    }
    return cReadOffsetPtr;
}

Variable LocklessThreadCrossingFIFO::getCArrayPtr() {
    bool initialized = cArrayPtrInitialized;
    LocklessThreadCrossingFIFO::initializeVarIfNotAlready(getSharedPointer(), cArrayPtr, cArrayPtrInitialized, "arrayPtr");
    cArrayPtr.setAtomicVar(false); //Memory ordering around the read and write pointer access is used to act as a barrier for array access.  This allows the array to not be declared as atomic

    if (!initialized){
        if (getOutputPorts().size() >= 1 && getOutputPort(0)->getArcs().size() >= 1) {
            cArrayPtr.setDataType((*(getOutputPort(0)->getArcs().begin()))->getDataType());
        } else {
            throw std::runtime_error(ErrorHelpers::genErrorStr("Should Have >= 1 Output Arc", getSharedPointer()));
        }

        cArrayPtr.setAtomicVar(false);
    }
    return cArrayPtr;
}

Variable LocklessThreadCrossingFIFO::getCWriteOffsetCached() {
    bool initialized = cWriteOffsetCachedInitialized;
    LocklessThreadCrossingFIFO::initializeVarIfNotAlready(getSharedPointer(), cWriteOffsetCached, cWriteOffsetCachedInitialized, "writeOffsetCached");
    cWriteOffsetCached.setAtomicVar(false);

    if(!initialized) {
        //Offset is in blocks
        DataType newDT = DataType(false, true, false, std::ceil(std::log2(2*(fifoLength+1))), 0, 1);
        newDT = newDT.getCPUStorageType();
        cWriteOffsetCached.setDataType(newDT);
        cWriteOffsetCached.setAtomicVar(false);
    }

    return cWriteOffsetCached;
}

Variable LocklessThreadCrossingFIFO::getCReadOffsetCached() {
    bool initialized = cReadOffsetCachedInitialized;
    LocklessThreadCrossingFIFO::initializeVarIfNotAlready(getSharedPointer(), cReadOffsetCached, cReadOffsetCachedInitialized, "readOffsetCached");
    cReadOffsetCached.setAtomicVar(false);

    if(!initialized) {
        //Offset is in blocks
        DataType newDT = DataType(false, true, false, std::ceil(std::log2(2*(fifoLength+1))), 0, 1);
        newDT = newDT.getCPUStorageType();
        cReadOffsetCached.setDataType(newDT);
        cReadOffsetCached.setAtomicVar(false);
    }

    return cReadOffsetCached;
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

std::string LocklessThreadCrossingFIFO::typeNameStr(){
    return "LocklessThreadCrossingFIFO";
}

std::string LocklessThreadCrossingFIFO::labelStr() {
    //No additional paramerters to add
    return ThreadCrossingFIFO::labelStr();
}

void LocklessThreadCrossingFIFO::validate() {
    //No new validation
    ThreadCrossingFIFO::validate();
}

std::string LocklessThreadCrossingFIFO::emitCIsNotEmpty(std::vector<std::string> &cStatementQueue, Role role) {
    //Empty = (writeOffset-readOffset == 1) || (writeOffset-readOffset==-(arrayLength-1))
    //Note: This is !Empty

    int arrayLength = (fifoLength+1);
    int checkPoint = -(arrayLength-1);

    if(role == Role::CONSUMER || role == Role::NONE){
        //Fetch the current write offset
        cStatementQueue.push_back(getCWriteOffsetCached().getCVarName(false) + " = atomic_load_explicit(" + getCWriteOffsetPtr().getCVarName(false) + ", memory_order_acquire);");
    }

    if(role == Role::PRODUCER || role == Role:: NONE){
        //Fetch the current read offset
        cStatementQueue.push_back(getCReadOffsetCached().getCVarName(false) + " = atomic_load_explicit(" + getCReadOffsetPtr().getCVarName(false) + ", memory_order_acquire);");
    }

    return "(!((" + getCWriteOffsetCached().getCVarName(false) + " - " + getCReadOffsetCached().getCVarName(false) + " == 1) || (" + getCWriteOffsetCached().getCVarName(false) + " - " + getCReadOffsetCached().getCVarName(false) + " == " + GeneralHelper::to_string(checkPoint) + ")))";
    //return "(!((" + getCWriteOffsetCached().getCVarName(false) + " - " + getCReadOffsetCached().getCVarName(false) + " == 1) || (" + getCWriteOffsetCached().getCVarName(false) + " == 0 && " + getCReadOffsetCached().getCVarName(false) + " == " + GeneralHelper::to_string(arrayLength-1) + ")))";
}

std::string LocklessThreadCrossingFIFO::emitCIsNotFull(std::vector<std::string> &cStatementQueue, Role role) {
    //Full = (readOffset == writeOffset)
    //Note: This is !Full

    if(role == Role::CONSUMER || role == Role::NONE){
        //Fetch the current write offset
        cStatementQueue.push_back(getCWriteOffsetCached().getCVarName(false) + " = atomic_load_explicit(" + getCWriteOffsetPtr().getCVarName(false) + ", memory_order_acquire);");
    }

    if(role == Role::PRODUCER || role == Role:: NONE){
        //Fetch the current read offset
        cStatementQueue.push_back(getCReadOffsetCached().getCVarName(false) + " = atomic_load_explicit(" + getCReadOffsetPtr().getCVarName(false) + ", memory_order_acquire);");
    }

    return "(" + getCReadOffsetCached().getCVarName(false) + " != " + getCWriteOffsetCached().getCVarName(false) + ")";
}

std::string LocklessThreadCrossingFIFO::emitCNumBlocksAvailToRead(std::vector<std::string> &cStatementQueue, Role role) {
    //Avail to read: ((readOffset < writeOffset) ? writeOffset - readOffset - 1 : arrayLength - readOffset + writeOffset - 1)

    if(role == Role::CONSUMER || role == Role::NONE){
        //Fetch the current write offset
        cStatementQueue.push_back(getCWriteOffsetCached().getCVarName(false) + " = atomic_load_explicit(" + getCWriteOffsetPtr().getCVarName(false) + ", memory_order_acquire);");
    }

    if(role == Role::PRODUCER || role == Role:: NONE){
        //Fetch the current read offset
        cStatementQueue.push_back(getCReadOffsetCached().getCVarName(false) + " = atomic_load_explicit(" + getCReadOffsetPtr().getCVarName(false) + ", memory_order_acquire);");
    }

    int arrayLength = fifoLength+1;

    return "((" + getCReadOffsetCached().getCVarName(false) + " < " + getCWriteOffsetCached().getCVarName(false) + ") ? " + getCWriteOffsetCached().getCVarName(false) + " - " + getCReadOffsetCached().getCVarName(false) + " - 1 : " + GeneralHelper::to_string(arrayLength) + " - " + getCReadOffsetCached().getCVarName(false) + " + " + getCWriteOffsetCached().getCVarName(false) + " - 1)";
}

std::string LocklessThreadCrossingFIFO::emitCNumBlocksAvailToWrite(std::vector<std::string> &cStatementQueue, Role role) {
    //Space Left: ((readOffset < writeOffset) ? arrayLength - writeOffset + readOffset : readOffset - writeOffset)

    if(role == Role::CONSUMER || role == Role::NONE){
        //Fetch the current write offset
        cStatementQueue.push_back(getCWriteOffsetCached().getCVarName(false) + " = atomic_load_explicit(" + getCWriteOffsetPtr().getCVarName(false) + ", memory_order_acquire);");
    }

    if(role == Role::PRODUCER || role == Role:: NONE){
        //Fetch the current read offset
        cStatementQueue.push_back(getCReadOffsetCached().getCVarName(false) + " = atomic_load_explicit(" + getCReadOffsetPtr().getCVarName(false) + ", memory_order_acquire);");
    }

    int arrayLength = fifoLength+1;

    return "((" + getCReadOffsetCached().getCVarName(false) + " < " + getCWriteOffsetCached().getCVarName(false) + ") ? " + GeneralHelper::to_string(arrayLength) + " - " + getCWriteOffsetCached().getCVarName(false) + " + " + getCReadOffsetCached().getCVarName(false) + " : " + getCReadOffsetCached().getCVarName(false) + " - " + getCWriteOffsetCached().getCVarName(false) + ")";
}

void
LocklessThreadCrossingFIFO::emitCWriteToFIFO(std::vector<std::string> &cStatementQueue, std::string src, int numBlocks, Role role, bool pushStateAfter) {
    //TODO: Consider optimizing if this becomes the bottleneck
    //It appears that memcpy drops the volatile designations which is an issue for this use case

    int arrayLengthBlocks = (fifoLength+1);

    std::string localWriteOffsetBlocks = getCWriteOffsetPtr().getCVarName(false)+"_local";
    std::string derefSharedWriteOffsetBlocks = "atomic_load_explicit(" + getCWriteOffsetPtr().getCVarName(false) + ", memory_order_acquire)";
    std::string arrayName = getCArrayPtr().getCVarName(false);
    std::string srcName = src;

    if(numBlocks == 1){
        //Read 1 full block
        //Open a block for the write to prevent scoping issues with declared temp
        cStatementQueue.push_back("{//Begin Scope for " + name + " FIFO Write");
        if(role == ThreadCrossingFIFO::Role::NONE) {
            cStatementQueue.push_back("//Load Write Ptr");
            cStatementQueue.push_back("int " + localWriteOffsetBlocks + " = " + derefSharedWriteOffsetBlocks + ";"); //Elements and blocks are the same
            cStatementQueue.push_back("");
        }else{
            //Acquire occurred earlier in cache update
            cStatementQueue.push_back("int " + localWriteOffsetBlocks + " = " + getCWriteOffsetCached().getCVarName(false) + ";"); //Elements and blocks are the same
        }
        cStatementQueue.push_back("//Write into array");
        cStatementQueue.push_back(arrayName + "[" + localWriteOffsetBlocks + "] = " + srcName + ";");
        cStatementQueue.push_back("if (" + localWriteOffsetBlocks + " >= " + GeneralHelper::to_string(arrayLengthBlocks - 1) + ") {"); //Elements and blocks are the same
        cStatementQueue.push_back(localWriteOffsetBlocks + " = 0;");
        cStatementQueue.push_back("} else {");
        cStatementQueue.push_back(localWriteOffsetBlocks + "++;");
        cStatementQueue.push_back("}");
        cStatementQueue.push_back(getCWriteOffsetCached().getCVarName(false) + " = " + localWriteOffsetBlocks + ";");
        if(pushStateAfter){
            cStatementQueue.push_back("//Update Write Ptr");
            cStatementQueue.push_back("atomic_store_explicit(" + getCWriteOffsetPtr().getCVarName(false) + ", " + localWriteOffsetBlocks + ", memory_order_release);"); //Elements and blocks are the same
        }
        //If not, a release will occur when the value is written
        cStatementQueue.push_back("}//End Scope for " + name + " FIFO Write");
    }else{
        //Open a block for the write to prevent scoping issues with declared temp
        cStatementQueue.push_back("{//Begin Scope for " + name + " FIFO Write");
        if(role == ThreadCrossingFIFO::Role::NONE) {
            cStatementQueue.push_back("//Load Write Ptr");
            cStatementQueue.push_back("int " + localWriteOffsetBlocks + " = " + derefSharedWriteOffsetBlocks + ";"); //Elements and blocks are the same
            cStatementQueue.push_back("");
        }else{
            //Acquire occurred earlier in cache update
            cStatementQueue.push_back("int " + localWriteOffsetBlocks + " = " + getCWriteOffsetCached().getCVarName(false) + ";"); //Elements and blocks are the same
        }
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
        cStatementQueue.push_back(getCWriteOffsetCached().getCVarName(false) + " = " + localWriteOffsetBlocks + ";");
        if(pushStateAfter) {
            cStatementQueue.push_back("//Update Write Ptr");
            cStatementQueue.push_back("atomic_store_explicit(" + getCWriteOffsetPtr().getCVarName(false) + ", " + localWriteOffsetBlocks + ", memory_order_release);"); //Elements and blocks are the same
        }
        //If not, a release will occur when the value is written
        cStatementQueue.push_back("}//End Scope for " + name + " FIFO Write");
    }
}

void
LocklessThreadCrossingFIFO::emitCReadFromFIFO(std::vector<std::string> &cStatementQueue, std::string dst, int numBlocks, Role role, bool pushStateAfter) {
    //TODO: Consider optimizing if this becomes the bottleneck
    //It appears that memcpy drops the volatile designations which is an issue for this use case

    //TODO: Consider optimizing if this becomes the bottleneck
    //It appears that memcpy drops the volatile designations which is an issue for this use case

    int arrayLengthBlocks = (fifoLength+1);

    std::string localReadOffsetBlocks = getCReadOffsetPtr().getCVarName(false)+"_local";
    std::string derefSharedReadOffsetBlocks = "atomic_load_explicit(" + getCReadOffsetPtr().getCVarName(false) + ", memory_order_acquire)";
    std::string arrayName = getCArrayPtr().getCVarName(false);
    std::string dstName = dst;

    if(numBlocks == 1){
        //Read an entire blocks
        //Open a block for the write to prevent scoping issues with declared temp
        cStatementQueue.push_back("{//Begin Scope for " + name + " FIFO Read");
        if(role == ThreadCrossingFIFO::Role::NONE) {
            cStatementQueue.push_back("//Load Read Ptr");
            cStatementQueue.push_back("int " + localReadOffsetBlocks + " = " + derefSharedReadOffsetBlocks + ";"); //Elements and blocks are the same
        }else{
            //Acquire occurred earlier in cache update
            cStatementQueue.push_back("int " + localReadOffsetBlocks + " = " + getCReadOffsetCached().getCVarName(false) + ";"); //Elements and blocks are the same
        }
        //Read pointer is at the position of the last read value. Needs to be incremented before read
        cStatementQueue.push_back("if (" + localReadOffsetBlocks + " >= " + GeneralHelper::to_string(arrayLengthBlocks - 1) + ") {"); //Elements and blocks are the same
        cStatementQueue.push_back(localReadOffsetBlocks + " = 0;");
        cStatementQueue.push_back("} else {");
        cStatementQueue.push_back(localReadOffsetBlocks + "++;");
        cStatementQueue.push_back("}");
        cStatementQueue.push_back("");
        cStatementQueue.push_back("//Read from array");
        cStatementQueue.push_back(dstName + " = " + arrayName + "[" + localReadOffsetBlocks + "];");
        cStatementQueue.push_back(getCReadOffsetCached().getCVarName(false) + " = " + localReadOffsetBlocks + ";");
        if(pushStateAfter) {
            cStatementQueue.push_back("//Update Read Ptr");
            cStatementQueue.push_back("atomic_store_explicit(" + getCReadOffsetPtr().getCVarName(false) + ", " + localReadOffsetBlocks + ", memory_order_release);"); //Elements and blocks are the same
        }
        //If not, a release will occur when the value is written
        cStatementQueue.push_back("}//End Scope for " + name + " FIFO Read");
    }else{
        //Open a block for the write to prevent scoping issues with declared temp
        cStatementQueue.push_back("{//Begin Scope for " + name + " FIFO Read");
        if(role == ThreadCrossingFIFO::Role::NONE) {
            cStatementQueue.push_back("//Load Read Ptr");
            cStatementQueue.push_back("int " + localReadOffsetBlocks + " = " + derefSharedReadOffsetBlocks + ";");
            cStatementQueue.push_back("");
        }else{
            //Acquire occurred earlier in cache update
            cStatementQueue.push_back("int " + localReadOffsetBlocks + " = " + getCReadOffsetCached().getCVarName(false) + ";"); //Elements and blocks are the same
        }
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
        cStatementQueue.push_back(getCReadOffsetCached().getCVarName(false) + " = " + localReadOffsetBlocks + ";");
        if(pushStateAfter) {
            cStatementQueue.push_back("//Update Read Ptr");
            cStatementQueue.push_back("atomic_store_explicit(" + getCReadOffsetPtr().getCVarName(false) + ", " + localReadOffsetBlocks + ", memory_order_release);"); //Elements and blocks are the same
        }
        //If not, a release will occur when the value is written
        cStatementQueue.push_back("}//End Scope for " + name + " FIFO Read");
    }
}

std::vector<std::pair<Variable, std::string>> LocklessThreadCrossingFIFO::getFIFOSharedVariables() {
    std::vector<std::pair<Variable, std::string>> vars;
    vars.push_back(std::pair<Variable, std::string>(getCReadOffsetPtr(), ""));
    vars.push_back(std::pair<Variable, std::string>(getCWriteOffsetPtr(), ""));
    vars.push_back(std::pair<Variable, std::string>(getCArrayPtr(), getFIFOStructTypeName()));

    return vars;
}

void LocklessThreadCrossingFIFO::createSharedVariables(std::vector<std::string> &cStatementQueue, int core) {
    //Will declare the shared vars.  References to these should be passed (using & for the pointers and directly for the )

    std::string cReadOffsetDT = (getCReadOffsetPtr().isAtomicVar() ? "_Atomic " : "") + getCReadOffsetPtr().getDataType().getCPUStorageType().toString(DataType::StringStyle::C, false, false);
    if(core < 0) {
        cStatementQueue.push_back(
                cReadOffsetDT + "* " + getCReadOffsetPtr().getCVarName(false) + " = (" + cReadOffsetDT +
                "*) vitis_aligned_alloc(VITIS_MEM_ALIGNMENT, sizeof(" + cReadOffsetDT + "));");
    }else{
        cStatementQueue.push_back(
                cReadOffsetDT + "* " + getCReadOffsetPtr().getCVarName(false) + " = (" + cReadOffsetDT +
                "*) vitis_aligned_alloc_core(VITIS_MEM_ALIGNMENT, sizeof(" + cReadOffsetDT + "), " +
                GeneralHelper::to_string(core) + ");");
    }

    std::string cWriteOffsetDT = (getCWriteOffsetPtr().isAtomicVar() ? "_Atomic " : "") + getCWriteOffsetPtr().getDataType().getCPUStorageType().toString(DataType::StringStyle::C, false, false);

    if(core < 0) {
        cStatementQueue.push_back(
                "*) vitis_aligned_alloc(VITIS_MEM_ALIGNMENT, sizeof(" + cWriteOffsetDT + "));");
    }else{
        cStatementQueue.push_back(
                cWriteOffsetDT + "* " + getCWriteOffsetPtr().getCVarName(false) + " = (" + cWriteOffsetDT +
                "*) vitis_aligned_alloc_core(VITIS_MEM_ALIGNMENT, sizeof(" + cWriteOffsetDT + "), " +
                GeneralHelper::to_string(core) + ");");
    }

    std::string cArrayDT = getFIFOStructTypeName();

    if(core < 0) {
        cStatementQueue.push_back(cArrayDT + "* " + getCArrayPtr().getCVarName(false) + " = (" + cArrayDT +
                                  "*) vitis_aligned_alloc(VITIS_MEM_ALIGNMENT, sizeof(" + cArrayDT + ")*" +
                                  GeneralHelper::to_string(fifoLength + 1) +
                                  ");"); //The Array length is 1 larger than the FIFO length.
    }else{
        cStatementQueue.push_back(cArrayDT + "* " + getCArrayPtr().getCVarName(false) + " = (" + cArrayDT +
                                  "*) vitis_aligned_alloc_core(VITIS_MEM_ALIGNMENT, sizeof(" + cArrayDT + ")*" +
                                  GeneralHelper::to_string(fifoLength + 1) +
                                  ", " + GeneralHelper::to_string(core) + ");"); //The Array length is 1 larger than the FIFO length.
    }
}

void LocklessThreadCrossingFIFO::cleanupSharedVariables(std::vector<std::string> &cStatementQueue) {
    cStatementQueue.push_back("free(" + getCReadOffsetPtr().getCVarName(false) + ");");
    cStatementQueue.push_back("free(" + getCWriteOffsetPtr().getCVarName(false) + ");");
    cStatementQueue.push_back("free(" + getCArrayPtr().getCVarName(false) + ");");
}

void LocklessThreadCrossingFIFO::initializeSharedVariables(std::vector<std::string> &cStatementQueue) {
    DataType arrayNumericType = getCArrayPtr().getDataType().getCPUStorageType();

    //It should be validated at this point that all ports have the same number of initial conditions
    for(int i = 0; i<getInitConditionsCreateIfNot(0).size(); i++){
        int blockInd = i/blockSize;
        int elementInd = i%blockSize;

        for(int portNum = 0; portNum<inputPorts.size(); portNum++) {
            if(getInitConditionsCreateIfNot(portNum).size() > fifoLength*blockSize != 0){
                throw std::runtime_error(ErrorHelpers::genErrorStr("The number of initial conditions in a FIFO must <= the length of the FIFO>", getSharedPointer()));
            }

            if(getInitConditionsCreateIfNot(portNum).size() % blockSize != 0){
                throw std::runtime_error(ErrorHelpers::genErrorStr("The number of initial conditions in a FIFO must be a multiple of its block size", getSharedPointer()));
            }

            //Note, the block index starts at 1 for initialization
            if (blockSize == 1) {
                cStatementQueue.push_back(
                        getCArrayPtr().getCVarName(false) + "[" + GeneralHelper::to_string(blockInd + 1) + "].port" + GeneralHelper::to_string(portNum) + "_real = " +
                        GeneralHelper::to_string(getInitConditionsCreateIfNot(portNum)[i].toStringComponent(false, arrayNumericType)) + ";");
                if (arrayNumericType.isComplex()) {
                    cStatementQueue.push_back(
                            getCArrayPtr().getCVarName(false) + "[" + GeneralHelper::to_string(blockInd + 1) +
                            "].port" + GeneralHelper::to_string(portNum) + "_imag = " +
                            GeneralHelper::to_string(getInitConditionsCreateIfNot(portNum)[i].toStringComponent(true, arrayNumericType)) +
                            ";");
                }
            } else {
                cStatementQueue.push_back(
                        getCArrayPtr().getCVarName(false) + "[" + GeneralHelper::to_string(blockInd + 1) + "].port" + GeneralHelper::to_string(portNum) + "_real[" +
                        GeneralHelper::to_string(elementInd) + "] = " +
                        GeneralHelper::to_string(getInitConditionsCreateIfNot(portNum)[i].toStringComponent(false, arrayNumericType)) + ";");
                if (arrayNumericType.isComplex()) {
                    cStatementQueue.push_back(
                            getCArrayPtr().getCVarName(false) + "[" + GeneralHelper::to_string(blockInd + 1) +
                            "].port" + GeneralHelper::to_string(portNum) + "_imag[" + GeneralHelper::to_string(elementInd) + "] = " +
                            GeneralHelper::to_string(getInitConditionsCreateIfNot(portNum)[i].toStringComponent(true, arrayNumericType)) +
                            ";");
                }
            }
        }
    }

    //Read index always initialized to 0
    cStatementQueue.push_back("atomic_init(" + getCReadOffsetPtr().getCVarName(false) + ", 0);");
    cStatementQueue.push_back("if(!atomic_is_lock_free(" + getCReadOffsetPtr().getCVarName(false) + ")){");
    cStatementQueue.push_back("printf(\"Warning: An atomic FIFO offset (" + getCReadOffsetPtr().getCVarName(false) + ") was expected to be lock free but is not\\n\");");
    cStatementQueue.push_back("}");

    //Write pointer initialized to (init.size()+1)%arrayLength = (init.size()+1)%(fifoLength+1)
    //Should be validated at this point that all ports have the same number of init conditions
    int arrayLength=fifoLength+1;
    int writeInd = (getInitConditionsCreateIfNot(0).size()/blockSize+1)%arrayLength;
    cStatementQueue.push_back("atomic_init(" + getCWriteOffsetPtr().getCVarName(false) + ", " + GeneralHelper::to_string(writeInd) + ");");
    cStatementQueue.push_back("if(!atomic_is_lock_free(" + getCWriteOffsetPtr().getCVarName(false) + ")){");
    cStatementQueue.push_back("printf(\"Warning: An atomic FIFO offset (" + getCWriteOffsetPtr().getCVarName(false) + ") was expected to be lock free but is not\\n\");");
    cStatementQueue.push_back("}");
}

void LocklessThreadCrossingFIFO::createLocalVars(std::vector<std::string> &cStatementQueue){
    cStatementQueue.push_back(getCWriteOffsetCached().getCVarDecl(false) + ";");
    cStatementQueue.push_back(getCReadOffsetCached().getCVarDecl(false) + ";");
}

void LocklessThreadCrossingFIFO::initLocalVars(std::vector<std::string> &cStatementQueue, Role role){
    pullLocalVars(cStatementQueue, role);
}

void LocklessThreadCrossingFIFO::pullLocalVars(std::vector<std::string> &cStatementQueue, Role role){
    if(role == Role::CONSUMER || role == Role::NONE){
        //Fetch the current write offset
        cStatementQueue.push_back(getCWriteOffsetCached().getCVarName(false) + " = atomic_load_explicit(" + getCWriteOffsetPtr().getCVarName(false) + ", memory_order_acquire);");
    }

    if(role == Role::PRODUCER || role == Role::NONE){
        //Fetch the current read offset
        cStatementQueue.push_back(getCReadOffsetCached().getCVarName(false) + " = atomic_load_explicit(" + getCReadOffsetPtr().getCVarName(false) + ", memory_order_acquire);");
    }
}

void LocklessThreadCrossingFIFO::pushLocalVars(std::vector<std::string> &cStatementQueue, Role role){
    if(role == Role::CONSUMER || role == Role::NONE){
        //Push the current read offset
        cStatementQueue.push_back("atomic_store_explicit(" + getCReadOffsetPtr().getCVarName(false) + ", " + getCReadOffsetCached().getCVarName(false) + ", memory_order_release);");
    }

    if(role == Role::PRODUCER || role == Role::NONE){
        //Push the current write offset
        cStatementQueue.push_back("atomic_store_explicit(" + getCWriteOffsetPtr().getCVarName(false) + ", " + getCWriteOffsetCached().getCVarName(false) + ", memory_order_release);");
    }
}

std::set<std::string> LocklessThreadCrossingFIFO::getExternalIncludes() {
    std::set<std::string> includes = Node::getExternalIncludes();
    includes.insert("#include <stdatomic.h>");
    includes.insert("#include <stdio.h>");
    return includes;
}
