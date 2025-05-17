#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H

#include "Avionics.h"
#include "ConfigurationRegistry.h"
#include "HardwareAbstraction.h"
#include <Arduino.h>

struct ConfigurationBase {
    uint8_t* data = nullptr;
    uint16_t size = 0;
    ConfigurationID_e name = ConfigurationID_e::NONE;
    bool m_isUpdated = false;
};

template<typename T>
struct ConfigurationData : private ConfigurationBase {
    const T* get() {
        return ((T*) data);
    }

    void set(const T& newVal) {
        *((T*) data) = newVal;
        m_isUpdated = true;
    }
};


class Configuration {
public:
    constexpr static ConfigurationID_e REQUIRED_CONFIGS[] = {CONFIGURATION_VERSION, CONFIGURATION_CRC};

    template<unsigned N, unsigned M>
    explicit Configuration(const ConfigSet_s (&allConfigs)[N], uint8_t (&buffer)[M]): m_dataBuffer(buffer), m_dataBufferMaxLength(M) {
        construct(allConfigs, N);
    }

    template<unsigned N>
    explicit Configuration(const ConfigSet_s (&allConfigs)[N]): m_dataBuffer(m_buffer), m_dataBufferMaxLength(MAX_CONFIGURATION_LENGTH) {
        construct(allConfigs, N);
    }

    template<unsigned N>
    ConfigurationData<typename GetConfigType_s<N>::type>* getConfigurable();

    void pushUpdates();

private:
    void construct(const ConfigSet_s* allConfigs, uint16_t allConfigsLength);

    void readConfigFromMemory();

    bool configExists(ConfigurationID_e name) const;

    void sortConfigs();

    void assignMemory();

    static void outOfMemoryError();

    uint8_t* m_dataBuffer;
    const uint32_t m_dataBufferMaxLength = 0;

    uint8_t m_buffer[MAX_CONFIGURATION_LENGTH] = {};
    ConfigurationBase m_configurations[MAX_CONFIGURATION_NUM] = {};
    uint32_t m_numConfigurations = 0;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H
