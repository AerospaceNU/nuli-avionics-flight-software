#ifndef DESKTOP_ARGUMENTFLAG_TPP
#define DESKTOP_ARGUMENTFLAG_TPP


#include "ArgumentFlag.h"


template<typename T>
ArgumentFlag<T>::ArgumentFlag(const char* name, T defaultValue, const char* helpText, bool required, uint8_t uid, void (*callback)(const char* name, uint8_t*, uint32_t length, uint8_t, uint8_t, BaseFlag* dependency))
        : BaseFlag(name, helpText, required, uid, callback), m_defaultValue(defaultValue), m_defaultValueSet(true) {}

template<typename T>
ArgumentFlag<T>::ArgumentFlag(const char* name, const char* helpText, bool required, uint8_t uid, void (*callback)(const char* name, uint8_t*, uint32_t length, uint8_t, uint8_t, BaseFlag* dependency))
        : BaseFlag(name, helpText, required, uid, callback), m_defaultValueSet(false) {}

template<typename T>
const char* ArgumentFlag<T>::name() const {
    return m_name;
}

template<typename T>
const char* ArgumentFlag<T>::help() const {
    return m_helpText;
}

template<typename T>
CLIReturnCode_e ArgumentFlag<T>::parse(char* arg) { //@TODO: Maybe change to return new argvPos?
    // early exit
    if (arg == nullptr) {
        if (m_defaultValueSet) {
            // use default argument
            m_argument = m_defaultValue;
            m_set = true;
            return CLI_SUCCESS;   // success
        } else {
            fprintf(stderr, "Default argument not set, value required for %s\n", this->name());
            return CLI_NO_DEFAULT_VALUE_SET;
        }
    }

    // set identifiers
    CLIReturnCode_e returnCode = this->parseArgument(arg, m_argument);
    if (returnCode != CLI_SUCCESS) {
        return returnCode;
    }

    m_set = true;
    return CLI_SUCCESS;
}

template<typename T>
void ArgumentFlag<T>::run(uint8_t groupUid) {
    if (m_callback) {
        // Create a local buffer to hold the value
        uint8_t buffer[sizeof(T)];

        // Copy the value into the buffer
        memcpy(buffer, &m_argument, sizeof(T));

        // Pass the buffer to the callback
        m_callback(m_name, buffer, sizeof(T), groupUid, m_identifier, m_dependency);
    }
}

// Specialization for float
template<>
void ArgumentFlag<float>::run(uint8_t groupUid) {
    if (m_callback) {
        uint8_t buffer[sizeof(float)];

        // Copy float data into byte buffer
        memcpy(buffer, &m_argument, sizeof(float));

        // Call the callback with the serialized data
        m_callback(m_name, buffer, sizeof(float), groupUid, m_identifier, m_dependency);
    }
}

// Specialization for const char*
template<>
void ArgumentFlag<const char*>::run(uint8_t groupUid) {
    if (m_callback) {
        if (m_argument) {
            size_t length = myStrlen(m_argument);

            // Call the callback with the string data
            // Note: we're passing the raw pointer but with the correct length
            m_callback(m_name, (uint8_t*)m_argument, length, groupUid, m_identifier, m_dependency);
        } else {
            // Handle null strings
            m_callback(m_name, nullptr, 0, groupUid, m_identifier, m_dependency);
        }
    }
}

template<typename T>
bool ArgumentFlag<T>::isSet() const {
    return m_set;
}

template<typename T>
bool ArgumentFlag<T>::isRequired() const {
    return m_required;
}

template<typename T>
void ArgumentFlag<T>::reset() {
    m_set = false;
    m_argument = m_defaultValue;
}

template<typename T>
bool ArgumentFlag<T>::verify() const {
    return !(this->isRequired() && !this->isSet());
}

template<typename T>
T ArgumentFlag<T>::getValueDerived() const {
    if (this->isSet()) {
        return m_argument;
    } else {
        return m_defaultValue;
    }
}

template<typename T>
void ArgumentFlag<T>::getValueRaw(void* outValue) const  {
    *static_cast<T*>(outValue) = m_argument;  // Cast and assign
}

template<typename T>
uint32_t ArgumentFlag<T>::myStrlen(const char* str) {
    if (str == nullptr) return 0;

    uint32_t length = 0;
    while (str[length] != '\0') {
        length++;
    }
    return length;
}

#endif // DESKTOP_ARGUMENTFLAG_TPP