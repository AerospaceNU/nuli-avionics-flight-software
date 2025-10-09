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
#include "core/ConfigurationCliBinding.h"
#include "core/FlightStateDeterminer.h"
#include "core/filters/StateEstimator1D.h"
#include "core/BasicLogger.h"
#include "util/StringHelper.h"
#include "core/SimulationParser.h"

// @todo: give all hardware access to the debug stream

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
// Core components
HardwareAbstraction hardware;
FlightStateDeterminer flightStateDeterminer;
StateEstimator1D stateEstimator1D;
Parser cliParser;
BasicLogger<SillyGooseLogData> logger;
// Configuration
ConfigurationID_t sillyGooseRequiredConfigs[] = {DROGUE_DELAY_c, MAIN_ELEVATION_c, BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c};
Configuration configuration({sillyGooseRequiredConfigs, Configuration::REQUIRED_CONFIGS, FlightStateDeterminer::REQUIRED_CONFIGS, StateEstimator1D::REQUIRED_CONFIGS});
ConfigurationData<float> mainElevation;
ConfigurationData<uint32_t> drogueDelay;

SimulationParser simulationParser;

// CLI
void runCli();
void callback_none() {}
void fireCallback();
SimpleFlag testfire("--fire", "Send start", true, 255, fireCallback);
SimpleFlag testDrogue("-d", "Send start", false, 255, callback_none);
SimpleFlag testMain("-m", "Send start", false, 255, callback_none);
BaseFlag* testfireGroup[] = {&testfire, &testDrogue, &testMain};
ConfigurationCliBinding<DROGUE_DELAY_c> drogueConfigurationCliBinding;
ConfigurationCliBinding<MAIN_ELEVATION_c> mainConfigurationCliBinding;
ConfigurationCliBinding<BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c> batteryVoltageConfigurationCliBinding;
ConfigurationCliBinding<GROUND_ELEVATION_c> groundElevationConfigurationCliBinding;
ConfigurationCliBinding<GROUND_TEMPERATURE_c> groundTemperatureConfigurationCliBinding;
ConfigurationCliBinding<CONFIGURATION_VERSION_c> versionConfigurationCliBinding;

void runIndicators(const Timestamp_s&);


void setup() {
    Serial.begin(9600);
    while (!Serial) {}

    // Make all SPI devices play nice by deactivating them all TODO: is there a nice abstraction that would work here?
    pinMode(FLASH_CS_PIN, OUTPUT);
    digitalWrite(FLASH_CS_PIN, HIGH);
    pinMode(FRAM_CS_PIN, OUTPUT);
    digitalWrite(FRAM_CS_PIN, HIGH);

    // Configuration defaults MUST be called prior to configuration.setup() for it to have effect
    configuration.setDefault<BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c>(VOLTAGE_SENSE_SCALE);

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

    // setup cli
    cliParser.addFlagGroup(testfireGroup);
    drogueConfigurationCliBinding.setup(&configuration, &cliParser, &serialDebug);
    mainConfigurationCliBinding.setup(&configuration, &cliParser, &serialDebug);
    batteryVoltageConfigurationCliBinding.setup(&configuration, &cliParser, &serialDebug);
    versionConfigurationCliBinding.setup(&configuration, &cliParser, &serialDebug);
    groundElevationConfigurationCliBinding.setup(&configuration, &cliParser, &serialDebug);
    groundTemperatureConfigurationCliBinding.setup(&configuration, &cliParser, &serialDebug);

    // Locally used configuration variables
    drogueDelay = configuration.getConfigurable<DROGUE_DELAY_c>();
    mainElevation = configuration.getConfigurable<MAIN_ELEVATION_c>();
    batteryVoltageSensor.setScaleFactor(configuration.getConfigurable<BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c>().get());

    led.setOutputPercent(6.0);

    // char buff[100] = "--streamLog -start";
    // cliParser.parse(buff);
    // cliParser.runFlags();
    // cliParser.resetFlags();

    logger.logMessage("System setup complete");
}

// @todo Make sure that on boot stuff is populated correctly (ground elevation, etc), particularly for launch detection

void loop() {
    // uint32_t start = micros();
    // simulationParser.blockingGetNextSimulationData();
    // uint32_t end = micros();
    // Serial.println(end - start);

    RocketState_s state{};

    // Run core hardware
    state.timestamp = hardware.enforceLoopTime();
    hardware.readSensors();

    // float tempK = simulationParser.getNextFloat();
    // float pressurePa = simulationParser.getNextFloat();
    // float ax = simulationParser.getNextFloat();
    // float ay = simulationParser.getNextFloat();
    // float az = simulationParser.getNextFloat();
    // barometer.inject(tempK, 0, pressurePa);
    // icm20602.getAccelerometer()->inject({-az, ax, ay}, 0);


    // Determine state
    state.state1D = stateEstimator1D.loopOnce(state.timestamp);
    state.flightState = flightStateDeterminer.loopOnce(state.state1D, state.timestamp);
    state.state6D = State6D_s(); // Not implemented
    state.rawGps = Coordinates_s(); // Not implemented

    // State machine to determine when to do what
    if (state.flightState == PRE_FLIGHT) {
        runCli();
    } else if (state.flightState == ASCENT) {
        if (!logger.isLoggingEnabled()) {
            logger.enableLogging();
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
        logger.disableLogging();
        runCli();
        droguePyro.disable();
        mainPyro.disable();
    }

    //// Handle outputs: indicators, pyros, logging, config, etc
    // Runs light and buzzer
    runIndicators(state.timestamp);
    // Run logging
    SillyGooseLogData logData{};
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
    logger.log(logData);
    configuration.pushUpdatesToMemory();
}


////////////////////////////////////////////
////////////////////////////////////////////


void runIndicators(const Timestamp_s& timestamp) {
    for (int i = 0; i < hardware.getNumIndicators(); i++) {
        if ((timestamp.runtime_ms / 200) % 2 == 0) {
            hardware.getIndicator(i)->on();
        } else {
            hardware.getIndicator(i)->off();
        }
    }
}

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

void fireCallback() {
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
