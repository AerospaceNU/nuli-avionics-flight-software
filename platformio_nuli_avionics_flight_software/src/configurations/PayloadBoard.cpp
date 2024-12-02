#include <Arduino.h>
#include <Avionics.h>
#include <GenericSensor.h>
#include <Barometer.h>
#include <Accelerometer.h>
#include <GPS.h>
#include <Gyroscope.h>
#include <Magnetometer.h>
#include <Pyro.h>
#include <AvionicsCore.h>
#include <HardwareAbstraction.h>
#include <Filters.h>
#include "ICM20948Sensor.h"
#include "MS8607Sensor.h"
#include "ArduinoSystemClock.h"

// Hardware devices
ArduinoSystemClock arduinoClock;
MS8607Sensor barometer;
ICM20948Sensor icm20948(5);

// Core objects accessible by all components
HardwareAbstraction hardware;
Configuration configuration;
Logger logger;
// Components, declared here to use dependency injection
Filters filter;

// The core
AvionicsCore avionicsCore;

void setup() {
    Serial.begin(9600);
    while(!Serial);
    SPI.begin();
    Wire.begin();

    Serial.println("Serial successfully started");

    hardware.addSystemClock(&arduinoClock);
    hardware.addBarometer(&barometer);
    // Add the ICM20948. This takes multiple steps because the ICM is actually 3 sensors in one
    hardware.addGenericSensor(&icm20948);
    hardware.addAccelerometer(icm20948.getAccelerometer());
    hardware.addGyroscope(icm20948.getGyroscope());
    hardware.addMagnetometer(icm20948.getMagnetometer());
    // Finish initializing all hardware
    hardware.setup();
    // Initialize other globals
    configuration.setup(&hardware);
    logger.setup(&hardware, &configuration);
    // Initialize components
    filter.setup(&configuration, &logger);
    // Initialize core
    avionicsCore.setup(&hardware, &configuration, &logger, &filter);
}

uint32_t lastTime = 0;

void loop() {
    avionicsCore.loopOnce();
    uint32_t time = millis();
    Serial.println(time - lastTime);
    lastTime = time;
}
