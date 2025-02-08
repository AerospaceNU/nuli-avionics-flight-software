#include <Arduino.h>
#include <Avionics.h>
#include "MS5607Sensor.h"
#include "ICM20602Sensor.h"
#include "ArduinoSystemClock.h"
#include "HardwareAbstraction.h"
#include "AvionicsCore.h"
#include <SerialDebug.h>

//ArduinoSystemClock arduinoClock;
//SerialDebug serialDebug(true);
//MS5607Sensor barometer;
//ICM20602Sensor icm20602Sensor;
//
//Configuration configuration;
//Logger logger;
//Filters filter;
//HardwareAbstraction hardware;
//AvionicsCore avionicsCore;

#include "S25FL512.h"

S25FL512 flash(A5, &SPI);

void setup() {
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH);
    pinMode(5, OUTPUT);
    digitalWrite(5, HIGH);
    SPI.begin();
    pinMode(A5, OUTPUT);


    Serial.begin(9600);
    while (!Serial);
    Serial1.begin(9600);
    Serial.println("Starting");

    uint8_t num = flash.read(0);
    Serial.println(int(num));
    flash.eraseSector(0);
    flash.write(0, num + 1);

//    Serial.println(flash.readStatusRegister());
//    Serial.println((int) flash.read(3));
//    flash.eraseSector(0);
//    Serial.println((int) flash.read(3));
//    flash.write(3, 7);
//    Serial.println((int) flash.read(3));

//    hardware.setDebugStream(&serialDebug);
//    hardware.setSystemClock(&arduinoClock);
//    hardware.addBarometer(&barometer);
//    hardware.addGenericSensor(&icm20602Sensor);
//    hardware.addAccelerometer(icm20602Sensor.getAccelerometer());
//    hardware.addGyroscope(icm20602Sensor.getGyroscope());
////    hardware.addConfiguration(&configuration);
//
//    hardware.setup();
//    logger.setup(&hardware, &configuration);
//    filter.setup(&configuration, &logger);
//    avionicsCore.setup(&hardware, &configuration, &logger, &filter);
}

void loop() {
//    if(Serial1.available()) {
//        Serial.write(Serial1.read());
//    }

//    avionicsCore.loopOnce();
//    avionicsCore.printDump();
}

