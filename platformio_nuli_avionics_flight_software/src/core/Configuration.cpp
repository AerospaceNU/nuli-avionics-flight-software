#include "Configuration.h"
#include "HardwareAbstraction.h"

constexpr ConfigurationID_t Configuration::REQUIRED_CONFIGS[];

void Configuration::construct(const ConfigurationIDSet_s* allConfigs, const uint16_t allConfigsLength) {
    // Add all requested configs to the list, ignoring duplicates
    for (int i = 0; i < allConfigsLength; i++) {
        for (int j = 0; j < allConfigs[i].length; j++) {
            if (m_numConfigurations >= MAX_CONFIGURATION_NUM) {
                outOfMemoryError();
                break;
            }

            const ConfigurationID_t requestedConfig = allConfigs[i].data[j];
            if (!configExists(requestedConfig)) {
                m_configurations[m_numConfigurations].name = requestedConfig;
                m_numConfigurations++;
            }
        }
    }
    // Sort the list to ensure a consistent order, then set them up with their respective memory
    sortConfigs();
    assignMemory();

    // Write all default values to the data structure
    for (uint32_t i = 0; i < m_numConfigurations; i++) {
        getConfigurationDefault(m_configurations[i].name, m_configurations[i].data);
    }
}


void Configuration::setup(HardwareAbstraction *hardware, const uint8_t id) {
    if (!hardware || hardware->getNumFramMemorys() < id) {
        outOfMemoryError();
    }
    m_hardware = hardware;
    m_memory = hardware->getFramMemory(id);
    m_debug = hardware->getDebugStream();

    // Copy all the default values, in case they need to be restored
    uint8_t bufferDefaultValues[sizeof(m_buffer)] = {};
    memcpy(bufferDefaultValues, m_buffer, sizeof(m_buffer));

    // Read in from memory
    readConfigFromMemory();

    // Handle restoring to default
    if (getConfigurable<CONFIGURATION_VERSION_c>().get() != 0) {
        // Restore default values
        memcpy(m_buffer, bufferDefaultValues, sizeof(m_buffer));
        // Flag all values to be updated, allowing
        for (uint32_t i = 0; i < m_numConfigurations; i++) {
            m_configurations[i].m_isUpdated = true;
        }
        // Write default values
        pushUpdatesToMemory();
        // Notify user
        m_debug->println("Configuration invalid, resetting to defaults");
    } else {
        // As we have just read in everything, nothing should be updated
        // This is a hack that just overwrites setDefault()'s unintended behavior of raising this flag
        for (uint32_t i = 0; i < m_numConfigurations; i++) {
            m_configurations[i].m_isUpdated = false;
        }
    }
}

bool Configuration::configExists(const ConfigurationID_t name) const {
    for (uint32_t i = 0; i < m_numConfigurations; i++) {
        if (m_configurations[i].name == name) {
            return true;
        }
    }
    return false;
}

void Configuration::sortConfigs() {
    for (uint32_t i = 0; i < m_numConfigurations; ++i) {
        for (uint32_t j = 0; j < m_numConfigurations - 1 - i; ++j) {
            if (m_configurations[j].name > m_configurations[j + 1].name) {
                // Swap
                ConfigurationID_t temp = m_configurations[j].name;
                m_configurations[j].name = m_configurations[j + 1].name;
                m_configurations[j + 1].name = temp;
            }
        }
    }
}

void Configuration::assignMemory() {
    m_dataBufferIndex = 0;
    for (uint32_t i = 0; i < m_numConfigurations; i++) {
        uint16_t configurationLength = getConfigurationLength(m_configurations[i].name);
        if (m_dataBufferIndex + configurationLength >= m_dataBufferMaxLength) {
            outOfMemoryError();
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

void Configuration::outOfMemoryError() const {
    m_debug->println("Configuration ran out of memory to start");
    while (true);
}








