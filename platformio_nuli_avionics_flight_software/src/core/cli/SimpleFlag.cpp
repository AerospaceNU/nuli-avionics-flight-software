#include "SimpleFlag.h"

SimpleFlag::SimpleFlag(const char* name, const char* helpText, bool required, uint8_t uid, void (*callback)(const char* name, uint8_t*, uint32_t length, uint8_t, uint8_t, BaseFlag* dependency))
        : BaseFlag(name, helpText, required, uid, callback) {}

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

void SimpleFlag::run(uint8_t groupUid) {
    if (m_callback) {
        // Create a local buffer to hold the boolean value
        uint8_t buffer[sizeof(bool)];

        // Copy the boolean value into the buffer
        buffer[0] = m_set ? 1 : 0;

        // Pass the buffer to the callback
        m_callback(m_name, buffer, sizeof(bool), groupUid, m_identifier, m_dependency);
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
