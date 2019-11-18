//
// Created by Christopher Yarp on 10/22/18.
//

#include "Context.h"
#include "ContextRoot.h"

Context::Context() : contextRoot(nullptr), subContext(0) {

};

Context::Context(std::shared_ptr<ContextRoot> contextRoot, int subContext) : contextRoot(contextRoot), subContext(subContext){

}

bool Context::operator==(const Context &rhs) const {
    return contextRoot == rhs.contextRoot &&
           subContext == rhs.subContext;
}

bool Context::operator!=(const Context &rhs) const {
    return !(rhs == *this);
}

std::shared_ptr<ContextRoot> Context::getContextRoot() const {
    return contextRoot;
}

void Context::setContextRoot(const std::shared_ptr<ContextRoot> &contextRoot) {
    Context::contextRoot = contextRoot;
}

int Context::getSubContext() const {
    return subContext;
}

void Context::setSubContext(int subContext) {
    Context::subContext = subContext;
}

bool Context::isEqOrSubContext(std::vector<Context> a, std::vector<Context> b){
    if(a.size() < b.size()){
        return false;
    }

    for(unsigned long i = 0; i<b.size(); i++){
        if(a[i] != b[i]){
            return false;
        }
    }

    return true;
}

bool Context::isEqContext(std::vector<Context> a, std::vector<Context> b){
    if(a.size() != b.size()){
        return false;
    }

    for(unsigned long i = 0; i<a.size(); i++){
        if(a[i] != b[i]){
            return false;
        }
    }

    return true;
}

bool Context::operator<(const Context &rhs) const {
    if (contextRoot < rhs.contextRoot)
        return true;
    if (rhs.contextRoot < contextRoot)
        return false;
    return subContext < rhs.subContext;
}

bool Context::operator>(const Context &rhs) const {
    return rhs < *this;
}

bool Context::operator<=(const Context &rhs) const {
    return !(rhs < *this);
}

bool Context::operator>=(const Context &rhs) const {
    return !(*this < rhs);
}

long Context::findMostSpecificCommonContext(std::vector<Context> &a, std::vector<Context> &b) {
    unsigned long maxInd = std::min(a.size(), b.size());

    long commonIndex;

    for(commonIndex = 0; commonIndex<maxInd; commonIndex++){
        if(a[commonIndex] != b[commonIndex]){
            break; //do not increment
        }
    }

    //Common index is off by 1 - will be 1 larger than what it should be.

    return commonIndex-1;
}
