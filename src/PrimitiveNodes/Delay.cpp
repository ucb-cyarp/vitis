//
// Created by Christopher Yarp on 7/6/18.
//

#include "Delay.h"

Delay::Delay() : delayValue(0){

}

Delay::Delay(std::shared_ptr<SubSystem> parent) : Node(parent), delayValue(0) {

}

int Delay::getDelayValue() const {
    return delayValue;
}

void Delay::setDelayValue(int delayValue) {
    Delay::delayValue = delayValue;
}
