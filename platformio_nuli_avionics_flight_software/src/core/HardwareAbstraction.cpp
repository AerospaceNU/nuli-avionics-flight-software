#include "HardwareAbstraction.h"
#include <Arduino.h>

void HardwareAbstraction::setup() {
    if (m_debugStream == nullptr) {
        setDebugStream(&voidDump);
    }

    m_debugStream->setup();
    for (int i = 0; i < m_numPyros; i++) m_pyroArray[i]->setup();
    for (int i = 0; i < m_numVoltageSensors; i++) m_voltageSensorArray[i]->setup();
    for (int i = 0; i < m_numBarometers; i++) m_barometerArray[i]->setup();
    for (int i = 0; i < m_numAccelerometers; i++) m_accelerometerArray[i]->setup();
    for (int i = 0; i < m_numMagnetometers; i++) m_magnetometerArray[i]->setup();
    for (int i = 0; i < m_numGyroscopes; i++) m_gyroscopeArray[i]->setup();
//    for (int i = 0; i < m_numGps; i++) m_gpsArray[i]->setup();
    for (int i = 0; i < m_numRadioLinks; i++) m_radioLinkArray[i]->setup();
    for (int i = 0; i < m_numFlashMemory; i++) m_flashMemoryArray[i]->setup();
    for (int i = 0; i < m_numGenericSensors; i++) m_genericSensorArray[i]->setup();
    for (int i = 0; i < m_numConfigurations; i++) m_configurationArray[i]->setup();
}

void HardwareAbstraction::readAllSensors() {
    for (int i = 0; i < m_numPyros; i++) m_pyroArray[i]->read();
    for (int i = 0; i < m_numVoltageSensors; i++) m_voltageSensorArray[i]->read();
    for (int i = 0; i < m_numBarometers; i++) m_barometerArray[i]->read();
    for (int i = 0; i < m_numAccelerometers; i++) m_accelerometerArray[i]->read();
    for (int i = 0; i < m_numMagnetometers; i++) m_magnetometerArray[i]->read();
    for (int i = 0; i < m_numGyroscopes; i++) m_gyroscopeArray[i]->read();
//    for (int i = 0; i < m_numGps; i++) m_gpsArray[i]->read();
    for (int i = 0; i < m_numGenericSensors; i++) m_genericSensorArray[i]->read();
}

void HardwareAbstraction::readAllRadioLinks() {
    for (int i = 0; i < m_numRadioLinks; i++) m_radioLinkArray[i]->loopOnce();
}








