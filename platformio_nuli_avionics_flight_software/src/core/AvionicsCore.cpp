#include "AvionicsCore.h"
#include "generic_hardware/Barometer.h"
#include "generic_hardware/Accelerometer.h"

void AvionicsCore::setup(HardwareAbstraction* hardware,
                         Configuration* configuration,
                         Logger* logger,
                         Filters* filter, USLI2025Payload* payload) {

    m_hardware = hardware;
    m_configuration = configuration;
    m_logger = logger;
    m_filter = filter;
    m_payload = payload;
}

#include "RunningMedian.h"

#define RAD_TO_DEG_2 57.2958 // Conversion factor from radians to degrees

#include <cmath>

double calculateTilt(double ax, double ay, double az) {
    double magnitude = sqrt(ax * ax + ay * ay + az * az);
    double angle = acos(ay / magnitude) * RAD_TO_DEG_2; // Remove fabs(ay) to get correct range
    return angle; // Now it correctly returns values from 0 to 180 degrees
}

RunningMedian altitudeFilter = RunningMedian(10);
RunningMedian batteryFilter = RunningMedian(10);
RunningMedian temperatureFilter = RunningMedian(10);
RunningMedian orientationFilter = RunningMedian(10);
RunningMedian velocityFilter = RunningMedian(3);

double lastAltitude = 0;

void AvionicsCore::loopOnce() {
    // Get the start timestamp for this loop
    m_hardware->updateLoopTimestamp();
    // Read in sensor data. This data is accessible through
    m_hardware->readAllSensors();

    float batVolt = (float) m_hardware->getVoltageSensor(0)->getVoltage();  // float(analogRead(A4)) / float(22.008);

    if (log) {
        m_logger->log(batVolt);
    }

    Vector3D_s accelerationsMss = m_hardware->getAccelerometer(0)->getAccelerationsMSS();

    double tilt = calculateTilt(accelerationsMss.x, accelerationsMss.y, accelerationsMss.z);

    altitudeFilter.add((float) m_hardware->getBarometer(0)->getAltitudeM());
    temperatureFilter.add((float) m_hardware->getBarometer(0)->getTemperatureK());
    batteryFilter.add(batVolt);
    orientationFilter.add((float) tilt);


    double filteredAlt = altitudeFilter.getMedian();
    double velocity = double(filteredAlt - lastAltitude) / (double(m_hardware->getLoopDtMs()) / 1000.0);
    velocityFilter.add((float) velocity);

    uint32_t runtime = m_hardware->getLoopTimestampMs();
    uint32_t dt = m_hardware->getLoopDtMs();
    double altitudeM = altitudeFilter.getMedian();
    double velocityMS = abs(velocityFilter.getAverage());
    double netAccelMSS = 0;
    double orientationDeg = orientationFilter.getMedian();
    double temp = temperatureFilter.getMedian();
    double batteryVoltage = batteryFilter.getMedian();

//    Serial.print(runtime);
//    Serial.print('\t');
//    Serial.print(dt);
//    Serial.print('\t');
//    Serial.print(altitudeM);
//    Serial.print('\t');
//    Serial.print(velocityMS);
//    Serial.print('\t');
//    Serial.print(netAccelMSS);
//    Serial.print('\t');
//    Serial.print(orientationDeg);
//    Serial.print('\t');
//    Serial.print(temp);
//    Serial.print('\t');
//    Serial.print(batteryVoltage);
//    Serial.println('\t');

    m_payload->loopOnce(runtime, dt, altitudeM, velocityMS, netAccelMSS, orientationDeg, temp, batteryVoltage);

    lastAltitude = filteredAlt;
}


void AvionicsCore::printDump() {
    DebugStream* debug = m_hardware->getDebugStream();

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
