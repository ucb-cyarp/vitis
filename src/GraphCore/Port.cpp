//
// Created by Christopher Yarp on 6/25/18.
//

#include "Port.h"

Port::Port() : portNum(0), type(Port::PortType::INPUT) {
    parent = std::shared_ptr<Node>(nullptr);
}

Port::Port(std::shared_ptr<Node> parent, Port::PortType type, int portNum) : portNum(portNum), type(type), parent(parent) {

}
