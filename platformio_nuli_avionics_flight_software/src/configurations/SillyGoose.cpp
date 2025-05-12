#include <Arduino.h>
#include "boards/SillyGoosePins.h"
#include "core/cli/SimpleFlag.h"
#include "core/cli/Parser.h"
#include "drivers/arduino/SerialDebug.h"
#include "drivers/arduino/ArduinoSystemClock.h"
#include "drivers/arduino/MS5607Sensor.h"
#include "drivers/arduino/ICM20602Sensor.h"
#include "drivers/arduino/S25FL512.h"
#include "drivers/arduino/ArduinoPyro.h"
#include "core/HardwareAbstraction.h"
#include "core/Configuration.h"
#include "core/Logger.h"
#include "drivers/arduino/ArduinoVoltageSensor.h"
#include "core/AvionicsCore.h"


ArduinoSystemClock arduinoClock;
SerialDebug serialDebug(true);

ArduinoPyro droguePyro(PYRO1_GATE_PIN, PYRO1_SENSE_PIN, PYRO_SENSE_THRESHOLD);
ArduinoPyro mainPyro(PYRO2_GATE_PIN, PYRO2_SENSE_PIN, PYRO_SENSE_THRESHOLD);

MS5607Sensor barometer;
ICM20602Sensor icm20602Sensor;
ArduinoVoltageSensor batteryVoltageSensor(VOLTAGE_SENSE_PIN, VOLTAGE_SENSE_SCALE );

S25FL512 flash(FLASH_CS_PIN);

Configuration configuration;
Logger logger;
Filters filter;
HardwareAbstraction hardware;
AvionicsCore avionicsCore;

void callback_name(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid) {
    Serial.println("testing12");
}

Parser myParser;
SimpleFlag stopTransmit("--stop", "Send stop", true, 255, callback_name);
BaseFlag* stopTransmitGroup[] = {&stopTransmit};

void getSerialInput(char* buffer) {
    if (! Serial.available()) {
        buffer[0] = '\0';
        return;
    }

    size_t bytesRead = Serial.readBytes(buffer, 254);
    buffer[bytesRead] = '\0';

    // clear serial buffer
    while (Serial.available()) {
        Serial.read();  // Discard any remaining bytes
    }
}

void serialSender() {
    char buf[255];  // @TODO: change into a macro/const
    getSerialInput(buf);

    // Only parse if we actually received something
    if (buf[0] != '\0') {
        Serial.print("Parsing input: ");
        Serial.println(buf);
        myParser.parse(buf);
    }
}

void setup() {
    myParser.addFlagGroup(stopTransmitGroup);

    // System configuration
    hardware.setDebugStream(&serialDebug);
    hardware.setSystemClock(&arduinoClock);
    // Devices
    hardware.addPyro(&droguePyro);
    hardware.addPyro(&mainPyro);
    hardware.addVoltageSensor(&batteryVoltageSensor);
    hardware.addBarometer(&barometer);
    hardware.addGenericSensor(&icm20602Sensor);
    hardware.addAccelerometer(icm20602Sensor.getAccelerometer());
    hardware.addGyroscope(icm20602Sensor.getGyroscope());
    hardware.addFlashMemory(&flash);

    hardware.setup();

    logger.erase();
    Serial.println("Done");
    logger.setup(&hardware);

    avionicsCore.setup(&hardware, &configuration, &logger);
}

void loop() {
    serialSender();
    myParser.runFlags();
    myParser.resetFlags();

    avionicsCore.loopOnce();
    logger.log();

//    Serial.print(hardware.getLoopDtMs());
//    Serial.print('\t');
//    Serial.print(barometer.getPressurePa());
//    Serial.print('\t');
//    Serial.print(barometer.getAltitudeM());
//    Serial.print('\t');
//    Serial.println(barometer.getTemperatureK());


//    avionicsCore.printDump();
}

