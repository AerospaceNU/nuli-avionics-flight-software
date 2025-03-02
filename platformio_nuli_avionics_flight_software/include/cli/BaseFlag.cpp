//
// Created by chris on 1/6/2025.
//

#include "BaseFlag.h"

BaseFlag::BaseFlag(const char* name, const char* helpText, bool required) :
        m_name(name), m_helpText(helpText), m_required(required), m_set(false) {}

BaseFlag::BaseFlag(const char* name, const char* helpText, bool required, FILE* inputStream, FILE* outputStream, FILE* errorStream) :
        m_name(name), m_helpText(helpText), m_required(required), m_set(false),
        m_inputStream(inputStream), m_outputStream(outputStream), m_errorStream(errorStream)  {}

void BaseFlag::setStreams(FILE* inputStream, FILE* outputStream, FILE* errorStream) {
    m_inputStream = inputStream;
    m_outputStream = outputStream;
    m_errorStream = errorStream;
}
