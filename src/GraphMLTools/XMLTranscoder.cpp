//
// Created by Christopher Yarp on 7/12/18.
//

#include "XMLTranscoder.h"

XMLTranscoder::XMLTranscoder(std::string str) {
    ptr = xercesc::XMLString::transcode(str.c_str());
}

XMLTranscoder::~XMLTranscoder() {
    xercesc::XMLString::release(&ptr);
}

XMLCh *XMLTranscoder::xmlch() {
    return ptr;
}
