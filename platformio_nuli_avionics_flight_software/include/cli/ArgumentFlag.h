#ifndef DESKTOP_ARGUMENTFLAG_H
#define DESKTOP_ARGUMENTFLAG_H

#include "BaseFlag.h"

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
    ArgumentFlag(const char* name, T defaultValue, const char* helpText, bool m_required, uint8_t uid, void (*callback)(uint8_t*, uint32_t length, uint8_t, uint8_t));

    /**
     * @brief Alternative constructor for an ArgumentFlag
     * @details Constructor for an ArgumentFlag without a provided
     * defaultValue. This may be useful in cases where there is no viable
     * or meaningful defaultValue. This can be important in situations where a
     * Flag's specific value is critical to the execution of a program.
     * @param name Name, or calling sign, of the flag
     * @param helpText A flag's help text
     * @param m_required If a flag is required
     * @param callback
     */
    ArgumentFlag(const char* name, const char* helpText, bool m_required, uint8_t uid, void (*callback)(uint8_t*, uint32_t length, uint8_t, uint8_t));

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
     * @details for ArgumentFlag, the argument is optional only if a
     * default argument was provided when constructing the flag.
     * @param arg argument to parse into flag, is nullable
     * @return 0 if success, negative for failure
     */
    int8_t parse(char* arg) override;

    /**
     * @brief Dispatches to a pre-set m_callback function.
     */
    void run(uint8_t groupUid) override;

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
     * @brief Retrieves the value of a derived flag
     * @tparam T type of the value
     * @return A flag's value
     */
    T getValueDerived() const;

protected:
    /**
     * @brief Retrieves the flag of a flag from a derived class
     * @param outValue Output
     */
    void getValueRaw(void* outValue) const override;

private:
    uint32_t myStrlen(const char* str);

    bool m_defaultValueSet = false; ///< if a default value has been set
    T m_defaultValue;   ///< default value when not provided by the user
    T m_argument;       ///< value of the flag
};

#include "ArgumentFlag.tpp"

#endif //DESKTOP_ARGUMENTFLAG_H
