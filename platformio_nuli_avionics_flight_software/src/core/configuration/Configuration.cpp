#include "Configuration.h"
#include "../HardwareAbstraction.h"
#include "util/CRC.h"

constexpr ConfigurationID_t Configuration::REQUIRED_CONFIGS[];

void Configuration::construct(const ConfigurationIDSet_s* allConfigs, const uint16_t allConfigsLength) {
    // Add all requested configs to the list, ignoring duplicates
    for (int i = 0; i < allConfigsLength; i++) {
        for (int j = 0; j < allConfigs[i].length; j++) {
            if (m_numConfigurations >= MAX_CONFIGURATION_NUM) {
                criticalError("Too many configurations");
                break;
            }

            const ConfigurationID_t requestedConfig = allConfigs[i].data[j];
            if (!configExists(requestedConfig)) {
                m_configurations[m_numConfigurations].id = requestedConfig;
                m_numConfigurations++;
            }
        }
    }
    // Sort the list to ensure a consistent order, then set them up with their respective memory
    sortConfigs();
    assignMemory();

    // Write all default values to the data structure
    for (uint32_t i = 0; i < m_numConfigurations; i++) {
        getConfigurationDefault(m_configurations[i].id, m_configurations[i].data);
    }
}

void Configuration::setup(HardwareAbstraction* hardware, const uint8_t id) {
    if (!hardware || hardware->getNumFramMemorys() < id) {
        criticalError("Either no hardware or no fram");
    }
    m_hardware = hardware;
    m_memory = hardware->getFramMemory(id);
    m_debug = hardware->getDebugStream();

    m_configurationCRC = getConfigurable<CONFIGURATION_CRC_c>();
    m_configurationAllIdCRC = getConfigurable<CONFIGURATION_ALL_ID_CRC_c>();
    m_configurationVersion = getConfigurable<CONFIGURATION_VERSION_c>();

    // Copy all the default values, in case they need to be restored.
    // This uses dynamic stack memory (i.e. alloca) (I think; could be wrong)
    // This is not ideal @todo fix. But also there isn't a way to fix it
    uint8_t bufferDefaultValues[m_dataBufferMaxLength] = {};
    memcpy(bufferDefaultValues, m_dataBuffer, m_dataBufferMaxLength);

    // Read in from memory
    readConfigFromMemory();

    // Handle restoring to default
    // Check version number
    if (hasError()) {
        // Restore default values
        memcpy(m_dataBuffer, bufferDefaultValues, m_dataBufferMaxLength);
        // Overwrite the all ID crc
        m_configurationAllIdCRC.set(calculateAllIdCrc());
        // Flag all values to be updated
        for (uint32_t i = 0; i < m_numConfigurations; i++) {
            m_configurations[i].m_isUpdated = true;
        }
    } else {
        // As we have just read in everything, nothing should be updated (unless it's invalid)
        // This is a hack that just overwrites setDefault()'s behavior of raising this flag
        // Lastly, we need to check that all values are valid, and reset invalid values to default
        for (uint32_t i = 0; i < m_numConfigurations; i++) {
            if (!getConfigurationValid(m_configurations[i].id, m_configurations[i].data)) {
                // This retrieves a specific default value from the buffer
                memcpy(m_configurations[i].data, (m_configurations[i].data - m_dataBuffer) + bufferDefaultValues, m_configurations[i].size);
                m_configurations[i].m_isUpdated = true;
                m_debug->warn("%s invalid value, resting to default", getConfigurationName(m_configurations[i].id));
            } else {
                m_configurations[i].m_isUpdated = false;
            }
        }
    }


    // Write default values
    pushUpdatesToMemory();
}

bool Configuration::configExists(const ConfigurationID_t id) const {
    // Can't use binary search, because it's called in the constructor prior to sorting
    for (uint32_t i = 0; i < m_numConfigurations; i++) {
        if (m_configurations[i].id == id) {
            return true;
        }
    }
    return false;
}

void Configuration::sortConfigs() {
    for (uint32_t i = 0; i < m_numConfigurations; ++i) {
        for (uint32_t j = 0; j < m_numConfigurations - 1 - i; ++j) {
            if (m_configurations[j].id > m_configurations[j + 1].id) {
                // Swap
                const ConfigurationID_t temp = m_configurations[j].id;
                m_configurations[j].id = m_configurations[j + 1].id;
                m_configurations[j + 1].id = temp;
            }
        }
    }
}

void Configuration::assignMemory() {
    m_dataBufferIndex = 0;
    for (uint32_t i = 0; i < m_numConfigurations; i++) {
        const uint16_t configurationLength = getConfigurationLength(m_configurations[i].id);
        if (m_dataBufferIndex + configurationLength >= m_dataBufferMaxLength) {
            criticalError("Out of memory error");
            m_numConfigurations = i;
            return;
        }
        m_configurations[i].size = configurationLength;
        m_configurations[i].data = m_dataBuffer + m_dataBufferIndex;
        m_dataBufferIndex += configurationLength;
    }
}

void Configuration::readConfigFromMemory() const {
    m_memory->read(0, m_dataBuffer, m_dataBufferIndex);
}

void Configuration::pushUpdatesToMemory() {
    const uint8_t* writeStartLocation = m_dataBuffer;
    uint32_t bytesToWrite = 0;

    // Update the CRC if any config has been updated
    for (uint32_t i = 0; i < m_numConfigurations; i++) {
        if (m_configurations[i].m_isUpdated) {
            m_configurationCRC.set(calculateCrc());
            break;
        }
    }

    for (uint32_t i = 0; i < m_numConfigurations; i++) {
        if (m_configurations[i].m_isUpdated) {
            m_configurations[i].m_isUpdated = false;
            if (bytesToWrite == 0) writeStartLocation = m_configurations[i].data;
            bytesToWrite += m_configurations[i].size;
        } else if (bytesToWrite > 0) {
            const uint32_t address = writeStartLocation - m_dataBuffer;
            m_memory->write(address, writeStartLocation, bytesToWrite);
            writeStartLocation = m_dataBuffer;
            bytesToWrite = 0;
        }
    }

    if (bytesToWrite > 0) {
        const uint32_t address = writeStartLocation - m_dataBuffer;
        m_memory->write(address, writeStartLocation, bytesToWrite);
    }
}

BaseConfigurationData_s* Configuration::getBaseConfigurationData(const ConfigurationID_t id) {
    int32_t left = 0;
    int32_t right = (int32_t)m_numConfigurations - 1;

    while (left <= right) {
        const int32_t mid = left + (right - left) / 2;
        const int32_t midName = m_configurations[mid].id;

        if (midName == id) {
            return &m_configurations[mid];
        } else if (midName < id) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return nullptr;
}

void Configuration::criticalError(const char* str) const {
    m_debug->setup();
    m_debug->error("Critical configuration error: %s", str);
    while (true);
}

bool Configuration::hasError() const {
    if (m_numConfigurations < 3 || m_configurations[0].id != CONFIGURATION_CRC_c || m_configurations[1].id != CONFIGURATION_ALL_ID_CRC_c || m_configurations[2].id != CONFIGURATION_VERSION_c) {
        criticalError("Required configurations for Configuration class not included");
    }
    // Ensure the memory has not been corrupted
    if (m_configurationCRC.get() != calculateCrc()) {
        m_debug->warn("Configuration invalid CRC, resetting to defaults");
        return true;
    }
    // Ensure the list of IDs is the same
    if (m_configurationAllIdCRC.get() != calculateAllIdCrc()) {
        m_debug->warn("Configuration invalid ID list, resetting to defaults %d", int(m_configurationAllIdCRC.get()));
        return true;
    }
    // Ensure the version is the correct one
    GetConfigurationType_s<CONFIGURATION_VERSION_c>::type defaultVersion;
    getConfigurationDefault(CONFIGURATION_VERSION_c, &defaultVersion);
    if (m_configurationVersion.get() != defaultVersion) {
        m_debug->warn("Configuration invalid version %d, resetting to defaults", int(m_configurationVersion.get()));
        return true;
    }
    return false;
}

uint32_t Configuration::calculateCrc() const {
    const uint8_t* start = m_configurations[1].data;
    const uint8_t* end = m_configurations[m_numConfigurations - 1].data + m_configurations[m_numConfigurations - 1].size;
    const uint32_t length = end - start;
    return crc32(start, length);
}

uint32_t Configuration::calculateAllIdCrc() const {
    ConfigurationID_t allIDs[MAX_CONFIGURATION_NUM];
    for (uint32_t i = 0; i < m_numConfigurations; i++) {
        allIDs[i] = m_configurations[i].id;
    }
    return crc32(&allIDs, m_numConfigurations * sizeof(ConfigurationID_t));
}
