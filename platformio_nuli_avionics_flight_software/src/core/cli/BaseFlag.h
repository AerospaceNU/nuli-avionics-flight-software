#ifndef DESKTOP_BASEFLAG_H
#define DESKTOP_BASEFLAG_H

#include <cstdint>
#include "ReturnCodes.h"
#include <functional>

class DebugStream;


/**
 * @brief Abstract base for command-line flags (SimpleFlag, ArgumentFlag).
 */
class BaseFlag {
public:
    virtual const char* name() const = 0;

    virtual const char* help() const = 0;

     /**
      * @brief Parses a single flag of the input; arg is nullable.
      */
    virtual CLIReturnCode_e parse(char* arg) = 0;

    void setDependency(BaseFlag *flag);

    virtual void run(DebugStream* debugStream) = 0;

    virtual bool isSet() const = 0;

    virtual bool isRequired() const = 0;

    /**
     * @brief True if run() is skipped (not an error) on a stream whose isHighBandwidth() is false.
     */
    virtual bool isHighBandwidthOnly() const = 0;

    virtual void reset() = 0;

    /**
     * @brief Returns false if the flag is required but not set.
     */
    virtual bool verify() const = 0;

    /**
     * @brief Returns the flag's value; the caller must check isSet() first.
     */
    template<typename T>
    T getValue();

protected:
    BaseFlag(const char* name, const char* helpText, bool required, const std::function<void(DebugStream*)> &callback, bool highBandwidthOnly = false);


    template<typename T>
    inline CLIReturnCode_e parseArgument(const char* value, T &result);

    virtual void getValueRaw(void* outValue) const = 0;

    const char* m_name;
    const char* m_helpText;
    const bool m_required;
    const bool m_highBandwidthOnly;
    bool m_set;
    std::function<void(DebugStream*)> m_callback;
    BaseFlag* m_dependency = nullptr;
};

#include "BaseFlag.tpp"

#endif //DESKTOP_BASEFLAG_H
