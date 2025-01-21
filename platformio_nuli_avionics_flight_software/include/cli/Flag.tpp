#ifndef DESKTOP_FLAG_TPP
#define DESKTOP_FLAG_TPP

#include "Flag.h"
#include <sstream>

template<typename T>
T BaseFlag::getValue() {
    if (SimpleFlag* simpleFlag = dynamic_cast<SimpleFlag*>(this)) {
        return simpleFlag->getValueDerived();
    } else if (ArgumentFlag<T>* argFlag = dynamic_cast<ArgumentFlag<T>*>(this)) {
        return argFlag->getValueDerived();
    } else {
        throw std::invalid_argument("Invalid Argument");
    }
}

template<typename T>
ArgumentFlag<T>::ArgumentFlag(const char* name, T defaultValue, const char* helpText, bool required)
        : m_name(name), m_argument(defaultValue), m_helpText(helpText), m_set(false), m_required(required) {}

template<typename T>
const char* ArgumentFlag<T>::name() const {
    return m_name;
}

template<typename T>
const char* ArgumentFlag<T>::help() const {
    return m_helpText;
}

template<typename T>
void ArgumentFlag<T>::parse(const char* value) {
    m_set = true;

    // convert to type T
    std::istringstream iss(value);
    T result;
    iss >> result;
    if (iss.fail()) {
//        char message[128];
//        snprintf(message, sizeof(message), "Conversion error for value: %s", value);
        throw std::runtime_error("Conversion error");
    }

    m_argument = result;
}
template<typename T>
bool ArgumentFlag<T>::isSet() const {
    return m_set;
}

template<typename T>
bool ArgumentFlag<T>::isRequired() const {
    return m_required;
}

#endif // DESKTOP_FLAG_TPP
