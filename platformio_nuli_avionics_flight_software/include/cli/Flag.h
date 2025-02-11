//
// Created by chris on 1/6/2025.
//

#ifndef DESKTOP_FLAG_H
#define DESKTOP_FLAG_H

#include <string>
#include <cstdint>
#include <sstream>

/**
 * @TODO: Change up parse implementation. Current implementation by passing in
 *          argc, argv, and argvPos are very hack-ey.
 *
 * @TODO: If a flag is not set, don't allow operations such as getValueDerived
 */


/**
 * @class BaseFlag
 * @brief Abstract base class for command-line flags.
 * @details This class defines a common interface for different types of
 * command-line flags, such as simple boolean flags and argument-based flags.
 * Not intended for direct instantiation.
 * \derived
 * - <code>SimpleFlag</code>: Represents flags that are either set or unset (e.g., `-help`).\n
 * - <code>ArgumentFlag</code>: Represents flags that take an associated value (e.g., `--threshold 5.0`).
 */
class BaseFlag {
public:
    /**
     * @brief Retrieves the flag's name
     * @return the flag's name
     */
    virtual const char* name() const = 0;

    /**
     * @brief Retrieves the help text
     * @return the help text, a `const char*`
     */
    virtual const char* help() const = 0;

     /**
      * @brief Parses a single flag of the input.
      * @param arg argument to parse into flag, is nullable
      * @return 0 if success, negative for failure
      */
    virtual int8_t parse(char* arg) = 0;

    /**
     * @brief Tells the caller if this flag has been set.
     * @return true if set
     */
    virtual bool isSet() const = 0;

    /**
     * @brief Tells the caller if this flag is required.
     * @return true if required
     */
    virtual bool isRequired() const = 0;

    /**
     * @brief Resets all dynamic parameters of flag
     */
    virtual void reset() = 0;

    /**
     * @brief ensures a flags parameters are correctly set
     * @return true if successful
     */
    virtual bool verify() const = 0;

    /**
     * @brief Retrieves the value of the flag
     * @tparam T type of the value
     * @return A flag's value
     */
    template<typename T>
    T getValue();
protected:
    /**
     * @brief Constructor
     * @param name Name of a flag
     * @param helpText A flag's help text
     * @param required If a flag is required
     */
    BaseFlag(const char* name, const char* helpText, bool required);

    /**
     * @brief Parses an input into the expected type
     * @tparam T type of the flag's argument
     * @param value a const char* representing an input
     * @param result input translated into type T
     * @return 0 if success, <0 if failure
     */
    template<typename T>
    inline int8_t parseArgument(const char* value, T &result);

    const char* m_name;         ///< Name, or calling sign, of the flag
    const char* m_helpText;     ///< A flag's help text
    bool m_set;                 ///< If a flag is in-use
    bool m_required;            ///< If a flag is required
private:
};

/**
 * @class SimpleFlag
 * @brief Derived class of BaseFlag. Defines boolean flags.
 * @details This class is derived from BaseFlag and implements a basic boolean
 * flag. A boolean flag is either set or unset.
 */
class SimpleFlag : public BaseFlag {
public:
    /**
     * @brief Constructor for a SimpleFlag
     * @param name Name of the flag
     * @param helpText Name, or calling sign, of the flag
     * @param m_required If a flag is required
     */
    SimpleFlag(const char* name, const char* helpText, bool m_required);

    /**
     * @brief Retrieves the flag's name
     * @return the flag's name
     */
    const char* name() const override;

    /**
     * @brief Retrieves the help text
     * @return the help text, a `const char*`
     */
    const char* help() const override;

    /**
     * @brief Parses a single flag of the input.
     * @details for SimpleFlag, the argument is not needed
     * @param arg argument to parse into flag, is nullable
     * @return 0 if success, negative for failure
     */
    int8_t parse(char* arg) override;

    /**
     * @brief Tells the caller if this flag has been set.
     * @return true if set
     */
    bool isSet() const override;

    /**
     * @brief Tells the caller if this flag is required.
     * @return true if required
     */
    bool isRequired() const override;

    /**
     * @brief Resets all dynamic parameters of flag
     */
    void reset() override;

    /**
     * @brief ensures a flags parameters are correctly set
     * @return true if successful
     */
    bool verify() const override;

    /**
     * @brief Retrieves the value of the flag
     * @tparam T type of the value
     * @return A flag's value
     */
    bool getValueDerived() const;

protected:
private:
};


/**
 * @class ArgumentFlag
 * @brief Derived class of BaseFlag. Defines a flag that takes a fundamental
 * type as an argument.
 * @details This class is derived from BaseFlag and implements a flag taking
 * exactly one argument. Fundamental types are supported and explict
 * specializations have been made for `const char*`.
 * @tparam T A fundamental type, or `const char*`.
 */
template<typename T>
class ArgumentFlag : public BaseFlag {
public:

    /**
     * @brief Constructor for an ArgumentFlag
     * @details Constructor for an ArgumentFlag with a provided defaultValue.
     * This may be useful in cases a reasonable or commonly used default can be
     * assumed.
     * @param name Name, or calling sign, of the flag
     * @param defaultValue Value of flag when no value is provided
     * @param helpText A flag's help text
     * @param m_required If a flag is required
     */
    ArgumentFlag(const char* name, T defaultValue, const char* helpText, bool m_required);

    /**
     * @brief Alternative constructor for an ArgumentFlag
     * @details Constructor for an ArgumentFlag without a provided
     * defaultValue. This may be useful in cases where there is no viable
     * or meaningful defaultValue. This can be important in situations where a
     * Flag's specific value is critical to the execution of a program.
     * @param name Name, or calling sign, of the flag
     * @param helpText A flag's help text
     * @param m_required If a flag is required
     */
    ArgumentFlag(const char* name, const char* helpText, bool m_required);

    /**
     * @brief Retrieves the flag's name
     * @return the flag's name
     */
    const char* name() const override;

    /**
     * @brief Retrieves the help text
     * @return the help text, a `const char*`
     */
    const char* help() const override;

    /**
     * @brief Parses a single flag of the input.
     * @param argc Total number of arguments
     * @param argv Input array
     * @param argvPos Current parsing position in input array
     * @return 0 if success, negative for failure
     */

    /**
     * @brief Parses a single flag of the input.
     * @details for ArgumentFlag, the argument is optional only if a
     * default argument was provided when constructing the flag.
     * @param arg argument to parse into flag, is nullable
     * @return 0 if success, negative for failure
     */
    int8_t parse(char* arg) override;

    /**
     * @brief Tells the caller if this flag has been set.
     * @return true if set
     */
    bool isSet() const override;

    /**
     * @brief Tells the caller if this flag is required.
     * @return true if required
     */
    bool isRequired() const override;

    /**
     * @brief Resets all dynamic parameters of flag
     */
    void reset() override;

    /**
     * @brief ensures a flags parameters are correctly set
     * @return true if successful
     */
    bool verify() const override;

    /**
     * @brief Retrieves the value of the flag
     * @tparam T type of the value
     * @return A flag's value
     */
    T getValueDerived() const {
        return m_argument;
    }

protected:
private:
    bool m_defaultValueSet = false; ///< if a default value has been set
    T m_defaultValue;   ///< default value when not provided by the user
    T m_argument;       ///< value of the flag
};

#include "Flag.tpp"

#endif //DESKTOP_FLAG_H
