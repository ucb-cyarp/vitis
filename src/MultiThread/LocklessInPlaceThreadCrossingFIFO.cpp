//
// Created by Christopher Yarp on 11/28/20.
//

#include "LocklessInPlaceThreadCrossingFIFO.h"
#include "General/ErrorHelpers.h"

LocklessInPlaceThreadCrossingFIFO::LocklessInPlaceThreadCrossingFIFO() : LocklessThreadCrossingFIFO() {

}

LocklessInPlaceThreadCrossingFIFO::LocklessInPlaceThreadCrossingFIFO(std::shared_ptr<SubSystem> parent) : LocklessThreadCrossingFIFO(parent) {

}

LocklessInPlaceThreadCrossingFIFO::LocklessInPlaceThreadCrossingFIFO(std::shared_ptr<SubSystem> parent,
                                                                     LocklessInPlaceThreadCrossingFIFO *orig) : LocklessThreadCrossingFIFO(parent, orig){

}

bool LocklessInPlaceThreadCrossingFIFO::isInPlace() {
    return true;
}

std::string
LocklessInPlaceThreadCrossingFIFO::emitCWriteToFIFO(std::vector<std::string> &cStatementQueue, std::string src, int numBlocks, Role role, bool pushStateAfter, bool forceNotInPlace) {
    if(forceNotInPlace){
        //Use the equivalent function in LocklessThreadCrossingFIFO (superclass)
        return LocklessThreadCrossingFIFO::emitCWriteToFIFO(cStatementQueue, src, numBlocks, role, pushStateAfter, forceNotInPlace);
    }

    if(pushStateAfter){
        throw std::runtime_error(ErrorHelpers::genErrorStr("When used in place, pushAfterState should be false", getSharedPointer()));
    }

    int arrayLengthBlocks = (fifoLength+1);

    std::string localWriteOffsetBlocks = getCWriteOffsetPtr().getCVarName(false)+"_local";
    std::string derefSharedWriteOffsetBlocks = "atomic_load_explicit(" + getCWriteOffsetPtr().getCVarName(false) + ", memory_order_acquire)";
    std::string arrayName = getCArrayPtr().getCVarName(false);

    //Declare write pointer variable here before scope opened below
    //Need the typename for the structure
    std::string fifoTypeName = getFIFOStructTypeName();
    std::string fifoBlockPtrName = src;
    cStatementQueue.push_back(fifoTypeName + " *" + fifoBlockPtrName + ";");

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

    cStatementQueue.push_back("//Get pointer to write position into shared array");
    //Get a pointer to the block and store in a variable so that the offsets can be updated without changing the pointer
    std::string blockPtrExpr = arrayName + "+" + localWriteOffsetBlocks;
    cStatementQueue.push_back(fifoBlockPtrName + " = " + blockPtrExpr + ";");

    cStatementQueue.push_back("//Increment write position");
    if(numBlocks == 1){
        cStatementQueue.push_back("if (" + localWriteOffsetBlocks + " >= " + GeneralHelper::to_string(arrayLengthBlocks - 1) + ") {"); //Elements and blocks are the same
        cStatementQueue.push_back(localWriteOffsetBlocks + " = 0;");
        cStatementQueue.push_back("} else {");
        cStatementQueue.push_back(localWriteOffsetBlocks + "++;");
        cStatementQueue.push_back("}");
        cStatementQueue.push_back(getCWriteOffsetCached().getCVarName(false) + " = " + localWriteOffsetBlocks + ";");

    }else{
        cStatementQueue.push_back("for (int32_t i = 0; i < " + GeneralHelper::to_string(numBlocks) + "; i++){");
        //Handle the mod with a branch as it should be cheaper
        cStatementQueue.push_back("if (" + localWriteOffsetBlocks + " >= " + GeneralHelper::to_string(arrayLengthBlocks - 1) + ") {");
        cStatementQueue.push_back(localWriteOffsetBlocks + " = 0;");
        cStatementQueue.push_back("} else {");
        cStatementQueue.push_back(localWriteOffsetBlocks + "++;");
        cStatementQueue.push_back("}");
        cStatementQueue.push_back("}");
        cStatementQueue.push_back("");
        cStatementQueue.push_back(getCWriteOffsetCached().getCVarName(false) + " = " + localWriteOffsetBlocks + ";");
    }

    cStatementQueue.push_back("}//End Scope for " + name + " FIFO Write");

    return fifoBlockPtrName;
}

std::string
LocklessInPlaceThreadCrossingFIFO::emitCReadFromFIFO(std::vector<std::string> &cStatementQueue, std::string dst, int numBlocks, Role role, bool pushStateAfter, bool forceNotInPlace) {
    if(forceNotInPlace){
        //Use the equivalent function in LocklessThreadCrossingFIFO (superclass)
        return LocklessThreadCrossingFIFO::emitCReadFromFIFO(cStatementQueue, dst, numBlocks, role, pushStateAfter, forceNotInPlace);
    }

    if(pushStateAfter){
        throw std::runtime_error(ErrorHelpers::genErrorStr("When used in place, pushAfterState should be false", getSharedPointer()));
    }

    int arrayLengthBlocks = (fifoLength+1);

    std::string localReadOffsetBlocks = getCReadOffsetPtr().getCVarName(false)+"_local";
    std::string derefSharedReadOffsetBlocks = "atomic_load_explicit(" + getCReadOffsetPtr().getCVarName(false) + ", memory_order_acquire)";
    std::string arrayName = getCArrayPtr().getCVarName(false);

    //Declare write pointer variable here before scope opened below
    //Need the typename for the structure
    std::string fifoTypeName = getFIFOStructTypeName();
    std::string fifoBlockPtrName = dst;
    cStatementQueue.push_back(fifoTypeName + " *" + fifoBlockPtrName + ";");

    //Open a block for the read to prevent scoping issues with declared temp
    cStatementQueue.push_back("{//Begin Scope for " + name + " FIFO Read");
    if(role == ThreadCrossingFIFO::Role::NONE) {
        cStatementQueue.push_back("//Load Read Ptr");
        cStatementQueue.push_back("int " + localReadOffsetBlocks + " = " + derefSharedReadOffsetBlocks + ";"); //Elements and blocks are the same
    }else{
        //Acquire occurred earlier in cache update
        cStatementQueue.push_back("int " + localReadOffsetBlocks + " = " + getCReadOffsetCached().getCVarName(false) + ";"); //Elements and blocks are the same
    }

    if(numBlocks == 1){
        //Read pointer is at the position of the last read value. Needs to be incremented before read
        cStatementQueue.push_back("if (" + localReadOffsetBlocks + " >= " + GeneralHelper::to_string(arrayLengthBlocks - 1) + ") {"); //Elements and blocks are the same
        cStatementQueue.push_back(localReadOffsetBlocks + " = 0;");
        cStatementQueue.push_back("} else {");
        cStatementQueue.push_back(localReadOffsetBlocks + "++;");
        cStatementQueue.push_back("}");
        cStatementQueue.push_back("");
        cStatementQueue.push_back("//Get pointer to read position into shared array");
        std::string blockPtrExpr = arrayName + "+" + localReadOffsetBlocks;
        cStatementQueue.push_back(fifoBlockPtrName + " = " + blockPtrExpr + ";");
        cStatementQueue.push_back(getCReadOffsetCached().getCVarName(false) + " = " + localReadOffsetBlocks + ";");
    }else{
        cStatementQueue.push_back("//Read from array");
        cStatementQueue.push_back("for (int32_t i = 0; i < " + GeneralHelper::to_string(numBlocks) + "; i++){");
        //Read pointer is at the position of the last read value. Needs to be incremented before read
        //Handle the mod with a branch as it should be cheaper
        cStatementQueue.push_back("if (" + localReadOffsetBlocks + " >= " + GeneralHelper::to_string(arrayLengthBlocks - 1) + ") {");
        cStatementQueue.push_back(localReadOffsetBlocks + " = 0;");
        cStatementQueue.push_back("} else {");
        cStatementQueue.push_back(localReadOffsetBlocks + "++;");
        cStatementQueue.push_back("}");
        //Only return the 1st increment
        cStatementQueue.push_back("if(i==0){");
        cStatementQueue.push_back("//Get pointer to read position into shared array");
        std::string blockPtrExpr = arrayName + "+" + localReadOffsetBlocks;
        cStatementQueue.push_back(fifoBlockPtrName + " = " + blockPtrExpr + ";");
        cStatementQueue.push_back("}");
        cStatementQueue.push_back("}");
        cStatementQueue.push_back("");
        cStatementQueue.push_back(getCReadOffsetCached().getCVarName(false) + " = " + localReadOffsetBlocks + ";");
    }

    cStatementQueue.push_back("}//End Scope for " + name + " FIFO Read");

    return fifoBlockPtrName;
}