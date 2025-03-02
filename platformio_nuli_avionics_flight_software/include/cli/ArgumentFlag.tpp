#ifndef DESKTOP_ARGUMENTFLAG_TPP
#define DESKTOP_ARGUMENTFLAG_TPP


#include "ArgumentFlag.h"

template<typename T>
ArgumentFlag<T>::ArgumentFlag(const char* name, T defaultValue, const char* helpText, bool required)
        : BaseFlag(name, helpText, required), m_defaultValue(defaultValue), m_defaultValueSet(true) {}

template<typename T>
ArgumentFlag<T>::ArgumentFlag(const char* name, const char* helpText, bool required)
        : BaseFlag(name, helpText, required) {}

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

#endif // DESKTOP_ARGUMENTFLAG_TPP