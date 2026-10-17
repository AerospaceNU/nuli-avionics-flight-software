#ifndef DESKTOP_SIMPLEFLAG_H
#define DESKTOP_SIMPLEFLAG_H


#include "BaseFlag.h"

/**
 * @brief Boolean flag: set when present, takes no argument.
 */
class SimpleFlag : public BaseFlag {
public:
    SimpleFlag(const char* name, const char* helpText, bool m_required, const std::function<void(DebugStream*)> &callback, bool highBandwidthOnly = false);


    const char* name() const override;

    const char* help() const override;

    CLIReturnCode_e parse(char* arg) override;

    void run(DebugStream* debugStream) override;

    bool isSet() const override;

    bool isRequired() const override;

    bool isHighBandwidthOnly() const override;

    void reset() override;

    bool verify() const override;

    bool getValueDerived() const;

protected:
    void getValueRaw(void* outValue) const override;
};


#endif //DESKTOP_SIMPLEFLAG_H
