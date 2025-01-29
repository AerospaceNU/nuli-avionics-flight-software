#ifndef DESKTOP_FLAG_TPP
#define DESKTOP_FLAG_TPP

#include "Flag.h"
#include <sstream>

/* /////////// */
/* / HELPERS / */
/* /////////// */

template <typename T>
struct TypeParser {
    static T parse(const char* value) {
        T result;
        std::istringstream iss(value);
        iss >> result;
        if (iss.fail()) {
            throw std::invalid_argument("Failed to parse argument");
        }
        return result;
    }
};

// Specialization for const char*
template <>
struct TypeParser<const char*> {
    static const char* parse(const char* value) {
        return value;  // Directly return the pointer
    }
};

/* ///////////// */
/* / BASE FLAG / */
/* ///////////// */

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

/* ///////////////// */
/* / ARGUMENT FLAG / */
/* ///////////////// */

template<typename T>
ArgumentFlag<T>::ArgumentFlag(const char* name, T defaultValue, const char* helpText, bool required)
        : m_name(name), m_defaultValue(defaultValue), m_helpText(helpText), m_set(false), m_defaultSet(true), m_required(required) {}

template<typename T>
ArgumentFlag<T>::ArgumentFlag(const char* name, const char* helpText, bool required)
        : m_name(name), m_helpText(helpText), m_set(false), m_defaultSet(false), m_required(required) {}

template<typename T>
const char* ArgumentFlag<T>::name() const {
    return m_name;
}

template<typename T>
const char* ArgumentFlag<T>::help() const {
    return m_helpText;
}

template<typename T>
void ArgumentFlag<T>::parse(int argc, char* argv[], int &argvPos) { //@TODO: Maybe change to return new argvPos?
    if (argvPos + 1 >= argc || argv[argvPos + 1][0] == '-') {
        if (m_defaultSet) {
            // use default argument
            m_argument = m_defaultValue;
        } else {
            throw std::invalid_argument("Default argument not set, value required");
        }

        m_set = true;
        return;
    }

    // set identifiers
    m_argument = TypeParser<T>::parse(argv[++argvPos]);;
    m_set = true;
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
void ArgumentFlag<T>::resetFlag() {
    m_set = false;
//    m_argument = NULL;  //@TODO: See if there's a better way to do this. This can be clunky if someone accidentally calls resetFlag and expects an argument
}
#endif // DESKTOP_FLAG_TPP
