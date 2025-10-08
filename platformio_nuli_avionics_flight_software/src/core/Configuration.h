#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H

#include "Avionics.h"
#include "ConfigurationRegistryWraper.h"
#include "HardwareAbstraction.h"
#include "generic_hardware/FramMemory.h"
#include "generic_hardware/DebugStream.h"
#include <Arduino.h>


struct BaseConfigurationData_s {
    uint8_t* data = nullptr;
    uint16_t size = 0;
    ConfigurationID_t name = NONE_c;
    bool m_isUpdated = false;
};

template <typename T>
struct ConfigurationData {
    BaseConfigurationData_s* base = nullptr; // pointer to the underlying data

    ConfigurationData() = default;

    // Constructor from BaseConfigurationData_s*
    explicit ConfigurationData(BaseConfigurationData_s* b) : base(b) {}

    T get() const {
        if (!base || !base->data) return T{}; // default-constructed T
        return *reinterpret_cast<const T*>(base->data);
    }

    void set(const T& newVal) {
        if (!base || !base->data) return;
        *reinterpret_cast<T*>(base->data) = newVal;
        base->m_isUpdated = true;
    }

    bool isValid() const { return base != nullptr; }
};

class Configuration {
public:
    constexpr static ConfigurationID_t REQUIRED_CONFIGS[] = {CONFIGURATION_VERSION_c, CONFIGURATION_VERSION_HASH_c, CONFIGURATION_CRC_c};

    template <unsigned N, unsigned M>
    explicit Configuration(const ConfigurationIDSet_s (&allConfigs)[N], uint8_t (&buffer)[M]): m_dataBuffer(buffer), m_dataBufferMaxLength(M) {
        construct(allConfigs, N);
    }

    template <unsigned N>
    explicit Configuration(const ConfigurationIDSet_s (&allConfigs)[N]): m_dataBuffer(m_buffer), m_dataBufferMaxLength(MAX_CONFIGURATION_LENGTH) {
        construct(allConfigs, N);
    }

    void setup(HardwareAbstraction* hardware, uint8_t id);

    template <unsigned N>
    ConfigurationData<typename GetConfigurationType_s<N>::type> getConfigurable() {
        using T = typename GetConfigurationType_s<N>::type;
        const ConfigurationID_t id = ConfigurationID_t(N);
        BaseConfigurationData_s* base = configExists(id) ? getBaseConfigurationData(id) : nullptr;
        return ConfigurationData<T>(base);
    }

    template <unsigned N>
    void setDefault(const typename GetConfigurationType_s<N>::type &value) {
        // Ensure setup has not been called
        if (m_hardware == nullptr) {
            getConfigurable<N>().set(value);
        } else {
            m_debug->println("It's too late to be setting a default");
        }
    }

    void pushUpdatesToMemory();

private:
    BaseConfigurationData_s* getBaseConfigurationData(const ConfigurationID_t name) {
        int32_t left = 0;
        int32_t right = (int32_t)m_numConfigurations - 1;

        while (left <= right) {
            const int32_t mid = left + (right - left) / 2;
            const int32_t midName = m_configurations[mid].name;

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

    void readConfigFromMemory() const;

    bool configExists(ConfigurationID_t name) const;

    void sortConfigs();

    void assignMemory();

    void outOfMemoryError() const;

    uint8_t* m_dataBuffer;
    uint32_t m_dataBufferIndex = 0;
    const uint32_t m_dataBufferMaxLength = 0;

    uint8_t m_buffer[MAX_CONFIGURATION_LENGTH] = {};
    BaseConfigurationData_s m_configurations[MAX_CONFIGURATION_NUM] = {};
    uint32_t m_numConfigurations = 0;

    HardwareAbstraction* m_hardware = nullptr;
    FramMemory* m_memory = nullptr;
    DebugStream* m_debug = nullptr;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H
