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
#include "S25FL512.h"
#include "ArduinoSystemClock.h"

const int BUFFER_SIZE = 16;  // Adjust buffer size as needed
char serialInputBuffer[BUFFER_SIZE];
int bufferIndex = 0;

bool isOffloading = false;
uint32_t currentOffloadAddress = 0;

uint8_t offloadReadBuffer[256];

// Hardware devices
ArduinoSystemClock arduinoClock;
MS8607Sensor barometer;
ICM20948Sensor icm20948(5);
S25FL512 s25fl512(A5);

// Core objects accessible by all components
HardwareAbstraction hardware;
Configuration configuration;
Logger logger;
// Components, declared here to use dependency injection
Filters filter;

void cliTick() {
    while (Serial.available()) {
        char c = Serial.read();
        // Ignore non-printable characters (except newline)
        if (c == '\r') return;

        // Add character to buffer if within size limit
        if (bufferIndex < BUFFER_SIZE - 1) {
            serialInputBuffer[bufferIndex++] = c;
        }

        // Detect end of command (newline or space-based parsing)
        if (c == '\n' || bufferIndex >= BUFFER_SIZE - 1) {
            serialInputBuffer[bufferIndex] = '\0'; // Null-terminate string
            bufferIndex = 0;  // Reset buffer for next message
            
            // Check if the received string matches "offload"
            if (true || strcmp(serialInputBuffer, "offload") == 0) {
                isOffloading = true;
                currentOffloadAddress = 0;
            }
        } 
    }

    while (isOffloading) {
        uint32_t readLength = logger.offloadData(currentOffloadAddress, offloadReadBuffer, sizeof(offloadReadBuffer));
        if (readLength == 0) {
            isOffloading = false;
        } else {
            Serial.write(offloadReadBuffer, readLength);
        }
        currentOffloadAddress += readLength;
    }
}


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
    hardware.addFlashMemory(&s25fl512);
    pinMode(A5, OUTPUT);
    digitalWrite(A5, HIGH);
    digitalWrite(8, HIGH);

    // s25fl512.eraseAll();
    // Serial.println("erase complete");

    
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
    cliTick();
    lastTime = time;
}
