#include "Configuration.h"
#include "core/HardwareAbstraction.h"
#include "util/CRC.h"

constexpr ConfigurationID_t Configuration::REQUIRED_CONFIGS[];

void Configuration::construct(const ConfigurationIDSet_s* allConfigs, const uint16_t allConfigsLength) {
    for (int i = 0; i < allConfigsLength; i++) {
        for (int j = 0; j < allConfigs[i].length; j++) {
            if (m_numConfigurations >= MAX_CONFIGURATION_NUM) {
                constructorFailed = true;
                break;
            }

            const ConfigurationID_t requestedConfig = allConfigs[i].data[j];
            if (!configExists(requestedConfig)) {
                m_configurations[m_numConfigurations].id = requestedConfig;
                m_numConfigurations++;
            }
        }
    }
    sortConfigs();
    assignMemory();

    for (uint32_t i = 0; i < m_numConfigurations; i++) {
        getConfigurationDefault(m_configurations[i].id, m_configurations[i].data);
    }
}

void Configuration::setup(HardwareAbstraction* hardware, const uint8_t id) {
    if (!hardware || hardware->getNumFramMemorys() <= id) {
        // With no hardware there is no way to report; spin so the watchdog or a debugger catches it.
        if (hardware) hardware->avionicsSystemError("Either no hardware or no fram");
        while (true) {}
    }
    m_hardware = hardware;
    m_memory = hardware->getFramMemory(id);
    m_debug = hardware->getDebugStream();

    if (constructorFailed) {
        m_hardware->avionicsSystemError("Too many configurations");
    }
    if (assignMemoryFailed) {
        m_hardware->avionicsSystemError("Configuration out of memory: total tracked size exceeds MAX_CONFIGURATION_LENGTH");
    }
    if (externalBufferMisaligned) {
        m_hardware->avionicsSystemError("Configuration external buffer not aligned to std::max_align_t — declare with alignas(std::max_align_t)");
    }

    m_configurationCRC = getConfigurable<CONFIGURATION_CRC_c>();
    m_configurationAllIdCRC = getConfigurable<CONFIGURATION_ALL_ID_CRC_c>();
    m_configurationVersion = getConfigurable<CONFIGURATION_VERSION_c>();

    // Defaults snapshot for restore; a variable-length array on the stack.
    uint8_t bufferDefaultValues[m_dataBufferMaxLength] = {};
    memcpy(bufferDefaultValues, m_dataBuffer, m_dataBufferMaxLength);

    readConfigFromMemory();

    if (hasError()) {
        memcpy(m_dataBuffer, bufferDefaultValues, m_dataBufferMaxLength);
        m_configurationAllIdCRC.set(calculateAllIdCrc());
        for (uint32_t i = 0; i < m_numConfigurations; i++) {
            m_configurations[i].m_isUpdated = true;
        }
    } else {
        // Clears the m_isUpdated raised by setDefault(); invalid stored values revert to their default.
        for (uint32_t i = 0; i < m_numConfigurations; i++) {
            if (!getConfigurationValid(m_configurations[i].id, m_configurations[i].data)) {
                memcpy(m_configurations[i].data, (m_configurations[i].data - m_dataBuffer) + bufferDefaultValues, m_configurations[i].size);
                m_configurations[i].m_isUpdated = true;
                m_debug->warn("%s invalid value, resetting to default", getConfigurationName(m_configurations[i].id));
            } else {
                m_configurations[i].m_isUpdated = false;
            }
        }
    }


    pushUpdatesToMemory();
}

bool Configuration::configExists(const ConfigurationID_t id) const {
    // Linear: called from the constructor before sorting
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
        // Cortex-M0/M0+ traps on unaligned access; alignof is a power of two, so masking is safe.
        const uint32_t alignment = getConfigurationAlignment(m_configurations[i].id);
        const uint32_t alignMask = alignment - 1u;
        m_dataBufferIndex = (m_dataBufferIndex + alignMask) & ~alignMask;
        const uint16_t configurationLength = getConfigurationLength(m_configurations[i].id);
        if (m_dataBufferIndex + configurationLength >= m_dataBufferMaxLength) {
            assignMemoryFailed = true;
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
    for (uint32_t i = 0; i < m_numConfigurations; i++) {
        if (m_configurations[i].m_isUpdated) {
            m_configurationCRC.set(calculateCrc());
            break;
        }
    }

    // Runs tracked by pointers, not a size sum, so alignment padding is included in each write.
    const uint8_t* runStart = nullptr;
    const uint8_t* runEnd = nullptr;
    for (uint32_t i = 0; i < m_numConfigurations; i++) {
        if (m_configurations[i].m_isUpdated) {
            m_configurations[i].m_isUpdated = false;
            if (runStart == nullptr) runStart = m_configurations[i].data;
            runEnd = m_configurations[i].data + m_configurations[i].size;
        } else if (runStart != nullptr) {
            m_memory->write(runStart - m_dataBuffer, runStart, runEnd - runStart);
            runStart = nullptr;
        }
    }

    if (runStart != nullptr) {
        m_memory->write(runStart - m_dataBuffer, runStart, runEnd - runStart);
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

bool Configuration::hasError() const {
    if (m_numConfigurations < 3 || m_configurations[0].id != CONFIGURATION_CRC_c || m_configurations[1].id != CONFIGURATION_ALL_ID_CRC_c || m_configurations[2].id != CONFIGURATION_VERSION_c) {
        m_hardware->avionicsSystemError("Required configurations for Configuration class not included");
        return true;
    }
    if (m_configurationCRC.get() != calculateCrc()) {
        m_debug->warn("Configuration invalid CRC, resetting to defaults");
        return true;
    }
    if (m_configurationAllIdCRC.get() != calculateAllIdCrc()) {
        m_debug->warn("Configuration invalid ID list, resetting to defaults %d", int(m_configurationAllIdCRC.get()));
        return true;
    }
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

void Configuration::formatBufferAsText(const uint32_t version, const uint8_t* rawConfigBytes, const uint32_t rawConfigBytesLen, char* buf, size_t bufSize) const {
    size_t pos = 0;
    auto append = [&](const char* s) {
        while (*s && pos + 1 < bufSize) buf[pos++] = *s++;
    };
    append("CONFIG");
    for (uint32_t i = 3; i < m_numConfigurations; i++) {
        const uint32_t offset = static_cast<uint32_t>(m_configurations[i].data - m_configurations[3].data);
        if (offset + m_configurations[i].size > rawConfigBytesLen) break;
        append("\t");
        append(getConfigurationName(m_configurations[i].id));
        append("=");
        if (pos < bufSize) pos += (size_t)getConfigurationPrint(m_configurations[i].id, rawConfigBytes + offset, buf + pos, bufSize - pos);
    }
    append("\tCONFIGURATION_VERSION=");
    if (pos < bufSize) pos += (size_t)mini_snprintf(buf + pos, (int)(bufSize - pos), "%u", (unsigned)version);
    if (pos < bufSize) buf[pos] = '\0';
    else if (bufSize > 0) buf[bufSize - 1] = '\0';
}

const uint8_t* Configuration::getConfigDataBuffer(uint32_t& outLength) const {
    // Indices 0-2 are the CRC/ID-CRC/version fields (ordering checked by hasError()).
    if (m_numConfigurations <= 3) {
        outLength = 0;
        return nullptr;
    }
    const uint8_t* start = m_configurations[3].data;
    const uint8_t* end = m_configurations[m_numConfigurations - 1].data + m_configurations[m_numConfigurations - 1].size;
    outLength = static_cast<uint32_t>(end - start);
    return start;
}
