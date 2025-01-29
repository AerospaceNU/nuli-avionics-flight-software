//
// Created by chris on 1/6/2025.
//

#include <stdexcept>
#include <sstream>
#include "Flag.h"

bool BaseFlag::verify() const { //@TODO: Check if this is bad design
    /*
     isRequired isSet               not
     yes        no     --> yes  --> no
     yes        yes    --> no   --> yes
     no         no     --> no   --> yes
     no         yes    --> no   --> yes
    */
    return !(this->isRequired() && !this->isSet());
}

SimpleFlag::SimpleFlag(const char* name, const char* helpText, bool required)
        : m_name(name), m_helpText(helpText), m_set(false), m_required(required) {}

const char* SimpleFlag::name() const {
    return m_name;
}

const char* SimpleFlag::help() const {
    return m_helpText;
}

void SimpleFlag::parse(int argc, char* argv[], int &argvPos) {
    m_set = true;
}

bool SimpleFlag::isSet() const {
    return m_set;
}

bool SimpleFlag::isRequired() const {
    return m_required;
}

void SimpleFlag::resetFlag() {
    m_set = false;
}
