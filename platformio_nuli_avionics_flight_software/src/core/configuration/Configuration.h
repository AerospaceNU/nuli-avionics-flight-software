#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H

#include "Avionics.h"
#include "ConfigurationRegistryWraper.h"
#include "core/HardwareAbstraction.h"
#include "core/generic_hardware/GenericHardware.h"
#include <cstddef>  // std::max_align_t
#include <cstdint>  // uintptr_t

struct BaseConfigurationData_s {
    uint8_t* data = nullptr;
    uint16_t size = 0;
    ConfigurationID_t id = NONE_c;
    bool m_isUpdated = false; ///< Needs writing to FRAM
};

/**
 * @brief Typed handle to a configuration variable; tracks updates for Configuration::pushUpdatesToMemory().
 * @tparam T Must match the type in ConfigurationRegistry.h
 */
template <typename T>
class ConfigurationData {
public:
    /**
     * @brief Invalid placeholder until assigned from Configuration::getConfigurable<>().
     */
    ConfigurationData() = default;

    explicit ConfigurationData(BaseConfigurationData_s* b) : base(b) {}

    /**
     * @brief Returns T{} if invalid.
     */
    T get() const {
        if (!base || !base->data) return T{};
        return *reinterpret_cast<const T*>(base->data);
    }

    /**
     * @brief Sets the value if it passes the registry's validity check; false if rejected or invalid.
     */
    bool set(const T& newVal) {
        if (!base || !base->data) return false;
        if (getConfigurationValid(base->id, &newVal)) {
            *reinterpret_cast<T*>(base->data) = newVal;
            base->m_isUpdated = true;
            return true;
        }
        return false;
    }

    /**
     * @brief Sets the value without the registry's validity check.
     */
    bool forceSet(const T& newVal) {
        if (!base || !base->data) return false;
        *reinterpret_cast<T*>(base->data) = newVal;
        base->m_isUpdated = true;
        return true;
    }

    bool isValid() const { return base != nullptr; }

private:
    BaseConfigurationData_s* base = nullptr;
};

/**
 * @brief Owns all configuration variables: FRAM persistence, CRC/version validation, typed access.
 */
class Configuration {
public:
    constexpr static ConfigurationID_t REQUIRED_CONFIGS[] = {CONFIGURATION_VERSION_c, CONFIGURATION_ALL_ID_CRC_c, CONFIGURATION_CRC_c};

    /**
     * @brief Uses an external buffer, which must be declared alignas(std::max_align_t).
     * @details allConfigs must list every ID used in the build; duplicates are filtered out.
     */
    template <unsigned N, unsigned M>
    explicit Configuration(const ConfigurationIDSet_s (&allConfigs)[N], uint8_t (&buffer)[M]): m_dataBuffer(buffer), m_dataBufferMaxLength(M) {
        // Misaligned buffer would fault on Cortex-M0/M0+; skip construct() and report in setup().
        constexpr uintptr_t requiredAlignMask = alignof(std::max_align_t) - 1u;
        if ((reinterpret_cast<uintptr_t>(buffer) & requiredAlignMask) != 0u) {
            externalBufferMisaligned = true;
            return;
        }
        construct(allConfigs, N);
    }

    /**
     * @brief Uses the internal buffer; allConfigs must list every ID used in the build (duplicates filtered).
     */
    template <unsigned N>
    explicit Configuration(const ConfigurationIDSet_s (&allConfigs)[N]): m_dataBuffer(m_bufferDO_NOT_USE), m_dataBufferMaxLength(MAX_CONFIGURATION_LENGTH) {
        construct(allConfigs, N);
    }

    /**
     * @brief Reads from FRAM; restores defaults on CRC, ID-set CRC, or version mismatch.
     * @param id Index into HardwareAbstraction's FRAM array
     */
    void setup(HardwareAbstraction* hardware, uint8_t id);

    /**
     * @brief Returns a typed handle; the template ID gives a compile-time type check.
     */
    template <unsigned N>
    ConfigurationData<typename GetConfigurationType_s<N>::type> getConfigurable() {
        using T = typename GetConfigurationType_s<N>::type;
        const ConfigurationID_t id = ConfigurationID_t(N);
        BaseConfigurationData_s* base = configExists(id) ? getBaseConfigurationData(id) : nullptr;
        return ConfigurationData<T>(base);
    }

    /**
     * @brief Overrides a default; has no effect after setup().
     */
    template <unsigned N>
    void setDefault(const typename GetConfigurationType_s<N>::type& value) {
        if (m_hardware == nullptr) {
            getConfigurable<N>().set(value);
        } else {
            m_debug->warn("It's too late to be setting a configuration default");
        }
    }

    /**
     * @brief Writes updated variables to FRAM, coalescing adjacent ones; call every loop.
     */
    void pushUpdatesToMemory();

    /**
     * @brief Raw bytes of every registered config except the 3 internal CRC/version fields, in sorted-ID order.
     * @return nullptr (outLength 0) if no such configs are registered
     */
    const uint8_t* getConfigDataBuffer(uint32_t& outLength) const;

    uint32_t getConfigVersion() const { return m_configurationVersion.get(); }

    uint32_t getConfigKeyCrc() const { return m_configurationAllIdCRC.get(); }

    /**
     * @brief Formats a buffer laid out like getConfigDataBuffer() as "CONFIG\tNAME=value\t..." text.
     * @details Decodes with the live field layout (e.g. for a snapshot logged on an earlier boot); never touches live values.
     * @param version Reported separately because CONFIGURATION_VERSION_c is not part of rawConfigBytes
     */
    void formatBufferAsText(uint32_t version, const uint8_t* rawConfigBytes, uint32_t rawConfigBytesLen, char* buf, size_t bufSize) const;

private:
    BaseConfigurationData_s* getBaseConfigurationData(ConfigurationID_t id);

    bool hasError() const;

    void construct(const ConfigurationIDSet_s* allConfigs, uint16_t allConfigsLength);

    void readConfigFromMemory() const;

    bool configExists(ConfigurationID_t id) const;

    void sortConfigs();

    void assignMemory();

    uint32_t calculateCrc() const;

    uint32_t calculateAllIdCrc() const;

    // Construction-time failures; m_hardware is null until setup(), which reports them.
    bool constructorFailed = false;
    bool assignMemoryFailed = false;
    bool externalBufferMisaligned = false;

    uint8_t* m_dataBuffer;
    uint32_t m_dataBufferIndex = 0;
    const uint32_t m_dataBufferMaxLength = 0;

    // Over-aligned base; assignMemory() pads each entry to its type's alignof (Cortex-M0/M0+ traps on unaligned access).
    alignas(std::max_align_t) uint8_t m_bufferDO_NOT_USE[MAX_CONFIGURATION_LENGTH] = {};
    BaseConfigurationData_s m_configurations[MAX_CONFIGURATION_NUM] = {};
    uint32_t m_numConfigurations = 0;

    ConfigurationData<uint32_t> m_configurationCRC;
    ConfigurationData<uint32_t> m_configurationAllIdCRC;
    ConfigurationData<uint32_t> m_configurationVersion;

    HardwareAbstraction* m_hardware = nullptr;
    FramMemory* m_memory = nullptr;
    DebugStream* m_debug = nullptr;
};


#endif //PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H
