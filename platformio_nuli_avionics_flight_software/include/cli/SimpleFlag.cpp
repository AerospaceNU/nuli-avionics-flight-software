//
// Created by chris on 2/24/2025.
//

#include "SimpleFlag.h"

SimpleFlag::SimpleFlag(const char* name, const char* helpText, bool required, void (*callback)(bool, int8_t))
        : BaseFlag(name, helpText, required), m_callback(callback) {}

const char* SimpleFlag::name() const {
    return m_name;
}

const char* SimpleFlag::help() const {
    return m_helpText;
}

int8_t SimpleFlag::parse(char* arg) {
    m_set = true;
    return 0;
}

void SimpleFlag::run(int8_t uid) {
    if (m_callback) m_callback(m_set, uid);
}


bool SimpleFlag::isSet() const {
    return m_set;
}

bool SimpleFlag::isRequired() const {
    return m_required;
}

void SimpleFlag::reset() {
    m_set = false;
}

bool SimpleFlag::verify() const {
    /*
     isRequired isSet               not
     yes        no     --> yes  --> no
     yes        yes    --> no   --> yes
     no         no     --> no   --> yes
     no         yes    --> no   --> yes
    */
    return !(this->isRequired() && !this->isSet());
}

void SimpleFlag::setStreams(FILE* inputStream, FILE* outputStream, FILE* errorStream) {
    m_inputStream = inputStream;
    m_outputStream = outputStream;
    m_errorStream = errorStream;
}


bool SimpleFlag::getValueDerived() const {
    return isSet();
}

void SimpleFlag::getValueRaw(void* outValue) const  {
    *static_cast<bool*>(outValue) = m_set;
}
