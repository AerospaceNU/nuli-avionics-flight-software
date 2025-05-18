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
SerialDebug serialDebug(true);
ConfigurationID_e configSet[] = {RADIO_FREQUENCY};
ConfigurationID_e configSet2[] = {CONFIGURATION_CRC, CONFIGURATION_VERSION, CONFIGURATION_VERSION_HASH};
ConfigurationIDSet_s allConfigs[] = {configSet2, configSet, Configuration::REQUIRED_CONFIGS};
Configuration configuration(allConfigs);
HardwareAbstraction hardware;

void setup() {
    pinMode(LIGHT_PIN, OUTPUT);
    digitalWrite(LIGHT_PIN, HIGH);
    pinMode(FLASH_CS_PIN, OUTPUT);
    digitalWrite(FLASH_CS_PIN, HIGH);
    pinMode(FRAM_CS_PIN, OUTPUT);
    digitalWrite(FRAM_CS_PIN, HIGH);

    Serial.begin(9600);
    while (!Serial);
    Serial.println("Starting");
    // System
    hardware.setLoopRate(10);
    hardware.setConfiguration(&configuration);
    hardware.setDebugStream(&serialDebug);
    hardware.setSystemClock(&arduinoClock);
    hardware.setConfigurationMemory(&fram);
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

    ConfigurationData<uint32_t>* configVersion = configuration.getConfigurable<CONFIGURATION_VERSION>();
    ConfigurationData<uint32_t>* configHash = configuration.getConfigurable<CONFIGURATION_VERSION_HASH>();
    ConfigurationData<uint32_t>* configCrc = configuration.getConfigurable<CONFIGURATION_CRC>();
    ConfigurationData<float>* radioFrequency = configuration.getConfigurable<RADIO_FREQUENCY>();

    Serial.println(configVersion->get());
    Serial.println(configHash->get());
    Serial.println(configCrc->get());
    Serial.println(radioFrequency->get());
    Serial.println();

    configVersion->set(12);
    configHash->set(332);
//    configCrc->set(7);
    radioFrequency->set(2433.77);
    configuration.pushUpdates();

    Serial.println(configVersion->get());
    Serial.println(configHash->get());
    Serial.println(configCrc->get());
    Serial.println(radioFrequency->get());
}


void loop() {
    hardware.enforceLoopTime();
    hardware.readAllSensors();

    digitalWrite(LIGHT_PIN, ((hardware.getLoopTimestampMs() / 200) % 2 == 0) ? LOW : HIGH);

//    Serial.println(hardware.getLoopDtMs());
    configuration.pushUpdates();
}

