#ifndef PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H
#define PLATFORMIO_NULI_AVIONICS_FLIGHT_SOFTWARE_CONFIGURATION_H

#include "Avionics.h"
#include "ConfigurationRegistryWraper.h"
#include "core/HardwareAbstraction.h"
#include "core/generic_hardware/GenericHardware.h"
#include <cstddef>  // std::max_align_t
#include <cstdint>  // uintptr_t

/**
 * @struct BaseConfigurationData_s
 * @brief Data structure representing a configuration varable
 * @details Contains pointer to the actual configuration data
 */
struct BaseConfigurationData_s {
    uint8_t* data = nullptr; ///< pointer to the actual varable
    uint16_t size = 0; ///< Size of the varable
    ConfigurationID_t id = NONE_c; ///< ID of the varable
    bool m_isUpdated = false; ///< Tracks if the varable needs to be writen to memory
};

/**
 * @class ConfigurationData
 * @brief Wrapper object for users to interact with configuration variables
 * @details Tracks updates, allowing for automatic writes to FRAM with Configuration.pushUpdatesToMemory();
 * @tparam T Data type of the configuration varable. Must match ConfigurationRegistry.h
 */
template <typename T>
class ConfigurationData {
public:
    /**
     * @brief Default constructor
     * @details Used to allow temporary invalid objects to be made in classes
     * before the varable is assigned to a valid object with Configuration.getConfigurable<>();
     */
    ConfigurationData() = default;

    /**
     * @brief Constructs a new ConfigurationData, used internally to Configuration
     * @details This class acts largely as a pointer container, letting Configuration
     * data to be promoted to the templated type.
     * @param b BaseConfigurationData_s that this object will reference
     */
    explicit ConfigurationData(BaseConfigurationData_s* b) : base(b) {}

    /**
     * @brief Getter method for the configuration variable
     * @details Will return default constructed T if the default constructor was used.
     * @return The value of the configuration variable
     */
    T get() const {
        if (!base || !base->data) return T{}; // default-constructed T
        return *reinterpret_cast<const T*>(base->data);
    }

    /**
     * @brief Sets the value of the configuration varable
     * @details Does nothing if object was created with the default constructor
     * @param newVal Value to set the configuration variable to
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
     * @brief Sets the value of the configuration varable, but doesn't have a default value check
     * @details Does nothing if object was created with the default constructor, but doesn't have a default value check
     * @param newVal Value to set the configuration variable to
     */
    bool forceSet(const T& newVal) {
        if (!base || !base->data) return false;
        *reinterpret_cast<T*>(base->data) = newVal;
        base->m_isUpdated = true;
        return true;
    }

    /**
     * @brief Determines if the object is a valid configuration varable
     * @details If the Configuration object was not created with your
     * intended configuration varable, or if the default constructor was
     * used this will return false.
     * @return if the varable is valid
     */
    bool isValid() const { return base != nullptr; }

private:
    BaseConfigurationData_s* base = nullptr; ///< pointer to the underlying data
};

/**
 * @class Configuration
 * @brief Manages the configuration system
 * @details Handles interactions with FRAM, CRC validation, and interfacing with the rest of the code
 */
class Configuration {
public:
    constexpr static ConfigurationID_t REQUIRED_CONFIGS[] = {CONFIGURATION_VERSION_c, CONFIGURATION_ALL_ID_CRC_c, CONFIGURATION_CRC_c}; ///< All configuration variables required for this to function

    /**
     * @brief Construct a Configuration object with an external buffer
     * @details Takes in an array of ConfigurationIDSet_s (which is an array of ConfigurationID_t).
     * These arrays must contain all configuration IDs that will be used anywhere in the specific
     * build of code. Duplicates are expected in will be filtered out.
     * @tparam N Size of the array of ConfigurationIDSet_s
     * @tparam M Size of the external buffer
     * @param allConfigs 2D array of all ConfigurationID_t that are going to be in the configuration
     * @param buffer data buffer to store all the configuration variables in
     */
    template <unsigned N, unsigned M>
    explicit Configuration(const ConfigurationIDSet_s (&allConfigs)[N], uint8_t (&buffer)[M]): m_dataBuffer(buffer), m_dataBufferMaxLength(M) {
        // The external buffer must be over-aligned to std::max_align_t so any registered
        // type can land at a valid offset within it. The caller is responsible for
        // declaring it as: alignas(std::max_align_t) uint8_t buffer[M];
        // If they didn't, skip construct() — typed access from the registry would
        // unaligned-access on Cortex-M0/M0+ — and defer the error to setup().
        constexpr uintptr_t requiredAlignMask = alignof(std::max_align_t) - 1u;
        if ((reinterpret_cast<uintptr_t>(buffer) & requiredAlignMask) != 0u) {
            externalBufferMisaligned = true;
            return;
        }
        construct(allConfigs, N);
    }

    /**
     * @brief Construct a Configuration object using the internal buffer
     * @details Takes in an array of ConfigurationIDSet_s (which is an array of ConfigurationID_t).
     * These arrays must contain all configuration IDs that will be used anywhere in the specific
     * build of code. Duplicates are expected in will be filtered out.
     * @tparam N Size of the array of ConfigurationIDSet_s
     * @param allConfigs 2D array of all ConfigurationID_t that are going to be in the configuration
     */
    template <unsigned N>
    explicit Configuration(const ConfigurationIDSet_s (&allConfigs)[N]): m_dataBuffer(m_bufferDO_NOT_USE), m_dataBufferMaxLength(MAX_CONFIGURATION_LENGTH) {
        construct(allConfigs, N);
    }

    /**
     * @brief Reads in the Configuration from FRAM
     * @details Checks that the configuration passes a CRC check, version number check, and a CRC check
     * on all the IDs present (to track if the set of configuration varables has changed). If any errors are
     * present, it restores the default values.
     * @param hardware HardwareAbstraction to retrieve the FRAM and DebugStream from
     * @param id Index of the FRAM in the HardwareAbstraction FRAM array
     */
    void setup(HardwareAbstraction* hardware, uint8_t id);

    /**
     * @brief Gets a configuration varable
     * @details Returns a ConfigurationData, which is a wrapper class that provides an interface to
     * configuration varables. It tracks changes to the value, allowing for a seamless update proces
     * with pushUpdatesToMemory().
     * @tparam N Configuration varable ID to retrieve. Templating allows for compile time type checks
     * @return Configuration varable
     */
    template <unsigned N>
    ConfigurationData<typename GetConfigurationType_s<N>::type> getConfigurable() {
        using T = typename GetConfigurationType_s<N>::type;
        const ConfigurationID_t id = ConfigurationID_t(N);
        BaseConfigurationData_s* base = configExists(id) ? getBaseConfigurationData(id) : nullptr;
        return ConfigurationData<T>(base);
    }

    /**
     * @brief Overwrites the existing default value for a configuration varable
     * @details MUST be called prior to Configuration.setup() or it will have no effect
     * @tparam N Configuration varable ID
     * @param value New default value
     */
    template <unsigned N>
    void setDefault(const typename GetConfigurationType_s<N>::type& value) {
        // Ensure setup has not been called
        if (m_hardware == nullptr) {
            getConfigurable<N>().set(value);
        } else {
            m_debug->warn("It's too late to be setting a configuration default");
        }
    }

    /**
     * @brief Writes all updated configuration varables to FRAM
     * @details Should be called every loop. Manages CRCs and efficiently blocks FRAM writes
     */
    void pushUpdatesToMemory();

    /**
     * @brief Raw bytes of every registered "real" config value (excludes the 3 internal CRC/version
     * fields), in sorted-ID order
     * @details Lets a caller (BasicLogger::logConfig()) snapshot the whole configuration generically,
     * with no per-board struct to keep in sync as fields are added.
     * @param outLength Set to the number of valid bytes in the returned buffer (0 if none)
     * @return Pointer into the internal data buffer; nullptr if fewer than 4 configs are registered
     */
    const uint8_t* getConfigDataBuffer(uint32_t& outLength) const;

    /// @brief Schema-compatibility version (CONFIGURATION_VERSION_c) - tags a logged config snapshot
    uint32_t getConfigVersion() const { return m_configurationVersion.get(); }

    /// @brief CRC over the registered config ID set (CONFIGURATION_ALL_ID_CRC_c) - tags a logged config snapshot
    uint32_t getConfigKeyCrc() const { return m_configurationAllIdCRC.get(); }

    /**
     * @brief Formats an arbitrary config-value buffer as "CONFIG\tNAME=value\t..." text
     * @details rawConfigBytes must be laid out exactly like getConfigDataBuffer()'s own return (same
     * registered field set/order/size) - lets a caller decode a snapshot that ISN'T necessarily this
     * object's own live values (e.g. one read back from flash, logged during an earlier boot),
     * using today's live field list/offsets to interpret it. Never touches this object's own values.
     * @param version Schema-compatibility version to report alongside the fields (CONFIGURATION_VERSION_c
     * isn't part of rawConfigBytes - it's tracked separately by whatever logged the snapshot)
     * @param rawConfigBytes Buffer of config values in the same layout as getConfigDataBuffer()
     * @param rawConfigBytesLen Length of rawConfigBytes
     * @param buf Output text buffer
     * @param bufSize Size of buf
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

    bool constructorFailed = false;
    // Set by assignMemory() if it OOMs. assignMemory() runs from the constructor where
    // m_hardware is still null, so it can't call avionicsSystemError directly; setup()
    // checks this flag once m_hardware is assigned and reports there.
    bool assignMemoryFailed = false;
    // Set by the external-buffer constructor if the caller-supplied buffer isn't
    // aligned to std::max_align_t. Same deferred-report pattern as above.
    bool externalBufferMisaligned = false;

    uint8_t* m_dataBuffer;
    uint32_t m_dataBufferIndex = 0;
    const uint32_t m_dataBufferMaxLength = 0;

    // Overalign to std::max_align_t so the buffer's base satisfies any scalar's
    // alignof. Per-entry padding is then handled by assignMemory() using each
    // registered type's alignof. This lets the registry hold types of any
    // standard alignment (int64_t, double, etc.) without HardFaulting on
    // Cortex-M0/M0+ unaligned access.
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
