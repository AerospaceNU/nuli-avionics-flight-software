#ifndef DESKTOP_FLAG_TPP
#define DESKTOP_FLAG_TPP

#include "Flag.h"
#include <sstream>

/* ///////////// */
/* / BASE FLAG / */
/* ///////////// */

// general
template<typename T>
int8_t BaseFlag::parseArgument(const char* value, T &result) {
    std::istringstream iss(value);
    iss >> result;
    if (iss.fail()) {
        fprintf(stderr, "Failed to parse argument. Likely incorrect type.\n");
        return -1;
    }

    return 0;
}

// specialization
template<>
inline int8_t BaseFlag::parseArgument<const char*>(const char* value, const char*&result) {
    result = value;
    return 0;
}


//@TODO: Need to make a specific version that works with const char*
//@TODO: Or just deprecate this thing
template<typename T>
T BaseFlag::getValue() {
    if (SimpleFlag* simpleFlag = dynamic_cast<SimpleFlag*>(this)) {
        return simpleFlag->getValueDerived();
    } else if (ArgumentFlag<T>* argFlag = dynamic_cast<ArgumentFlag<T>*>(this)) {
        return argFlag->getValueDerived();
    } else {
//        return nullptr;
        throw std::invalid_argument("Invalid Argument");
    }
}

//template<>
//const char* BaseFlag::getValue() {
//    if (ArgumentFlag<const char*>* argFlag = dynamic_cast<ArgumentFlag<const char*>*>(this)) {
//        return argFlag->getValueDerived();
//    } else {
//        throw std::invalid_argument("Invalid Argument");
//    }
    //    if (SimpleFlag* simpleFlag = dynamic_cast<SimpleFlag*>(this)) {
//        return simpleFlag->getValueDerived();
//    } else if (ArgumentFlag<T>* argFlag = dynamic_cast<ArgumentFlag<T>*>(this)) {
//        return argFlag->getValueDerived();
//    } else {
//        throw std::invalid_argument("Invalid Argument");
//    }
//}


/* ///////////////// */
/* / ARGUMENT FLAG / */
/* ///////////////// */

template<typename T>
ArgumentFlag<T>::ArgumentFlag(const char* name, T defaultValue, const char* helpText, bool required)
        : BaseFlag(name, helpText, required), m_defaultValue(defaultValue) {}

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
int8_t ArgumentFlag<T>::parse(int argc, char* argv[], int &argvPos) { //@TODO: Maybe change to return new argvPos?
    // early exit
    if (argvPos + 1 >= argc || argv[argvPos + 1][0] == '-') { //@TODO: Maybe change to passing in the next argument (which can be nullptr)
        if (m_defaultValue) {
            // use default argument
            m_argument = m_defaultValue;
        } else {
            fprintf(stderr, "Default argument not set, value required\n");
            return -1;
        }

        m_set = true;
        return 0;   // success
    }

    // set identifiers
    if (this->parseArgument(argv[++argvPos], m_argument) < 0) {
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
//    m_argument = NULL;  //@TODO: See if there's a better way to do this. This can be clunky if someone accidentally calls reset and expects an argument
}

template<typename T>
bool ArgumentFlag<T>::verify() const {
    return !(this->isRequired() && !this->isSet());
}


#endif // DESKTOP_FLAG_TPP
