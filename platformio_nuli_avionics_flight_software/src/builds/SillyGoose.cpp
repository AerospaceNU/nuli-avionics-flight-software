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
#include "core/cli/SimpleFlag.h"
#include "core/cli/ArgumentFlag.h"
#include "core/cli/Parser.h"
#include "core/StateDeterminer.h"
#include "core/filters/PoseEstimator.h"
#include "core/Logger.h"

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

HardwareAbstraction hardware;
StateDeterminer stateDeterminer;
PoseEstimator poseEstimator;
Parser cliParser;

ConfigurationID_e sillyGooseRequiredConfigs[] = {DROGUE_DELAY, MAIN_ELEVATION, BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR};
Configuration configuration({
        sillyGooseRequiredConfigs,
        Configuration::REQUIRED_CONFIGS,
        StateDeterminer::REQUIRED_CONFIGS
    });
ConfigurationData<float> mainElevation;
ConfigurationData<uint32_t> drogueDelay;

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


void callback_config(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid, BaseFlag* dependency);

// Define flags //
SimpleFlag erase("--erase", "Send start", true, DEFAULT_FLAT_UID, callback_erase);
SimpleFlag offload("--offload", "Send start", true, DEFAULT_FLAT_UID, callback_offload);
SimpleFlag offload_binary("-b", "binary", false, DEFAULT_FLAT_UID, callback_none);
ArgumentFlag<int> testfire("--testfire", "Send start", true, DEFAULT_FLAT_UID, callback_testfire);
SimpleFlag configFlag("--config", "Send start", true, DEFAULT_FLAT_UID, callback_config);
ArgumentFlag<float> mainAltitudeFlagNew("-m", "Send start", false, DEFAULT_FLAT_UID, callback_none);
ArgumentFlag<float> drogueDelayFlagNew("-d", "Send start", false, DEFAULT_FLAT_UID, callback_none);

void callback_config(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid, BaseFlag* dependency) {
    if (mainAltitudeFlagNew.isSet()) {
        mainElevation.set(mainAltitudeFlagNew.getValueDerived());
        Serial.print("MAIN_ELEVATION has been set to: ");
        Serial.print(mainElevation.get());
        Serial.println(" m");

    } else {
        Serial.print("MAIN_ELEVATION is: ");
        Serial.print(mainElevation.get());
        Serial.println(" m");

    }

    if (drogueDelayFlagNew.isSet()) {
        const float timeMs = drogueDelayFlagNew.getValueDerived() * 1000;
        drogueDelay.set(uint32_t(timeMs));
        Serial.print("DROGUE_DELAY has been set to: ");
        Serial.print(drogueDelay.get());
        Serial.println(" ms");
    } else {
        Serial.print("DROGUE_DELAY is: ");
        Serial.print(drogueDelay.get());
        Serial.println(" ms");
    }
}

BaseFlag* eraseGroup[] = {&erase};
BaseFlag* offloadGroup[] = {&offload, &offload_binary};
BaseFlag* testfireGroup[] = {&testfire};
BaseFlag* configGroup[] = {&configFlag, &drogueDelayFlagNew, &mainAltitudeFlagNew};

void runIndicators() {
    for (int i = 0; i < hardware.getNumIndicators(); i++) {
        if ((hardware.getLoopTimestampMs() / 200) % 2 == 0) {
            hardware.getIndicator(i)->on();
        } else {
            hardware.getIndicator(i)->off();
        }
    }
}

char serialRead[500] = "";
int32_t serialReadIndex = 0;

void runCli() {
    while (Serial.available() > 0) {
        const char c = Serial.read();
        serialRead[serialReadIndex++] = c;
        Serial.print(c);
        if (c == '\n') {
            serialRead[serialReadIndex] = '\0';
            const int errorCode = cliParser.parse(serialRead);
            if (errorCode == 0) {
                cliParser.runFlags();
                cliParser.resetFlags();
            } else {
                Serial.print("Invalid message: ");
                Serial.print(errorCode);
                Serial.print(", ");
                Serial.println(serialRead);
            }
            serialReadIndex = 0;
        }
    }
}

void setup() {
    Serial.begin(9600);
    while (!Serial) {}

    // Make all SPI devices play nice by deactivating them all TODO: is there a nice abstraction that would work here?
    pinMode(FLASH_CS_PIN, OUTPUT);
    digitalWrite(FLASH_CS_PIN, HIGH);
    pinMode(FRAM_CS_PIN, OUTPUT);
    digitalWrite(FRAM_CS_PIN, HIGH);

    Serial.begin(9600);
    // while (!Serial);
    Serial.println("Starting");
    Serial.println(getConfigurationName(CONFIGURATION_VERSION));
    Serial.println(getConfigurationID("CONFIGURATION_VERSION"));

    // System
    hardware.setLoopRateHz(100);
    hardware.setDebugStream(&serialDebug);
    hardware.setSystemClock(&arduinoClock);
    // Devices
    const int16_t framID = hardware.appendFramMemory(&fram);
    hardware.appendFlashMemory(&flash);
    hardware.appendPyro(&droguePyro);
    hardware.appendPyro(&mainPyro);
    hardware.appendVoltageSensor(&batteryVoltageSensor);
    hardware.appendBarometer(&barometer);
    hardware.appendGenericSensor(&icm20602);
    hardware.appendAccelerometer(icm20602.getAccelerometer());
    hardware.appendGyroscope(icm20602.getGyroscope());
    hardware.appendIndicator(&led);
    hardware.appendIndicator(&buzzer);
    // Setup components
    hardware.setup();
    configuration.setup(&hardware, framID);
    poseEstimator.setup(&hardware, &configuration);
    stateDeterminer.setup(&configuration);

    // setup dependency
    offload.setDependency(&offload_binary);
    cliParser.addFlagGroup(eraseGroup);
    cliParser.addFlagGroup(offloadGroup);
    cliParser.addFlagGroup(testfireGroup);
    cliParser.addFlagGroup(configGroup);

    // Locally used configuration variables
    drogueDelay = configuration.getConfigurable<DROGUE_DELAY>();
    mainElevation = configuration.getConfigurable<MAIN_ELEVATION>();
    batteryVoltageSensor.setScaleFactor(configuration.getConfigurable<BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR>().get());

    led.setOutputPercent(6.0);
}

void loop() {
    hardware.enforceLoopTime();
    hardware.readSensors();

    const Pose_s pose = poseEstimator.loopOnce();
    const State_e state = stateDeterminer.loopOnce(pose);

    switch (state) {
    case PRE_FLIGHT:
        runCli();
    case DESCENT: // @todo turn off pyros after some time
        if (pose.timestamp_ms - stateDeterminer.getStateStartTime() > drogueDelay.get()) {
            droguePyro.fire();
        }

        if (pose.position.z < mainElevation.get()) {
            mainPyro.fire();
        }
    case POST_FLIGHT:
        runCli();
        droguePyro.disable();
        mainPyro.disable();
    default: ;
    }

    runIndicators();
    configuration.pushUpdatesToMemory();
}
