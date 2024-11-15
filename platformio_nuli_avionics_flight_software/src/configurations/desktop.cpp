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
#include <FlashMemory.h>
#include <CommunicationLink.h>

Barometer barometer;
Accelerometer accelerometer;
Gyroscope gyroscope;
Magnetometer magnetometer;

// Core objects accessible by all components
HardwareAbstraction hardware;
Configuration configuration;
Logger logger;
Filters filter;
AvionicsCore avionicsCore;

int main(int argc, char *argv[]) {
    hardware.addBarometer(&barometer);
    hardware.addAccelerometer(&accelerometer);
    hardware.addGyroscope(&gyroscope);
    hardware.addMagnetometer(&magnetometer);

    hardware.setup();
    configuration.setup(&hardware);
    logger.setup(&hardware);
    filter.setup(&configuration, &logger);
    avionicsCore.setup(&hardware, &configuration, &logger, &filter);

    while (true) {
        avionicsCore.loopOnce();
    }
}