#include "Avionics.h"
#include <Arduino.h>
#include "boards/SillyGoosePins.h"
#include "drivers/arduino/SerialDebug.h"
#include "drivers/arduino/ArduinoSystemClock.h"
#include "drivers/arduino/MS5607Sensor.h"
#include "drivers/arduino/ICM20602Sensor.h"
#include "drivers/arduino/S25FL512.h"
#include "drivers/arduino/ArduinoPyro.h"
#include "drivers/arduino/ArduinoFram.h"
#include "drivers/arduino/ArduinoVoltageSensor.h"
#include "core/HardwareAbstraction.h"
#include "core/Configuration.h"
#include "core/cli/SimpleFlag.h"
#include "core/cli/Parser.h"
#include "core/Logger.h"
#include "core/AvionicsCore.h"
#include "core/Configuration.h"

ArduinoPyro droguePyro(PYRO1_GATE_PIN, PYRO1_SENSE_PIN, PYRO_SENSE_THRESHOLD);
ArduinoPyro mainPyro(PYRO2_GATE_PIN, PYRO2_SENSE_PIN, PYRO_SENSE_THRESHOLD);

MS5607Sensor barometer;
ICM20602Sensor icm20602Sensor;
ArduinoVoltageSensor batteryVoltageSensor(VOLTAGE_SENSE_PIN, VOLTAGE_SENSE_SCALE);

S25FL512 flash(FLASH_CS_PIN);
ArduinoFram fram(FRAM_CS_PIN);

ArduinoSystemClock arduinoClock;
SerialDebug serialDebug(false);
ConfigurationIDSet_s allConfigs[] = {Configuration::REQUIRED_CONFIGS};
Configuration configuration(allConfigs);
HardwareAbstraction hardware;

void setup() {
    pinMode(LIGHT_PIN, OUTPUT);
    digitalWrite(LIGHT_PIN, HIGH);

    // System
    hardware.setLoopRate(100);
    hardware.setConfiguration(&configuration);
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
    hardware.addFramMemory(&fram);

    hardware.setup();
}


void loop() {
    hardware.enforceLoopTime();
    hardware.readAllSensors();

    digitalWrite(LIGHT_PIN, ((hardware.getLoopTimestampMs() / 200) % 2 == 0) ? LOW : HIGH);

    Serial.println(hardware.getLoopDtMs());
}

