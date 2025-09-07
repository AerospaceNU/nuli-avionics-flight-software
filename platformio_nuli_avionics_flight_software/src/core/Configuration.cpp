#include "Configuration.h"
#include "HardwareAbstraction.h"

constexpr ConfigurationID_e Configuration::REQUIRED_CONFIGS[];

void Configuration::construct(const ConfigurationIDSet_s* allConfigs, uint16_t allConfigsLength) {
    // Add all requested configs to the list, ignoring duplicates
    for (int i = 0; i < allConfigsLength; i++) {
        for (int j = 0; j < allConfigs[i].length; j++) {
            if (m_numConfigurations >= MAX_CONFIGURATION_NUM) {
                outOfMemoryError();
                break;
            }

            ConfigurationID_e requestedConfig = allConfigs[i].data[j];
            if (!configExists(requestedConfig)) {
                m_configurations[m_numConfigurations].name = requestedConfig;
                m_numConfigurations++;
            }
        }
    }
}


void Configuration::setup(ConfigurationMemory* memory, DebugStream* debugStream) {
    m_memory = memory;
    m_debug = debugStream;
    // Sort the list to ensure a consistent order, then set them up with their respective memory
    sortConfigs();
    assignMemory();

    // Read in from memory
    readConfigFromMemory();
}

//template<unsigned N>
//ConfigurationData<typename GetConfigType_s<N>::type>* Configuration::getConfigurable()

bool Configuration::configExists(ConfigurationID_e name) const {
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
                ConfigurationID_e temp = m_configurations[j].name;
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

void Configuration::readConfigFromMemory() {
    m_memory->read(0, m_dataBuffer, m_dataBufferIndex);


}

void Configuration::pushUpdatesToMemory() {
    uint8_t* writeStartLocation = m_dataBuffer;
    uint32_t bytesToWrite = 0;

    for (uint32_t i = 0; i < m_numConfigurations; i++) {
        if (m_configurations[i].m_isUpdated) {
            m_configurations[i].m_isUpdated = false;
            if (bytesToWrite == 0) writeStartLocation = m_configurations[i].data;
            bytesToWrite += m_configurations[i].size;
        } else if (bytesToWrite > 0) {
            uint32_t address = writeStartLocation - m_dataBuffer;
            m_memory->write(address, writeStartLocation, bytesToWrite);
            writeStartLocation = m_dataBuffer;
            bytesToWrite = 0;
        }
    }

    if (bytesToWrite > 0) {
        uint32_t address = writeStartLocation - m_dataBuffer;
        m_memory->write(address, writeStartLocation, bytesToWrite);
    }
}

void Configuration::outOfMemoryError() {
    Serial.println("Configuration ran out of memory to start");
}








