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
#include <FlashMemory.h>
#include <CommunicationLink.h>
#include "drivers/arduino/ICM20948.h"
#include "drivers/arduino/UART_GPS.h"

// Hardware devices
Pyro pyro1(1, A0, 500);
Pyro pyro2(2, A1, Pyro::USE_DIGITAL_CONTINUITY);
Barometer barometer;
ICM20948 icm20948(5);
//GPS gps();
UART_GPS gps(&Serial1);
FlashMemory flashMemory;
RadioTransmitterLink radioTransmitterLink;
SerialConnectionLink serialConnectionLink;

// Core objects accessible by all components
HardwareAbstraction hardware;
Configuration configuration;
Logger logger;
// Components, declared here to use dependency injection
Filters filter;

// The core
AvionicsCore avionicsCore;

void setup() {
    // Arduino setup
    SPI.begin();
    Wire.begin();
    delay(2000);
    
   /*
    Serial.begin(115200);
    Serial.println("helooo");
    SPI.begin();
    Wire.begin();
    delay(2000);
    
    while (!Serial);      // Wait for Serial Monitor to connect (only for boards with native USB)
    Serial.println("Setting up hardware and components...");
    */
    // Add all hardware
    hardware.addPyro(&pyro1);
    hardware.addPyro(&pyro2);
    hardware.addBarometer(&barometer);
    hardware.addGPS(&gps);
    hardware.addFlashMemory(&flashMemory);
    hardware.addCommunicationLink(&radioTransmitterLink);
    hardware.addCommunicationLink(&serialConnectionLink);
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
    gps.setup();
    Serial.println("GPS setup complete.");

}

void loop() {
    avionicsCore.loopOnce();
    // Read GPS data
    gps.read();

    // Check if GPS data fits within buffer
    if (gps.checkDataFit()) {
        Serial.println("GPS data fits within the buffer:");
    } else {
        Serial.println("GPS data exceeds the buffer size!");
    }

    // Print the last received GPS data
    String lastData = gps.getLastGPSData();
    Serial.println("Last GPS Data:");
    Serial.println(lastData);

    // Print additional GPS information
    Serial.print("Latitude: "); Serial.println(gps.getLatitude());
    Serial.print("Longitude: "); Serial.println(gps.getLongitude());
    Serial.print("Altitude: "); Serial.println(gps.getAltitude());
    Serial.print("Fix Quality: "); Serial.println(gps.getFixQuality());
    Serial.print("Satellites Tracked: "); Serial.println(gps.getSatellitesTracked());
    Serial.print("HDOP: "); Serial.println(gps.getHDOP());

    // Delay to avoid flooding the Serial Monitor
    delay(1000);
}
