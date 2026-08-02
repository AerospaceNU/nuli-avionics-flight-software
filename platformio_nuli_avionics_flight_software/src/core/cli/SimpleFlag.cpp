#include "SimpleFlag.h"

SimpleFlag::SimpleFlag(const char* name, const char* helpText, bool required, const std::function<void(DebugStream*)> &callback)
        : BaseFlag(name, helpText, required, callback) {}

const char* SimpleFlag::name() const {
    return m_name;
}

const char* SimpleFlag::help() const {
    return m_helpText;
}

CLIReturnCode_e SimpleFlag::parse(char* arg) {
    m_set = true;
    return CLI_SUCCESS;
}

void SimpleFlag::run(DebugStream* debugStream) {
    if (m_callback) {
        m_callback(debugStream);
    }
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

void SimpleFlag::getValueRaw(void* outValue) const  {
    *static_cast<bool*>(outValue) = m_set;
}
