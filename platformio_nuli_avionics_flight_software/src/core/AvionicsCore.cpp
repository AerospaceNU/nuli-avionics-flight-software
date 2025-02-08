#include "AvionicsCore.h"
#include <Barometer.h>
#include <Accelerometer.h>
#include <Arduino.h>

void AvionicsCore::setup(HardwareAbstraction* hardware,
                         Configuration* configuration,
                         Logger* logger,
                         Filters* filter) {

    m_hardware = hardware;
    m_configuration = configuration;
    m_logger = logger;
    m_filter = filter;
}

void AvionicsCore::loopOnce() {
    // Get the start timestamp for this loop
    m_hardware->updateLoopTimestamp();
    // Read in sensor data. This data is accessible through
    m_hardware->readAllSensors();
    m_hardware->readAllCommunicationLinks();
    // Filter sensors
    m_filter->runFilterOnce();
    // ^ sensorData = m_filter->runFilterOnce();
    // state = updateState();
    // for i in detectors
    //    detector.detect()

    // for i in outputs
    //

    m_logger->log();


    // End by pushing configuration updates to flash
    m_configuration->writeFlashIfUpdated();
}


void AvionicsCore::printDump() {
    DebugStream *debug = m_hardware->getDebugStreamArray();

    debug->print("time,");
    for (int i = 0; i < m_hardware->getNumBarometers(); i++)
        debug->print("Pressure, Temp, Alt,");
    for (int i = 0; i < m_hardware->getNumAccelerometers(); i++)
        debug->print("A_X, A_Y, A_Z,");
    for (int i = 0; i < m_hardware->getNumGyroscopes(); i++)
        debug->print("X_RADS, Y_RADS, Z_RADS,");
    debug->println();


    debug->print(m_hardware->getLoopDtMs());
    debug->print(',');
    for (int i = 0; i < m_hardware->getNumBarometers(); i++) {
        Barometer *barometer = &m_hardware->getBarometerArray()[i];
        debug->print(barometer->getPressurePa());
        debug->print(',');
        debug->print(barometer->getTemperatureK());
        debug->print(',');
        debug->print(barometer->getAltitudeM());
        debug->print(',');
    }

    for (int i = 0; i < m_hardware->getNumAccelerometers(); i++) {
        Accelerometer *accelerometer = &m_hardware->getAccelerometerArray()[i];
        debug->print(accelerometer->getAccelerationsMSS().x);
        debug->print(',');
        debug->print(accelerometer->getAccelerationsMSS().y);
        debug->print(',');
        debug->print(accelerometer->getAccelerationsMSS().z);
        debug->print(',');
    }
    for (int i = 0; i < m_hardware->getNumGyroscopes(); i++) {
        Gyroscope *gyroscope = &m_hardware->getGyroscopeArray()[i];
        debug->print(gyroscope->getVelocitiesRadS().x);
        debug->print(',');
        debug->print(gyroscope->getVelocitiesRadS().y);
        debug->print(',');
        debug->print(gyroscope->getVelocitiesRadS().z);
        debug->print(',');
    }
    debug->println();
}
