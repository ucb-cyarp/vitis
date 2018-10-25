//
// Created by Christopher Yarp on 10/22/18.
//

#include "Context.h"
#include "ContextRoot.h"

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
