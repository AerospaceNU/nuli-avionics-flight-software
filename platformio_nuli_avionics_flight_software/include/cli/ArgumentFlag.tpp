#ifndef DESKTOP_ARGUMENTFLAG_TPP
#define DESKTOP_ARGUMENTFLAG_TPP


#include "ArgumentFlag.h"

template<typename T>
ArgumentFlag<T>::ArgumentFlag(const char* name, T defaultValue, const char* helpText, bool required, void (*callback)(T, int8_t))
        : BaseFlag(name, helpText, required), m_defaultValue(defaultValue), m_defaultValueSet(true), m_callback(callback) {}

template<typename T>
ArgumentFlag<T>::ArgumentFlag(const char* name, const char* helpText, bool required, void (*callback)(T, int8_t))
        : BaseFlag(name, helpText, required), m_defaultValueSet(false), m_callback(callback) {}

template<typename T>
const char* ArgumentFlag<T>::name() const {
    return m_name;
}

template<typename T>
const char* ArgumentFlag<T>::help() const {
    return m_helpText;
}

template<typename T>
int8_t ArgumentFlag<T>::parse(char* arg) { //@TODO: Maybe change to return new argvPos?
    // early exit
    if (arg == nullptr) {
        if (m_defaultValueSet) {
            // use default argument
            m_argument = m_defaultValue;
            m_set = true;
            return 0;   // success
        } else {
            fprintf(m_errorStream, "Default argument not set, value required for %s\n", this->name());
            return -1;
        }
    }

    // set identifiers
    if (this->parseArgument(arg, m_argument) < 0) {
        return -1;
    }

    m_set = true;
    return 0;
}

template<typename T>
void ArgumentFlag<T>::run(int8_t uid) {
    if (m_callback) m_callback(m_argument, uid);
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
void ArgumentFlag<T>::setStreams(FILE* inputStream, FILE* outputStream, FILE* errorStream) {
    m_inputStream = inputStream;
    m_outputStream = outputStream;
    m_errorStream = errorStream;
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

#endif // DESKTOP_ARGUMENTFLAG_TPP