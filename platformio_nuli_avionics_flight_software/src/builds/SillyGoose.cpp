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
#include "core/FlightStateDeterminer.h"
#include "core/filters/StateEstimator1D.h"
#include "core/BasicLogger.h"
#include "util/StringHelper.h"

#define DEFAULT_FLAT_UID 255

// clang-format off
struct SillyGooseLogData {
    uint32_t timestampMs;
    uint32_t dtMS;
    float pressurePa;
    float barometerTemperatureK;
    float accelerationMSS_x;
    float accelerationMSS_y;
    float accelerationMSS_z;
    float velocityRadS_x;
    float velocityRadS_y;
    float velocityRadS_z;
    float imuTemperatureK;
    float batterVoltageV;
    float altitudeM;
    float velocityMS;
    float accelerationMSS;
    float unfilteredAltitudeM;
    int32_t flightState;
    bool drogueContinuity;
    bool drogueFired;
    bool mainContinuity;
    bool mainFired;
} remove_struct_padding;
#define LOG_HEADER "timestampMs\tdtMS\tpressurePa\tbarometerTemperatureK\taccelerationMSS_x\taccelerationMSS_y\taccelerationMSS_z\tvelocityRadS_x\tvelocityRadS_y\tvelocityRadS_z\timuTemperatureK\tbatterVoltageV\taltitudeM\tvelocityMS\taccelerationMSS\tunfilteredAltitudeM\tflightState\tdrogueContinuity\tdrogueFired\tmainContinuity\tmainFired"
void printLog(const SillyGooseLogData &d) { char buf[256]; mini_snprintf(buf,sizeof(buf),"%lu\t%lu\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%d\t%d\t%d\t%d\t%d\n",d.timestampMs,d.dtMS,d.pressurePa,d.barometerTemperatureK,d.accelerationMSS_x,d.accelerationMSS_y,d.accelerationMSS_z,d.velocityRadS_x,d.velocityRadS_y,d.velocityRadS_z,d.imuTemperatureK,d.batterVoltageV,d.altitudeM,d.velocityMS,d.accelerationMSS,d.unfilteredAltitudeM,d.flightState,d.drogueContinuity?1:0,d.drogueFired?1:0,d.mainContinuity?1:0,d.mainFired?1:0); Serial.print(buf); }
// clang-format on


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
FlightStateDeterminer flightStateDeterminer;
StateEstimator1D stateEstimator1D;
Parser cliParser;
BasicLogger<SillyGooseLogData> logger;

ConfigurationID_e sillyGooseRequiredConfigs[] = {DROGUE_DELAY, MAIN_ELEVATION, BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR};
Configuration configuration({
        sillyGooseRequiredConfigs,
        Configuration::REQUIRED_CONFIGS,
        FlightStateDeterminer::REQUIRED_CONFIGS
    });
ConfigurationData<float> mainElevation;
ConfigurationData<uint32_t> drogueDelay;

bool enableLogging = false;
bool enableStreaming = false;

void runCli();
void runIndicators();

void callback_none(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid, BaseFlag* dependency) {}
void callback_erase(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid, BaseFlag* dependency);
void callback_offload(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid, BaseFlag* dependency);
void callback_testfire(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid, BaseFlag* dependency);
void callback_config(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid, BaseFlag* dependency);
void callback_log(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid, BaseFlag* dependency);
void callback_stream(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid, BaseFlag* dependency);

// Define flags //
SimpleFlag erase("--erase", "Send start", true, DEFAULT_FLAT_UID, callback_erase);
SimpleFlag offload("--offload", "Send start", true, DEFAULT_FLAT_UID, callback_offload);
SimpleFlag offload_binary("-b", "binary", false, DEFAULT_FLAT_UID, callback_none);
SimpleFlag testfire("--fire", "Send start", true, DEFAULT_FLAT_UID, callback_testfire);
SimpleFlag testDrogue("-d", "Send start", false, DEFAULT_FLAT_UID, callback_none);
SimpleFlag testMain("-m", "Send start", false, DEFAULT_FLAT_UID, callback_none);
SimpleFlag configFlag("--config", "Send start", true, DEFAULT_FLAT_UID, callback_config);
ArgumentFlag<float> mainAltitudeFlagNew("-m", "Send start", false, DEFAULT_FLAT_UID, callback_none);
ArgumentFlag<float> drogueDelayFlagNew("-d", "Send start", false, DEFAULT_FLAT_UID, callback_none);
ArgumentFlag<int32_t> logFlag("--log", "Send start", false, DEFAULT_FLAT_UID, callback_log);
ArgumentFlag<int32_t> streamFlag("--stream", "Send start", false, DEFAULT_FLAT_UID, callback_stream);

BaseFlag* eraseGroup[] = {&erase};
BaseFlag* offloadGroup[] = {&offload, &offload_binary};
BaseFlag* testfireGroup[] = {&testfire, &testDrogue, &testMain};
BaseFlag* configGroup[] = {&configFlag, &drogueDelayFlagNew, &mainAltitudeFlagNew};
BaseFlag* logGroup[] = {&logFlag};
BaseFlag* streamGroup[] = {&streamFlag};


void setup() {
    Serial.begin(9600);
    while (!Serial) {}

    // Make all SPI devices play nice by deactivating them all TODO: is there a nice abstraction that would work here?
    pinMode(FLASH_CS_PIN, OUTPUT);
    digitalWrite(FLASH_CS_PIN, HIGH);
    pinMode(FRAM_CS_PIN, OUTPUT);
    digitalWrite(FRAM_CS_PIN, HIGH);

    // System
    hardware.setLoopRateHz(100);
    hardware.setDebugStream(&serialDebug);
    hardware.setSystemClock(&arduinoClock);
    // Devices
    const int16_t framID = hardware.appendFramMemory(&fram);
    const int16_t flashID = hardware.appendFlashMemory(&flash);
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
    stateEstimator1D.setup(&hardware, &configuration);
    flightStateDeterminer.setup(&configuration);
    logger.setup(&hardware, &cliParser, flashID, LOG_HEADER, printLog);

    // setup dependency
    offload.setDependency(&offload_binary);
    cliParser.addFlagGroup(eraseGroup);
    cliParser.addFlagGroup(offloadGroup);
    cliParser.addFlagGroup(testfireGroup);
    cliParser.addFlagGroup(configGroup);
    cliParser.addFlagGroup(logGroup);
    cliParser.addFlagGroup(streamGroup);

    // Locally used configuration variables
    drogueDelay = configuration.getConfigurable<DROGUE_DELAY>();
    mainElevation = configuration.getConfigurable<MAIN_ELEVATION>();
    // batteryVoltageSensor.setScaleFactor(configuration.getConfigurable<BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR>().get());

    led.setOutputPercent(6.0);
}

// @todo Make sure that on boot stuff is populated correctly (ground elevation, etc), particularly for launch detection

void loop() {
    // Run core hardware
    hardware.enforceLoopTime();
    hardware.readSensors();

    // Determine state
    RocketState_s state{};
    state.timestamp = hardware.getTimestamp();
    state.state1D = stateEstimator1D.loopOnce(state.timestamp);
    state.flightState = flightStateDeterminer.loopOnce(state.state1D, state.timestamp);
    state.state6D = State6D_s(); // Not implemented
    state.rawGps = Coordinates_s(); // Not implemented

    // State machine to determine when to do what
    if (state.flightState == PRE_FLIGHT) {
        runCli();
    } else if (state.flightState == ASCENT) {
        if (!enableLogging) {
            enableLogging = true;
            logger.newFlight();
        }
    } else if (state.flightState == DESCENT) {
        // @todo turn off pyros after some time
        if (state.timestamp.runtime_ms - flightStateDeterminer.getStateStartTime() > drogueDelay.get()) {
            droguePyro.fire();
        }

        if (state.state1D.altitudeM < mainElevation.get()) {
            mainPyro.fire();
        }
    } else if (state.flightState == POST_FLIGHT) {
        enableLogging = false;
        runCli();
        droguePyro.disable();
        mainPyro.disable();
    }

    // Handle outputs: indicators, pyros, logging, config, etc
    runIndicators();
    configuration.pushUpdatesToMemory();

    // Run logging
    SillyGooseLogData logData{};
    // Populate data
    logData.timestampMs = state.timestamp.runtime_ms;
    logData.dtMS = state.timestamp.dt_ms;
    logData.pressurePa = hardware.getBarometer(0)->getPressurePa();
    logData.barometerTemperatureK = hardware.getBarometer(0)->getTemperatureK();
    logData.accelerationMSS_x = icm20602.getAccelerometer()->getAccelerationsMSS().x;
    logData.accelerationMSS_y = icm20602.getAccelerometer()->getAccelerationsMSS().y;
    logData.accelerationMSS_z = icm20602.getAccelerometer()->getAccelerationsMSS().z;
    logData.velocityRadS_x = icm20602.getGyroscope()->getVelocitiesRadS().x;
    logData.velocityRadS_y = icm20602.getGyroscope()->getVelocitiesRadS().y;
    logData.velocityRadS_z = icm20602.getGyroscope()->getVelocitiesRadS().z;
    logData.imuTemperatureK = icm20602.getGyroscope()->getTemperatureK();
    logData.batterVoltageV = batteryVoltageSensor.getVoltage();
    logData.altitudeM = state.state1D.altitudeM;
    logData.velocityMS = state.state1D.velocityMS;
    logData.accelerationMSS = state.state1D.accelerationMSS;
    logData.unfilteredAltitudeM = state.state1D.unfilteredNoOffsetAltitudeM;
    logData.flightState = state.flightState;
    logData.drogueContinuity = droguePyro.hasContinuity();
    logData.drogueFired = droguePyro.isFired();
    logData.mainContinuity = mainPyro.hasContinuity();
    logData.mainFired = mainPyro.isFired();
    if (enableLogging) {
        logger.log(logData);
    }
    if (enableStreaming) {
        printLog(logData);
    }
}



////////////////////////////////////////////
////////////////////////////////////////////

void runCli() {
    static char serialRead[500] = "";
    static int32_t serialReadIndex = 0;
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

void runIndicators() {
    for (int i = 0; i < hardware.getNumIndicators(); i++) {
        if ((hardware.getTimestamp().runtime_ms / 200) % 2 == 0) {
            hardware.getIndicator(i)->on();
        } else {
            hardware.getIndicator(i)->off();
        }
    }
}

void callback_testfire(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid, BaseFlag* dependency) {
    if (testDrogue.isSet()) {
        Serial.println("Firing drogue");
        droguePyro.fire();
        delay(100);
        droguePyro.disable();
    } else if (testMain.isSet()) {
        Serial.println("Firing main");
        mainPyro.fire();
        delay(100);
        mainPyro.disable();
    }
}

void callback_stream(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid, BaseFlag* dependency) {
    if (streamFlag.getValueDerived() > 0) {
        enableStreaming = true;
        Serial.println("Streaming enabled");
    } else {
        enableStreaming = false;
        Serial.println("Streaming disabled");
    }
}

void callback_log(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid, BaseFlag* dependency) {
    Serial.print("Entry's in log ");
    Serial.println(logger.getEntryNumber());
    Serial.print("Remaining log length (s): ");
    Serial.println(logger.getRemainingLogLengthSeconds());
    if (logFlag.getValueDerived() == 2) {
        enableLogging = true;
        Serial.println("Logging enabled");
    } else if (logFlag.getValueDerived() == 1) {
        enableLogging = false;
        Serial.println("Logging disabled");
    } else {
        Serial.println(enableLogging ? "Logging is enabled" : "Logging is disabled");
    }
}


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

void callback_offload(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid, BaseFlag* dependency) {
    uint32_t failCount = 0;
    Serial.println(LOG_HEADER);
    for (uint32_t i = 0; true; i++) {
        uint8_t id;
        const SillyGooseLogData logData = logger.offload(i, id);
        if (id == 0xFF) {
            failCount++;
            if (failCount >= 4) {
                break;
            }
        } else if (id == 0x01) {
            printLog(logData);
        } else if (id == 0x02) {
            Serial.println("New flight");
        }
    }
}

void callback_erase(const char* name, uint8_t* data, uint32_t length, uint8_t group_uid, uint8_t flag_uid, BaseFlag* dependency) {
    Serial.println("Erasing all");
    logger.erase();
    Serial.println("Done");
}