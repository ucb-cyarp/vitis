//
// Created by Christopher Yarp on 7/12/18.
//

#ifndef VITIS_XMLTRANSCODER_H
#define VITIS_XMLTRANSCODER_H

#include <string>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/PlatformUtils.hpp>

/**
 * \addtogroup GraphMLTools GraphML Import/Export Tools
 */
/*@{*/

/**
 * @brief This class is used to transcode text from a std::string to an XML string
 *
 * This exists as a class so that the memory allocated by the xerces transcoder can be automatically cleaned up durring object destruction.
 *
 * This is based off of XStr from CreateDOMDocument.cpp
 *
 */
class XMLTranscoder{
private:
    XMLCh* ptr;
public:
    /**
     * @brief Transcode a string to XMLCh
     *
     * get the transcoded string with @ref xmlch
     *
     * @param str string to transcode
     */
    XMLTranscoder(std::string str);

    /**
     * @brief Cleanup the memory allocated durring the transcode
     */
    ~XMLTranscoder();

    /**
     * @brief Get the transcoded XMLCh pointer
     * @return
     */
    XMLCh* xmlch();
};

//
/**
 * @brief Transcode text for XML and cleanup allocated memory
 *
 * Using the technique in CreateDOMDocument.cpp to define a function like macro to get the transcribed string and automatically cleanup the memory allocated by xerces
 */
#define TranscodeToXMLCh(str) XMLTranscoder(str).xmlch()

/*@}*/

#endif //VITIS_XMLTRANSCODER_H
