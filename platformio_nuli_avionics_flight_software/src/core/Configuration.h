#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H

#include "Avionics.h"
#include "ConfigurationRegistry.h"
#include "generic_hardware/ConfigurationMemory.h"
#include "generic_hardware/DebugStream.h"
#include <Arduino.h>

struct BaseConfigurationData_s {
    uint8_t* data = nullptr;
    uint16_t size = 0;
    ConfigurationID_e name = ConfigurationID_e::NONE;
    bool m_isUpdated = false;
};

template<typename T>
struct ConfigurationData : private BaseConfigurationData_s {
    const T &get() {
        return *((T*) data);
    }

    void set(const T &newVal) {
        *((T*) data) = newVal;
        m_isUpdated = true;
    }
};


class Configuration {
public:
    constexpr static ConfigurationID_e REQUIRED_CONFIGS[] = {CONFIGURATION_VERSION, CONFIGURATION_VERSION_HASH, CONFIGURATION_CRC};

    template<unsigned N, unsigned M>
    explicit Configuration(const ConfigurationIDSet_s (&allConfigs)[N], uint8_t (&buffer)[M]): m_dataBuffer(buffer), m_dataBufferMaxLength(M) {
        construct(allConfigs, N);
    }

    template<unsigned N>
    explicit Configuration(const ConfigurationIDSet_s (&allConfigs)[N]): m_dataBuffer(m_buffer), m_dataBufferMaxLength(MAX_CONFIGURATION_LENGTH) {
        construct(allConfigs, N);
    }

    void setup(ConfigurationMemory* memory, DebugStream* debugStream);

    template<unsigned N>
    ConfigurationData<typename GetConfigType_s<N>::type>* getConfigurable() {
        return (ConfigurationData<typename GetConfigType_s<N>::type>*) getBaseConfigurationData(ConfigurationID_e(N));
    }

//    template<unsigned N>
//    void setDefaultValue(const typename GetConfigType_s<N>::type &value) {
//        auto configurationData = getConfigurable<N>();
//        if(!configurationData->isInitialized()) {
//            configurationData->set(value);
//        }
//    }

    void pushUpdates();

private:
    BaseConfigurationData_s* getBaseConfigurationData(ConfigurationID_e name) {
        int32_t left = 0;
        int32_t right = (int32_t) m_numConfigurations - 1;

        while (left <= right) {
            int32_t mid = left + (right - left) / 2;
            int32_t midName = m_configurations[mid].name;

            if (midName == name) {
                return &m_configurations[mid];
            } else if (midName < name) {
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }
        return nullptr;
    }

    void construct(const ConfigurationIDSet_s* allConfigs, uint16_t allConfigsLength);

    void readConfigFromMemory();

    bool configExists(ConfigurationID_e name) const;

    void sortConfigs();

    void assignMemory();

    static void outOfMemoryError();

    uint8_t* m_dataBuffer;
    uint32_t m_dataBufferIndex = 0;
    const uint32_t m_dataBufferMaxLength = 0;

    uint8_t m_buffer[MAX_CONFIGURATION_LENGTH] = {};
    BaseConfigurationData_s m_configurations[MAX_CONFIGURATION_NUM] = {};
    uint32_t m_numConfigurations = 0;

    ConfigurationMemory* m_memory = nullptr;
    DebugStream* m_debug = nullptr;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H
