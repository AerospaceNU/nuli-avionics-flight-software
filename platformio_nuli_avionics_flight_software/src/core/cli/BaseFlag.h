#ifndef DESKTOP_BASEFLAG_H
#define DESKTOP_BASEFLAG_H

#include <string>
#include <cstdint>
#include "ReturnCodes.h"
#include <functional>

/**
 * @TODO: Change up parse implementation. Current implementation by passing in
 *          argc, argv, and argvPos are very hack-ey.
 *
 * @TODO: If a flag is not set, don't allow operations such as getValue. This
 *          is a little finicky without throwing an exception. I guess we can
 *          pass in the thing for where the returned value can go and then
 *          return an integer. OR, we can just have the user check if the
 *          flag's value has been set each time before calling. Just depends on
 *          what type of "overhead" we want.
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
    virtual CLIReturnCode_e parse(char* arg) = 0;

    /**
     *
     * @param flag
     */
    void setDependency(BaseFlag *flag);

    /**
     * @brief Dispatches to a pre-set m_callback function.
     */
    virtual void run(uint8_t groupUid) = 0;

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
     * @brief Retrieves the value of the flag.
     * @details When retrieving the value of a flag, a user must check if the
     * flag's value is set.
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
     * @param callback
     */
    // BaseFlag(const char* name, const char* helpText, bool required, uint8_t uid, void (*callback)(const char* name, uint8_t*, uint32_t length, uint8_t, uint8_t, BaseFlag*));
    BaseFlag(const char* name, const char* helpText, bool required, uint8_t uid, const std::function<void(void)> &callback);


    /**
     * @brief Parses an input into the expected type
     * @tparam T type of the flag's argument
     * @param value a const char* representing an input
     * @param result input translated into type T
     * @return 0 if success, <0 if failure
     */
    template<typename T>
    inline CLIReturnCode_e parseArgument(const char* value, T &result);

    /**
     * @brief Retrieves the flag of a flag from a derived class
     * @param outValue Output
     */
    virtual void getValueRaw(void* outValue) const = 0;

    const char* m_name;         ///< Name, or calling sign, of the flag
    const char* m_helpText;     ///< A flag's help text
    const bool m_required;      ///< If a flag is required
    const uint8_t m_identifier; ///< command identifier
    bool m_set;                 ///< If a flag is in-use
    // void (*m_callback)(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid, BaseFlag* dependency);   ///< Callback function. Takes in if a flag is set and its group's uid
    std::function<void(void)> m_callback;
    BaseFlag* m_dependency = nullptr;    ///<
};

#include "BaseFlag.tpp"

#endif //DESKTOP_BASEFLAG_H
