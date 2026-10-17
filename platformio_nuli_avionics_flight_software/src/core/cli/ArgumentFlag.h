#ifndef DESKTOP_ARGUMENTFLAG_H
#define DESKTOP_ARGUMENTFLAG_H

#include "BaseFlag.h"

/**
 * @brief Flag taking exactly one argument of a fundamental type or `const char*`.
 */
template<typename T>
class ArgumentFlag : public BaseFlag {
public:

    /**
     * @brief Constructs a flag whose argument is optional; defaultValue is used when none is given.
     */
    ArgumentFlag(const char* name, T defaultValue, const char* helpText, bool m_required, const std::function<void(DebugStream*)> &callback, bool highBandwidthOnly = false);

    /**
     * @brief Constructs a flag with no default; an argument must be supplied.
     */
    ArgumentFlag(const char* name, const char* helpText, bool m_required, const std::function<void(DebugStream*)> &callback, bool highBandwidthOnly = false);

    const char* name() const override;

    const char* help() const override;

    /**
     * @brief Parses the flag's argument; a null arg is accepted only if a default was provided.
     */
    CLIReturnCode_e parse(char* arg) override;

    void run(DebugStream* debugStream) override;

    bool isSet() const override;

    bool isRequired() const override;

    bool isHighBandwidthOnly() const override;

    void reset() override;

    bool verify() const override;

    T getValueDerived() const;

protected:
    void getValueRaw(void* outValue) const override;

private:
    bool m_defaultValueSet = false;
    T m_defaultValue{};
    T m_argument{};
};

#include "ArgumentFlag.tpp"

#endif //DESKTOP_ARGUMENTFLAG_H
