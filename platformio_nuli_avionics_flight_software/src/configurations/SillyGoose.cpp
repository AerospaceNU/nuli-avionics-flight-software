#include <Arduino.h>
#include <Avionics.h>
#include "MS5607Sensor.h"
#include "ICM20602Sensor.h"
#include "ArduinoSystemClock.h"
#include "HardwareAbstraction.h"
#include "AvionicsCore.h"
#include <SerialDebug.h>

ArduinoSystemClock arduinoClock;
SerialDebug serialDebug(true);
MS5607Sensor barometer;
ICM20602Sensor icm20602Sensor;

Configuration configuration;
Logger logger;
Filters filter;
HardwareAbstraction hardware;
AvionicsCore avionicsCore;

void setup() {
    hardware.addDebugStream(&serialDebug);
    hardware.addSystemClock(&arduinoClock);
    hardware.addBarometer(&barometer);
    hardware.addGenericSensor(&icm20602Sensor);
    hardware.addAccelerometer(icm20602Sensor.getAccelerometer());
    hardware.addGyroscope(icm20602Sensor.getGyroscope());

    hardware.setup();
    configuration.setup(&hardware);
    logger.setup(&hardware, &configuration);
    filter.setup(&configuration, &logger);
    avionicsCore.setup(&hardware, &configuration, &logger, &filter);
}

void loop() {
    avionicsCore.loopOnce();
    avionicsCore.printDump();
}

