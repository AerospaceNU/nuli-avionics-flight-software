#ifndef DESKTOP_ARGUMENTFLAG_TPP
#define DESKTOP_ARGUMENTFLAG_TPP

#include "ArgumentFlag.h"
#include "Avionics.h"


template<typename T>
ArgumentFlag<T>::ArgumentFlag(const char* name, T defaultValue, const char* helpText, bool required, const std::function<void(DebugStream*)> &callback, bool highBandwidthOnly)
        : BaseFlag(name, helpText, required, callback, highBandwidthOnly), m_defaultValue(defaultValue), m_defaultValueSet(true) {}

template<typename T>
ArgumentFlag<T>::ArgumentFlag(const char* name, const char* helpText, bool required, const std::function<void(DebugStream*)> &callback, bool highBandwidthOnly)
        : BaseFlag(name, helpText, required, callback, highBandwidthOnly), m_defaultValueSet(false) {}

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
void ArgumentFlag<T>::run(DebugStream* debugStream) {
    if (m_callback) {
        m_callback(debugStream);
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
bool ArgumentFlag<T>::isHighBandwidthOnly() const {
    return m_highBandwidthOnly;
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

#endif // DESKTOP_ARGUMENTFLAG_TPP