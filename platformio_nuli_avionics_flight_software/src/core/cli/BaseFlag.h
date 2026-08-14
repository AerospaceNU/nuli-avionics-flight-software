#ifndef DESKTOP_BASEFLAG_H
#define DESKTOP_BASEFLAG_H

#include <cstdint>
#include "ReturnCodes.h"
#include <functional>

class DebugStream;

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
    virtual void run(DebugStream* debugStream) = 0;

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
     * @brief Tells the caller if this flag may only run on a high-bandwidth DebugStream
     * @details Checked by Parser::FlagGroup_s::runFlags() before invoking run() - a flag
     * marked true is silently skipped (not an error) on a stream whose isHighBandwidth()
     * is false, e.g. a CLI channel relayed over a slow radio link.
     * @return true if restricted to high-bandwidth streams
     */
    virtual bool isHighBandwidthOnly() const = 0;

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
     * @param highBandwidthOnly If true, run() is skipped on a stream whose isHighBandwidth() is
     * false (see DebugStream) - default false, i.e. allowed over every communication method
     */
    BaseFlag(const char* name, const char* helpText, bool required, const std::function<void(DebugStream*)> &callback, bool highBandwidthOnly = false);


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
    const bool m_highBandwidthOnly; ///< If true, skipped on a low-bandwidth DebugStream
    bool m_set;                 ///< If a flag is in-use
    std::function<void(DebugStream*)> m_callback;
    BaseFlag* m_dependency = nullptr;    ///<
};

#include "BaseFlag.tpp"

#endif //DESKTOP_BASEFLAG_H
