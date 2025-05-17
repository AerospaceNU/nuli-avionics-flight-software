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


void Configuration::setup(ConfigurationMemory *memory, DebugStream *debugStream) {
    m_memory = memory;
    m_debug = debugStream;
    // Sort the list to ensure a consistent order, then set them up with their respective memory
    sortConfigs();
    assignMemory();

    // Read in from memory
    readConfigFromMemory();
}

template<unsigned N>
ConfigurationData<typename GetConfigType_s<N>::type>* Configuration::getConfigurable() {
    int left = 0;
    int right = m_numConfigurations - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        int midName = m_configurations[mid].name;

        if (midName == N) {
            return (ConfigurationData<typename GetConfigType_s<N>::type>*) &m_configurations[mid];
        } else if (midName < N) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return nullptr;
}

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
    uint16_t index = 0;
    for (uint32_t i = 0; i < m_numConfigurations; i++) {
        uint16_t configLength = getConfigLength(m_configurations[i].name);
        if (index + configLength >= m_dataBufferMaxLength) {
            outOfMemoryError();
            m_numConfigurations = i;
            return;
        }
        m_configurations[i].size = configLength;
        m_configurations[i].data = m_dataBuffer + index;
        index += configLength;
    }
}

void Configuration::readConfigFromMemory() {

}

void Configuration::pushUpdates() {

}

void Configuration::outOfMemoryError() {
    Serial.println("Configuration ran out of memory to start");
}








