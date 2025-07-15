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
#include "drivers/arduino/IndicatorLED.h"
#include "drivers/arduino/IndicatorBuzzer.h"
#include "core/HardwareAbstraction.h"
#include "core/Configuration.h"
#include "core/altitude_kf.h"
#include "core/cli/SimpleFlag.h"
#include "core/cli/ArgumentFlag.h"
#include "core/cli/Parser.h"
#include "core/Logger.h"
#include "core/AvionicsCore.h"
#include "core/Configuration.h"
#include "core/altitude_kf.h"

ArduinoPyro droguePyro(PYRO1_GATE_PIN, PYRO1_SENSE_PIN, PYRO_SENSE_THRESHOLD);
ArduinoPyro mainPyro(PYRO2_GATE_PIN, PYRO2_SENSE_PIN, PYRO_SENSE_THRESHOLD);

MS5607Sensor barometer;
ICM20602Sensor icm20602;
ArduinoVoltageSensor batteryVoltageSensor(VOLTAGE_SENSE_PIN, VOLTAGE_SENSE_SCALE);

S25FL512 flash(FLASH_CS_PIN);
ArduinoFram fram(FRAM_CS_PIN);

IndicatorLED led(LIGHT_PIN);
IndicatorBuzzer buzzer(BUZZER_PIN, 4000);

AltitudeKf altitudeKf;

ArduinoSystemClock arduinoClock;
SerialDebug serialDebug(false);
ConfigurationID_e configSet[] = {DROGUE_DELAY, MAIN_ELEVATION};
ConfigurationID_e configSet2[] = {CONFIGURATION_CRC, CONFIGURATION_VERSION, CONFIGURATION_VERSION_HASH,
                                  RADIO_FREQUENCY};
ConfigurationIDSet_s allConfigs[] = {configSet2, configSet, Configuration::REQUIRED_CONFIGS};
Configuration configuration(allConfigs);
HardwareAbstraction hardware;

Parser cliParser;

// Define Callbacks //
void callback_none(const char *name, uint8_t *data, uint32_t length, uint8_t group_uid, uint8_t flag_uid,
                   BaseFlag *dependency) {}

void callback_erase(const char *name, uint8_t *data, uint32_t length, uint8_t group_uid, uint8_t flag_uid,
                    BaseFlag *dependency) {
    Serial.println("Erasing all");
    flash.eraseAll();
}

void callback_offload(const char *name, uint8_t *data, uint32_t length, uint8_t group_uid, uint8_t flag_uid,
                      BaseFlag *dependency) {
    if (dependency != nullptr && dependency->isSet()) {
        Serial.println("Dependency set!");
        // do something
    } else {
      Serial.println("Dependency not set!");
    }
}

void callback_testfire(const char *name, uint8_t *data, uint32_t length, uint8_t group_uid, uint8_t flag_uid,
                       BaseFlag *dependency) {
    if (length >= sizeof(int)) {
        // Extract the integer value from the data buffer
        int value = 0;
        memcpy(&value, data, sizeof(int));

        Serial.print("Testfire received integer value: ");
        Serial.println(value);
    } else {
        Serial.println("Error: Insufficient data for integer type");
    }
}

void callback_mainAltitude(const char *name, uint8_t *data, uint32_t length, uint8_t group_uid, uint8_t flag_uid,
                           BaseFlag *dependency) {
    printf("Group UID: %u, Flag UID: %u\n", group_uid, flag_uid);

    if (length >= sizeof(float)) {
        // Extract the float value from the data buffer
        float value = 0.0f;
        memcpy(&value, data, sizeof(float));

        Serial.println("Received float value: ");
        Serial.print(value);

        ConfigurationData<float> *configMainAltitude = configuration.getConfigurable<MAIN_ELEVATION>();
        configMainAltitude->set(value);
    } else {
        Serial.println("Error: Insufficient data for float type\n");
    }
}

void callback_drogueDelay(const char *name, uint8_t *data, uint32_t length, uint8_t group_uid, uint8_t flag_uid,
                          BaseFlag *dependency) {
    if (length >= sizeof(float)) {
        // Extract the float value from the data buffer
        float value = 0.0f;
        memcpy(&value, data, sizeof(float));

        Serial.print("Received float value: ");
        Serial.println(value);

        ConfigurationData<float> *configDrogueDelay = configuration.getConfigurable<DROGUE_DELAY>();
        configDrogueDelay->set(value);
    } else {
        Serial.println("Error: Insufficient data for float type");
    }
}

void getSerialInput(char* buffer) {
  if (! Serial.available()) {
    return;
  }

  size_t bytesRead = Serial.readBytes(buffer, 63);
  buffer[bytesRead] = '\0';
}

// Define flags //
SimpleFlag erase("--erase", "Send start", true, 255, callback_erase);

SimpleFlag offload("--offload", "Send start", true, 255, callback_offload);
SimpleFlag offload_binary("-b", "binary", false, 255, callback_none);
ArgumentFlag<int> testfire("--testfire", "Send start", true, 255, callback_testfire);
ArgumentFlag<float> mainAltitude("--mainAltitude", "Send start", true, 255, callback_mainAltitude);
ArgumentFlag<float> drogueDelay("--drogueDelay", "Send start", true, 255, callback_drogueDelay);

BaseFlag *eraseGroup[] = {&erase};
BaseFlag *offloadGroup[] = {&offload, &offload_binary};
BaseFlag *testfireGroup[] = {&testfire};
BaseFlag *mainAltitudeGroup[] = {&mainAltitude};
BaseFlag *drogueDelayGroup[] = {&drogueDelay};



void setup() {
    pinMode(FLASH_CS_PIN, OUTPUT);
    digitalWrite(FLASH_CS_PIN, HIGH);
    pinMode(FRAM_CS_PIN, OUTPUT);
    digitalWrite(FRAM_CS_PIN, HIGH);

    altitudeKf.calculateDiscreteTimeA(.01);

    Serial.begin(9600);
    // while (!Serial);
    Serial.println("Starting");
    // System
    hardware.setLoopRate(100);
    hardware.setDebugStream(&serialDebug);
    hardware.setSystemClock(&arduinoClock);
    hardware.setConfigurationMemory(&fram);
    hardware.setConfiguration(&configuration);
    // configuration.setDefaultValue<RADIO_FREQUENCY>(915.32);
    // Devices
    hardware.addPyro(&droguePyro);
    hardware.addPyro(&mainPyro);
    hardware.addVoltageSensor(&batteryVoltageSensor);
    hardware.addBarometer(&barometer);
    hardware.addGenericSensor(&icm20602);
    hardware.addAccelerometer(icm20602.getAccelerometer());
    hardware.addGyroscope(icm20602.getGyroscope());
    hardware.addFlashMemory(&flash);
    hardware.addIndicator(&led);
    hardware.addIndicator(&buzzer);

    hardware.setup();

    cliParser = Parser();

    // setup dependency
    offload.setDependency(&offload_binary);

    cliParser.addFlagGroup(eraseGroup);
    cliParser.addFlagGroup(offloadGroup);
    cliParser.addFlagGroup(testfireGroup);
    cliParser.addFlagGroup(mainAltitudeGroup);
    cliParser.addFlagGroup(drogueDelayGroup);

    // ConfigurationData<uint32_t>* configVersion = configuration.getConfigurable<CONFIGURATION_VERSION>();
    // ConfigurationData<uint32_t>* configHash = configuration.getConfigurable<CONFIGURATION_VERSION_HASH>();
    // ConfigurationData<uint32_t>* configCrc = configuration.getConfigurable<CONFIGURATION_CRC>();
    // ConfigurationData<float>* radioFrequency = configuration.getConfigurable<RADIO_FREQUENCY>();
    //
    // Serial.println(configVersion->get());
    // Serial.println(configHash->get());
    // Serial.println(configCrc->get());
    // Serial.println(radioFrequency->get());
    // Serial.println();
    //
    // configVersion->set(912);
    // configHash->set(9332);
    // configCrc->set(97);
    // radioFrequency->set(92433.77);
    // configuration.pushUpdates();
    //
    // Serial.println(configVersion->get());
    // Serial.println(configHash->get());
    // Serial.println(configCrc->get());
    // Serial.println(radioFrequency->get());
}


void loop() {
    /*
    char input[64] = {0};
    getSerialInput(input);

    // parses user input
    if (cliParser.parse(input) == 0) {
      Serial.println("Running flags");
      cliParser.runFlags();
      cliParser.resetFlags();
    }

    uint32_t start = micros();
    hardware.enforceLoopTime();
    hardware.readAllSensors();

    for(int i = 0; i < hardware.getNumIndicators(); i++) {
        if((hardware.getLoopTimestampMs() / 250) % 2 == 0) {
            hardware.getIndicator(i)->on();
        } else {
            hardware.getIndicator(i)->off();
        }
    }
     */

    // Serial.print()
    
    altitudeKf.predict(hardware.getLoopTimestampMs() / 1000.0);
    altitudeKf.dataUpdate(barometer.getAltitudeM(), icm20602.getAccelerometer()->getAccelerationsMSS().z);

    configuration.pushUpdates();

    uint32_t end = micros();
    Serial.print(altitudeKf.getAltitude());
    Serial.print('\t');
    Serial.print((end - start) / 1000.0);
    Serial.print('\t');
    Serial.print(batteryVoltageSensor.getVoltage());
    Serial.print('\t');
    Serial.println(barometer.getAltitudeM());
}
















