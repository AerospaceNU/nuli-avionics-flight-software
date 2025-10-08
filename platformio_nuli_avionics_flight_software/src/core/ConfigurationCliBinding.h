#ifndef CLICONFIGURATIONCOMMANDGENERATOR_H
#define CLICONFIGURATIONCOMMANDGENERATOR_H

#include <Avionics.h>
#include "core/cli/ArgumentFlag.h"
#include "core/cli/SimpleFlag.h"
#include "core/cli/Parser.h"
#include "core/generic_hardware/DebugStream.h"
#include "core/Configuration.h"

template <unsigned ConfigurationID>
class ConfigurationCliBinding {
public:
    ConfigurationCliBinding(): m_setValueFlag("-set", "", false, 255, []() {}), m_configurationFlag(command(), "", true, 255, [this]() { this->callback(); }) {}

    void setup(Configuration* configuration, Parser* parser, DebugStream *debugStream) {
        m_data = configuration->getConfigurable<ConfigurationID>();
        parser->addFlagGroup(m_configurationGeneratorGroup);
        m_debugStream = debugStream;
    }

    void callback() {
        if (m_setValueFlag.isSet()) {
            m_data.set(m_setValueFlag.getValueDerived());
            m_debugStream->setDecimalPlaces(8);
            m_debugStream->print(name());
            m_debugStream->print(" has been set to: ");
            m_debugStream->println(m_data.get());
        } else {
            m_debugStream->setDecimalPlaces(8);
            m_debugStream->print(name());
            m_debugStream->print(" is set to: ");
            m_debugStream->println(m_data.get());
        }
    }
private:
    static constexpr const char* name() {
        return GetConfigurationType_s<ConfigurationID>::name;
    }

    static constexpr const char* command() {
        return GetConfigurationType_s<ConfigurationID>::command;
    }

    using config_type_t = typename GetConfigurationType_s<ConfigurationID>::type;

    DebugStream *m_debugStream = nullptr;
    ConfigurationData<config_type_t> m_data;
    ArgumentFlag<config_type_t> m_setValueFlag;
    SimpleFlag m_configurationFlag;
    BaseFlag* m_configurationGeneratorGroup[2] = {&m_configurationFlag, &m_setValueFlag};
};

#endif //CLICONFIGURATIONCOMMANDGENERATOR_H
