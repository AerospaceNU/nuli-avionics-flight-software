//
// Created by chris on 1/6/2025.
//

#include <stdexcept>
#include <sstream>
#include "Flag.h"

SimpleFlag::SimpleFlag(const char* name, const char* helpText, bool required)
        : BaseFlag(name, helpText, required) {}

const char* SimpleFlag::name() const {
    return m_name;
}

const char* SimpleFlag::help() const {
    return m_helpText;
}

int8_t SimpleFlag::parse(int argc, char* argv[], int &argvPos) {
    m_set = true;
    return 0;
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

bool SimpleFlag::getValueDerived() const {
    return isSet();
}

BaseFlag::BaseFlag(const char* name, const char* helpText, bool required) :
        m_name(name), m_helpText(helpText), m_required(required), m_set(false) {}
