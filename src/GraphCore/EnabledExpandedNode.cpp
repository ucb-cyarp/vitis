//
// Created by Christopher Yarp on 6/26/18.
//

#include "EnabledExpandedNode.h"

#include "GraphMLTools/GraphMLHelper.h"
#include "General/GeneralHelper.h"
#include "NodeFactory.h"

EnabledExpandedNode::EnabledExpandedNode() {

}

EnabledExpandedNode::EnabledExpandedNode(std::shared_ptr<SubSystem> parent) : EnabledSubSystem(parent), ExpandedNode(parent) {

}

EnabledExpandedNode::EnabledExpandedNode(std::shared_ptr<SubSystem> parent, std::shared_ptr<Node> orig, void* nop) : EnabledSubSystem(parent), ExpandedNode(parent, orig, nop) {

}

std::string EnabledExpandedNode::typeNameStr(){
    return "Enabled Expanded";
}

std::string EnabledExpandedNode::labelStr() {
    std::string label = Node::labelStr();

    label += "\nType: " + typeNameStr();

    return label;
}

xercesc::DOMElement *EnabledExpandedNode::emitGraphML(xercesc::DOMDocument *doc, xercesc::DOMElement *graphNode,
                                                      bool include_block_node_type) {
    //Get the parameters from the orig node.  Have the original emit everything but its block type
    xercesc::DOMElement* thisNode = origNode->emitGraphML(doc, graphNode, false);

    if(include_block_node_type) {
        GraphMLHelper::addDataNode(doc, thisNode, "block_node_type", "Expanded");
    }

    emitGramphMLSubgraphAndChildren(doc, thisNode);

    return thisNode;
}

EnabledExpandedNode::EnabledExpandedNode(std::shared_ptr<SubSystem> parent, EnabledExpandedNode* orig) : EnabledSubSystem(parent, orig), ExpandedNode(parent, orig){
    //Nothing extra to copy, call superclass constructors
}

std::shared_ptr<Node> EnabledExpandedNode::shallowClone(std::shared_ptr<SubSystem> parent) {
    return NodeFactory::shallowCloneNode<EnabledExpandedNode>(parent, this);
}

void EnabledExpandedNode::shallowCloneWithChildren(std::shared_ptr<SubSystem> parent,
                                                   std::vector<std::shared_ptr<Node>> &nodeCopies,
                                                   std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &origToCopyNode,
                                                   std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> &copyToOrigNode) {
    //Copy this node
    std::shared_ptr<EnabledExpandedNode> clonedNode = std::dynamic_pointer_cast<EnabledExpandedNode>(shallowClone(parent)); //This is a subsystem so we can cast to a subsystem pointer

    //Put into vectors and maps
    nodeCopies.push_back(clonedNode);
    origToCopyNode[shared_from_this()] = clonedNode;
    copyToOrigNode[clonedNode] = shared_from_this();

    //Copy the un-expanded node
    std::shared_ptr<Node> unexpandNode = origNode->shallowClone(nullptr); //This is a subsystem so we can cast to a subsystem pointer

    //Put into vectors and maps
    nodeCopies.push_back(unexpandNode);
    origToCopyNode[origNode] = unexpandNode;
    copyToOrigNode[unexpandNode] = origNode;

    //Add as unexpandedNode of clonedNode
    clonedNode->setOrigNode(unexpandNode);

    //Copy children
    for(auto it = children.begin(); it != children.end(); it++){
        std::shared_ptr<EnableInput> enabledInput = GeneralHelper::isType<Node, EnableInput>(*it);
        std::shared_ptr<EnableOutput> enabledOutput = GeneralHelper::isType<Node, EnableOutput>(*it);

        if(enabledInput != nullptr){
            std::shared_ptr<EnableInput> enableInputCopy = std::static_pointer_cast<EnableInput>(enabledInput->shallowClone(clonedNode));

            nodeCopies.push_back(enableInputCopy);
            origToCopyNode[enabledInput] = enableInputCopy;
            copyToOrigNode[enableInputCopy] = enabledInput;

            clonedNode->enabledInputs.push_back(enableInputCopy);
        }else if(enabledOutput != nullptr){
            std::shared_ptr<EnableOutput> enableOutputCopy = std::static_pointer_cast<EnableOutput>(enabledOutput->shallowClone(clonedNode));

            nodeCopies.push_back(enableOutputCopy);
            origToCopyNode[enabledOutput] = enableOutputCopy;
            copyToOrigNode[enableOutputCopy] = enabledOutput;

            clonedNode->enabledOutputs.push_back(enableOutputCopy);
        }else {
            //Recursive call to this function
            (*it)->shallowCloneWithChildren(clonedNode, nodeCopies, origToCopyNode, copyToOrigNode); //Use the copied node as the parent
        }
    }
}

