#include "Avionics.h"
#include <Arduino.h>
#include "pinmaps/SillyGoosePinmap.h"
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
#include "core/StateDeterminer.h"

#define DEFAULT_FLAT_UID 255

// Hardware
ArduinoSystemClock arduinoClock;
SerialDebug serialDebug(false);
MS5607Sensor barometer;
ICM20602Sensor icm20602;
ArduinoPyro droguePyro(PYRO1_GATE_PIN, PYRO1_SENSE_PIN, PYRO_SENSE_THRESHOLD);
ArduinoPyro mainPyro(PYRO2_GATE_PIN, PYRO2_SENSE_PIN, PYRO_SENSE_THRESHOLD);
ArduinoVoltageSensor batteryVoltageSensor(VOLTAGE_SENSE_PIN, VOLTAGE_SENSE_SCALE);
S25FL512 flash(FLASH_CS_PIN);
ArduinoFram fram(FRAM_CS_PIN);
IndicatorLED led(LIGHT_PIN);
IndicatorBuzzer buzzer(BUZZER_PIN, 4000);

AltitudeKf altitudeKf;
StateDeterminer stateDeterminer;
HardwareAbstraction hardware;
Parser cliParser;

ConfigurationID_e sillyGooseRequiredConfigs[] = {DROGUE_DELAY, MAIN_ELEVATION};
Configuration configuration({
        sillyGooseRequiredConfigs,
        Configuration::REQUIRED_CONFIGS,
        StateDeterminer::REQUIRED_CONFIGS
    });
ConfigurationData<float>* mainElevation;
ConfigurationData<uint32_t>* drogueDelay;

// Define Callbacks //
void callback_none(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid,
                   BaseFlag* dependency) {}

void callback_erase(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid,
                    BaseFlag* dependency) {
    Serial.println("Erasing all");
    flash.eraseAll();
}

void callback_offload(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid,
                      BaseFlag* dependency) {
    if (dependency != nullptr && dependency->isSet()) {
        Serial.println("Dependency set!");
        // do something
    } else {
        Serial.println("Dependency not set!");
    }
}

void callback_testfire(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid,
                       BaseFlag* dependency) {
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

void callback_mainAltitude(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid,
                           BaseFlag* dependency) {
    printf("Group UID: %u, Flag UID: %u\n", group_uid, flag_uid);

    if (length >= sizeof(float)) {
        // Extract the float value from the data buffer
        float value = 0.0f;
        memcpy(&value, data, sizeof(float));

        Serial.println("Received float value: ");
        Serial.print(value);

        ConfigurationData<float>* configMainAltitude = configuration.getConfigurable<MAIN_ELEVATION>();
        configMainAltitude->set(value);
    } else {
        Serial.println("Error: Insufficient data for float type\n");
    }
}

void callback_drogueDelay(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid,
                          BaseFlag* dependency) {
    if (length >= sizeof(float)) {
        // Extract the float value from the data buffer
        float value = 0.0f;
        memcpy(&value, data, sizeof(float));

        Serial.print("Received float value: ");
        Serial.println(value);

        ConfigurationData<float>* configDrogueDelay = configuration.getConfigurable<DROGUE_DELAY>();
        configDrogueDelay->set(value);
    } else {
        Serial.println("Error: Insufficient data for float type");
    }
}

// Define flags //
SimpleFlag erase("--erase", "Send start", true, DEFAULT_FLAT_UID, callback_erase);
SimpleFlag offload("--offload", "Send start", true, DEFAULT_FLAT_UID, callback_offload);
SimpleFlag offload_binary("-b", "binary", false, DEFAULT_FLAT_UID, callback_none);
ArgumentFlag<int> testfire("--testfire", "Send start", true, DEFAULT_FLAT_UID, callback_testfire);
ArgumentFlag<float> mainAltitudeFlag("--mainAltitude", "Send start", true, DEFAULT_FLAT_UID, callback_mainAltitude);
ArgumentFlag<float> drogueDelayFlag("--drogueDelay", "Send start", true, DEFAULT_FLAT_UID, callback_drogueDelay);

BaseFlag* eraseGroup[] = {&erase};
BaseFlag* offloadGroup[] = {&offload, &offload_binary};
BaseFlag* testfireGroup[] = {&testfire};
BaseFlag* mainAltitudeGroup[] = {&mainAltitudeFlag};
BaseFlag* drogueDelayGroup[] = {&drogueDelayFlag};


void setup() {
    // Make all SPI devices play nice by deactivating them all
    pinMode(FLASH_CS_PIN, OUTPUT);
    digitalWrite(FLASH_CS_PIN, HIGH);
    pinMode(FRAM_CS_PIN, OUTPUT);
    digitalWrite(FRAM_CS_PIN, HIGH);

    Serial.begin(9600);
    // while (!Serial);
    Serial.println("Starting");
    // System
    hardware.setLoopRate(100);
    hardware.setDebugStream(&serialDebug);
    hardware.setSystemClock(&arduinoClock);
    hardware.setConfigurationMemory(&fram);
    hardware.setConfiguration(&configuration);
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

    stateDeterminer.setup(&configuration);
    altitudeKf.calculateDiscreteTimeA(.01);

    // setup dependency
    offload.setDependency(&offload_binary);

    cliParser.addFlagGroup(eraseGroup);
    cliParser.addFlagGroup(offloadGroup);
    cliParser.addFlagGroup(testfireGroup);
    cliParser.addFlagGroup(mainAltitudeGroup);
    cliParser.addFlagGroup(drogueDelayGroup);

    drogueDelay = configuration.getConfigurable<DROGUE_DELAY>();
    mainElevation = configuration.getConfigurable<MAIN_ELEVATION>();
}

void runIndicators() {
    for (int i = 0; i < hardware.getNumIndicators(); i++) {
        if ((hardware.getLoopTimestampMs() / 250) % 2 == 0) {
            hardware.getIndicator(i)->on();
        } else {
            hardware.getIndicator(i)->off();
        }
    }
}

void printDebug() {
    Serial.print(altitudeKf.getAltitude());
    Serial.print('\t');
    Serial.print(hardware.getLastTickDuration());
    Serial.print('\t');
    Serial.print(batteryVoltageSensor.getVoltage());
    Serial.print('\t');
    Serial.println(barometer.getAltitudeM());
}

void loop() {
    hardware.enforceLoopTime();
    hardware.readSensors();

    altitudeKf.predict(hardware.getLoopTimestampMs() / 1000.0);
    altitudeKf.dataUpdate(barometer.getAltitudeM(), icm20602.getAccelerometer()->getAccelerationsMSS().z);
    constexpr Pose_s pose{}; // This will be provided from the filter

    const State_e state = stateDeterminer.loopOnce(pose);

    switch (state) {
    case DESCENT:
        if (pose.timestamp_ms - stateDeterminer.getStateStartTime() > drogueDelay->get()) {
            droguePyro.fire();
        }

        if (pose.position.z < mainElevation->get()) {
            mainPyro.fire();
        }
    case POST_FLIGHT:
        droguePyro.disable();
        mainPyro.disable();
    default: ;
    }

    runIndicators();
    printDebug();

    configuration.pushUpdatesToMemory();
}
