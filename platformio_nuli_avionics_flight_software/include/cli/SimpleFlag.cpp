//
// Created by chris on 2/24/2025.
//

#include "SimpleFlag.h"

SimpleFlag::SimpleFlag(const char* name, const char* helpText, bool required)
        : BaseFlag(name, helpText, required) {}

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