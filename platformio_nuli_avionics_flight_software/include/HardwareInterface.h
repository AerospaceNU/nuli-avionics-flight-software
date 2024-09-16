#ifndef DESKTOP_HARDWAREMANAGER_H
#define DESKTOP_HARDWAREMANAGER_H

#include <stdint.h>
#include <Arduino.h>
#include <CommonHardware.h>
#include <CommonStructs.h>


class HardwareInterface {
public:
    RawSensorData readAllSensors() {
        return {};
    }

    Messages readCommunicationLinks() {
        return {};
    }

    void executeEvents(TriggeredEvents* activeEvents) {

    }

    void writeCommunicationLinks(Messages *responses) {

    }

    void setup() {

    }

    void addCommunicationLink(CommunicationLink* communicationLink) {
        m_communicationLinks[0] = communicationLink;
    }

    void addPyro(Pyro* pyro) {
        m_pyros[0] = pyro;
    }

    void addBarometer(Barometer* barometer) {
        m_barometers[0] = barometer;
    }

    void addGyroscope(Gyroscope* gyroscope) {
        m_gyroscopes[0] = gyroscope;
    }

    void addAccelerometer(Accelerometer* accelerometer) {
        m_accelerometers[0] = accelerometer;
    }

    void addMagnetometer(Magnetometer* magnetometer) {
        m_magnetometers[0] = magnetometer;
    }

    void addGPS(GPS* gps) {
        m_gps[0] = gps;
    }

    void addFlashMemory(FlashMemory* flashMemory) {
        m_flashMemory[0] = flashMemory;
    }


private:
    Pyro* m_pyros[10];
    Barometer* m_barometers[4];
    Accelerometer* m_accelerometers[4];
    Magnetometer* m_magnetometers[4];
    Gyroscope* m_gyroscopes[4];
    GPS* m_gps[4];
    FlashMemory* m_flashMemory[4];
    CommunicationLink* m_communicationLinks[4];

};


#endif //DESKTOP_HARDWAREMANAGER_H
