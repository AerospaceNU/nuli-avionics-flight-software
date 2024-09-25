#include "HardwareAbstraction.h"

void HardwareAbstraction::setup() {
    for (int i = 0; i < m_numPyros; i++) m_pyroArray[i]->setup();
    for (int i = 0; i < m_numBarometers; i++) m_barometerArray[i]->setup();
    for (int i = 0; i < m_numAccelerometers; i++) m_accelerometerArray[i]->setup();
    for (int i = 0; i < m_numMagnetometers; i++) m_magnetometerArray[i]->setup();
    for (int i = 0; i < m_numGyroscopes; i++) m_gyroscopeArray[i]->setup();
    for (int i = 0; i < m_numGps; i++) m_gpsArray[i]->setup();
    for (int i = 0; i < m_numCommunicationLinks; i++) m_communicationLinkArray[i]->setup();
    for (int i = 0; i < m_numFlashMemory; i++) m_flashMemoryArray[i]->setup();
}

void HardwareAbstraction::readAllSensors() {
    for (int i = 0; i < m_numPyros; i++) m_pyroArray[i]->read();
    for (int i = 0; i < m_numBarometers; i++) m_barometerArray[i]->read();
    for (int i = 0; i < m_numAccelerometers; i++) m_accelerometerArray[i]->read();
    for (int i = 0; i < m_numMagnetometers; i++) m_magnetometerArray[i]->read();
    for (int i = 0; i < m_numGyroscopes; i++) m_gyroscopeArray[i]->read();
    for (int i = 0; i < m_numGps; i++) m_gpsArray[i]->read();
}

void HardwareAbstraction::readAllCommunicationLinks() {
    for (int i = 0; i < m_numCommunicationLinks; i++) m_communicationLinkArray[i]->read();
}








