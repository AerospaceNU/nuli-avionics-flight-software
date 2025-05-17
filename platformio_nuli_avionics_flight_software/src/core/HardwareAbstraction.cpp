#include "HardwareAbstraction.h"
#include <Arduino.h>

void HardwareAbstraction::setLoopRate(uint32_t loopRate) {
    double delay = 1.0 / loopRate;
    delay *= 1000;
    m_loopTime = (uint32_t) delay;
}

void HardwareAbstraction::setup() {
    // Setup core
    if (m_debugStream == nullptr || m_systemClock == nullptr || m_configuration == nullptr || m_configurationMemory == nullptr) {
        if(m_debugStream != nullptr) {
            m_debugStream->print("Debug stream, clock, configuration, and configuration memory required");
            m_debugStream->println();
        }
        while (true);
    }
    m_debugStream->setup();
    m_systemClock->setup();
    m_configurationMemory->setup();
    m_configuration->setup(m_configurationMemory, m_debugStream);
    
    for (int i = 0; i < m_numPyros; i++) m_pyroArray[i]->setup();
    for (int i = 0; i < m_numVoltageSensors; i++) m_voltageSensorArray[i]->setup();
    for (int i = 0; i < m_numBarometers; i++) m_barometerArray[i]->setup();
    for (int i = 0; i < m_numAccelerometers; i++) m_accelerometerArray[i]->setup();
    for (int i = 0; i < m_numMagnetometers; i++) m_magnetometerArray[i]->setup();
    for (int i = 0; i < m_numGyroscopes; i++) m_gyroscopeArray[i]->setup();
    for (int i = 0; i < m_numRadioLinks; i++) m_radioLinkArray[i]->setup();
    for (int i = 0; i < m_numFlashMemory; i++) m_flashMemoryArray[i]->setup();
    for (int i = 0; i < m_numGenericSensors; i++) m_genericSensorArray[i]->setup();
}

void HardwareAbstraction::readAllSensors() {
    for (int i = 0; i < m_numPyros; i++) m_pyroArray[i]->read();
    for (int i = 0; i < m_numVoltageSensors; i++) m_voltageSensorArray[i]->read();
    for (int i = 0; i < m_numBarometers; i++) m_barometerArray[i]->read();
    for (int i = 0; i < m_numAccelerometers; i++) m_accelerometerArray[i]->read();
    for (int i = 0; i < m_numMagnetometers; i++) m_magnetometerArray[i]->read();
    for (int i = 0; i < m_numGyroscopes; i++) m_gyroscopeArray[i]->read();
    for (int i = 0; i < m_numGenericSensors; i++) m_genericSensorArray[i]->read();
}

void HardwareAbstraction::readAllRadioLinks() {
    for (int i = 0; i < m_numRadioLinks; i++) m_radioLinkArray[i]->loopOnce();
}

void HardwareAbstraction::enforceLoopTime() {
    uint32_t end = getLoopTimestampMs() + 10;
    while (getRuntimeMs() < end);
    updateLoopTimestamp();
}









