#include "AvionicsCore.h"
#include "generic_hardware/Barometer.h"
#include "generic_hardware/Accelerometer.h"
#include "drivers/arduino/PayloadFlightData.h"

void AvionicsCore::setup(HardwareAbstraction* hardware,
                         Configuration* configuration,
                         Logger* logger) {
    m_hardware = hardware;
    m_configuration = configuration;
    m_logger = logger;
}

void AvionicsCore::loopOnce() {
    // Get the start timestamp for this loop
//    m_hardware->updateLoopTimestamp();
    // Read in sensor data. This data is accessible through
    m_hardware->readAllSensors();
}


void AvionicsCore::printDump() {
    DebugStream* debug = m_hardware->getDebugStream();

//    debug->print("time,");
//    for (int i = 0; i < m_hardware->getNumBarometers(); i++)
//        debug->print("Pressure, Temp, Alt,");
//    for (int i = 0; i < m_hardware->getNumAccelerometers(); i++)
//        debug->print("A_X, A_Y, A_Z,");
//    for (int i = 0; i < m_hardware->getNumGyroscopes(); i++)
//        debug->print("X_RADS, Y_RADS, Z_RADS,");
//    debug->println();


    debug->print(m_hardware->getLoopDtMs());
    debug->print(',');
    for (int i = 0; i < m_hardware->getNumBarometers(); i++) {
        Barometer* barometer = m_hardware->getBarometer(i);
        debug->print(barometer->getPressurePa());
        debug->print(',');
        debug->print(barometer->getTemperatureK());
        debug->print(',');
        debug->print(barometer->getAltitudeM());
        debug->print(',');
    }

    for (int i = 0; i < m_hardware->getNumAccelerometers(); i++) {
        Accelerometer* accelerometer = m_hardware->getAccelerometer(i);
        debug->print(accelerometer->getAccelerationsMSS().x);
        debug->print(',');
        debug->print(accelerometer->getAccelerationsMSS().y);
        debug->print(',');
        debug->print(accelerometer->getAccelerationsMSS().z);
        debug->print(',');
    }
    for (int i = 0; i < m_hardware->getNumGyroscopes(); i++) {
        Gyroscope* gyroscope = m_hardware->getGyroscope(i);
        debug->print(gyroscope->getVelocitiesRadS().x);
        debug->print(',');
        debug->print(gyroscope->getVelocitiesRadS().y);
        debug->print(',');
        debug->print(gyroscope->getVelocitiesRadS().z);
        debug->print(',');
    }
    debug->println();
}
