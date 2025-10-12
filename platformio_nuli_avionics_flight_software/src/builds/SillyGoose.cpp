#include "Avionics.h"
#include <Arduino.h>
#include "pinmaps/SillyGoosePinmap.h"
#include "util/StringHelper.h"
#include "util/Debounce.h"
#include "drivers/arduino/ArduinoAvionicsHelper.h"
#include "drivers/arduino/SerialDebug.h"
#include "drivers/arduino/ArduinoSystemClock.h"
#include "drivers/arduino/MS5607Sensor.h"
#include "drivers/arduino/ICM20602Sensor.h"
#include "drivers/arduino/S25FL512.h"
#include "drivers/arduino/ArduinoPyro.h"
#include "drivers/arduino/ArduinoFram.h"
#include "drivers/arduino/ArduinoVoltageSensor.h"
#include "drivers/arduino/IndicatorLED.h"
#include "drivers/arduino/ArduinoSerialReader.h"
#include "drivers/arduino/IndicatorBuzzer.h"
#include "drivers/arduino/ArduinoSimulationParser.h"
#include "core/HardwareAbstraction.h"
#include "core/Configuration.h"
#include "core/cli/SimpleFlag.h"
#include "core/cli/ArgumentFlag.h"
#include "core/cli/IntegratedParser.h"
#include "core/ConfigurationCliBinding.h"
#include "core/FlightStateDeterminer.h"
#include "core/IndicatorManager.h"
#include "core/filters/StateEstimator1D.h"
#include "core/BasicLogger.h"

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
void printLog(const SillyGooseLogData &d) { char buf[256]; mini_snprintf(buf,sizeof(buf),"%lu\t%lu\t%.6f\t%.2f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%d\t%d\t%d\t%d\t%d\n",d.timestampMs,d.dtMS,d.pressurePa,d.barometerTemperatureK,d.accelerationMSS_x,d.accelerationMSS_y,d.accelerationMSS_z,d.velocityRadS_x,d.velocityRadS_y,d.velocityRadS_z,d.imuTemperatureK,d.batterVoltageV,d.altitudeM,d.velocityMS,d.accelerationMSS,d.unfilteredAltitudeM,d.flightState,d.drogueContinuity?1:0,d.drogueFired?1:0,d.mainContinuity?1:0,d.mainFired?1:0); Serial.print(buf); }
// clang-format on

// Hardware
ArduinoSystemClock arduinoClock;
SerialDebug serialDebug(true);
MS5607Sensor barometer;
ICM20602Sensor icm20602;
ArduinoPyro droguePyro(PYRO1_GATE_PIN, PYRO1_SENSE_PIN, PYRO_SENSE_THRESHOLD);
ArduinoPyro mainPyro(PYRO2_GATE_PIN, PYRO2_SENSE_PIN, PYRO_SENSE_THRESHOLD);
ArduinoVoltageSensor batteryVoltageSensor(VOLTAGE_SENSE_PIN, VOLTAGE_SENSE_SCALE);
S25FL512 flash(FLASH_CS_PIN);
ArduinoFram fram(FRAM_CS_PIN);
IndicatorLED led(LIGHT_PIN);
IndicatorBuzzer buzzer(BUZZER_PIN, 4000, 1000);
// Core components
HardwareAbstraction hardware;
FlightStateDeterminer flightStateDeterminer;
StateEstimator1D stateEstimator1D;
BasicLogger<SillyGooseLogData> logger;
ArduinoSerialReader<500> serialReader(true);
IntegratedParser cliParser;
ConfigurationID_t sillyGooseRequiredConfigs[] = {DROGUE_DELAY_c, MAIN_ELEVATION_c, BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c, PYRO_FIRE_DURATION_c};
Configuration configuration({sillyGooseRequiredConfigs, Configuration::REQUIRED_CONFIGS, FlightStateDeterminer::REQUIRED_CONFIGS, StateEstimator1D::REQUIRED_CONFIGS});
ArduinoSimulationParser simulationParser;
IndicatorManager indicatorManager;
// Locally used configuration data
ConfigurationData<float> mainElevation;
ConfigurationData<uint32_t> drogueDelay;
ConfigurationData<uint32_t> pyroFireDuration;
// CLI
void fireCallback();
SimpleFlag simFlag("--sim", "Send start", true, 255, []() {
    simulationParser.activate();
    stateEstimator1D.reset();
});
SimpleFlag testfire("--fire", "Send start", true, 255, [](){});
SimpleFlag testDrogue("-d", "Send start", false, 255, []() {
    serialDebug.message("Firing drogue");
    droguePyro.fireFor(pyroFireDuration.get());
});
SimpleFlag testMain("-m", "Send start", false, 255, []() {
    serialDebug.message("Firing main");
    mainPyro.fireFor(pyroFireDuration.get());
});
BaseFlag* testfireGroup[] = {&testfire, &testDrogue, &testMain};
BaseFlag* simGroup[] = {&simFlag};
ConfigurationCliBinding<DROGUE_DELAY_c> drogueConfigurationCliBinding;
ConfigurationCliBinding<MAIN_ELEVATION_c> mainConfigurationCliBinding;
ConfigurationCliBinding<BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c> batteryVoltageConfigurationCliBinding;
ConfigurationCliBinding<GROUND_ELEVATION_c> groundElevationConfigurationCliBinding;
ConfigurationCliBinding<GROUND_TEMPERATURE_c> groundTemperatureConfigurationCliBinding;
ConfigurationCliBinding<CONFIGURATION_VERSION_c> versionConfigurationCliBinding;
ConfigurationCliBinding<PYRO_FIRE_DURATION_c> pyroFireDurationConfigurationCliBinding;

void setup() {
    // Initialize
    disableChipSelectPins({FRAM_CS_PIN, FLASH_CS_PIN}); // All CS pins must disable prior to SPI device setup on multi device buses to prevent one device from locking the bus
    configuration.setDefault<BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c>(VOLTAGE_SENSE_SCALE); // Configuration defaults MUST be called prior to configuration.setup() for it to have effect
    led.setOutputPercent(6.0); // Lower the LED Power

    // Setup Hardware
    const int16_t framID = hardware.appendFramMemory(&fram);
    const int16_t flashID = hardware.appendFlashMemory(&flash);
    const int16_t drogueID = hardware.appendPyro(&droguePyro);
    const int16_t mainID = hardware.appendPyro(&mainPyro);
    hardware.appendVoltageSensor(&batteryVoltageSensor);
    hardware.appendBarometer(&barometer);
    hardware.appendGenericSensor(&icm20602);
    hardware.appendAccelerometer(icm20602.getAccelerometer());
    hardware.appendGyroscope(icm20602.getGyroscope());
    hardware.appendIndicator(&led);
    hardware.appendIndicator(&buzzer);
    hardware.setup(&serialDebug, &arduinoClock, 100);

    // Setup components
    serialDebug.message("SETTING UP COMPONENTS");
    configuration.setup(&hardware, framID);
    stateEstimator1D.setup(&hardware, &configuration);
    flightStateDeterminer.setup(&configuration);
    indicatorManager.setup(&hardware, drogueID, mainID);
    logger.setup(&hardware, &cliParser, flashID, LOG_HEADER, printLog);
    cliParser.setup(&serialReader, &serialDebug);
    // Setup cli
    cliParser.addFlagGroup(testfireGroup);
    cliParser.addFlagGroup(simGroup);
    drogueConfigurationCliBinding.setup(&configuration, &cliParser, &serialDebug);
    mainConfigurationCliBinding.setup(&configuration, &cliParser, &serialDebug);
    batteryVoltageConfigurationCliBinding.setup(&configuration, &cliParser, &serialDebug);
    versionConfigurationCliBinding.setup(&configuration, &cliParser, &serialDebug);
    groundElevationConfigurationCliBinding.setup(&configuration, &cliParser, &serialDebug);
    groundTemperatureConfigurationCliBinding.setup(&configuration, &cliParser, &serialDebug);
    pyroFireDurationConfigurationCliBinding.setup(&configuration, &cliParser, &serialDebug);
    // Locally used configuration variables
    drogueDelay = configuration.getConfigurable<DROGUE_DELAY_c>();
    mainElevation = configuration.getConfigurable<MAIN_ELEVATION_c>();
    pyroFireDuration = configuration.getConfigurable<PYRO_FIRE_DURATION_c>();
    batteryVoltageSensor.setScaleFactor(configuration.getConfigurable<BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c>().get());
    // Done
    serialDebug.message("COMPONENTS SET UP COMPLETE\r\n");
}

void loop() {
    // Run core hardware
    RocketState_s state{};
    state.timestamp = hardware.enforceLoopTime();
    hardware.readSensors();

    if (simulationParser.isActive()) {
        if (simulationParser.blockingGetNextSimulationData()) {
            const float tempK = simulationParser.getNextFloat();
            const float pressurePa = simulationParser.getNextFloat();
            const float ax = simulationParser.getNextFloat();
            const float ay = simulationParser.getNextFloat();
            const float az = simulationParser.getNextFloat();
            barometer.inject(tempK, 0, pressurePa);
            icm20602.getAccelerometer()->inject({-az, ax, ay}, 0);
        } else {
            stateEstimator1D.reset();
        }
    }

    // Determine state
    state.state1D = stateEstimator1D.loopOnce(state.timestamp, flightStateDeterminer.getFlightState());
    state.flightState = flightStateDeterminer.loopOnce(state.timestamp, state.state1D);

    // State machine to determine when to do what
    if (state.flightState == PRE_FLIGHT) {
        static Debounce logTimer(5000);
        if (logTimer.check(true, state.timestamp)) {
            logger.logOnce();
            logTimer.reset();
        }
        indicatorManager.beepContinuity(state.timestamp);
        cliParser.runCli();
    } else if (state.flightState == ASCENT) {
        indicatorManager.keepAliveBeep(state.timestamp);
        if (!logger.isLoggingEnabled()) {
            logger.enableLogging();
            logger.newFlight();
        }
    } else if (state.flightState == DESCENT) {
        indicatorManager.keepAliveBeep(state.timestamp);
        static uint8_t deployState = 0; // Ensure each is only fired once
        if (state.timestamp.runtime_ms - flightStateDeterminer.getStateStartTime() > drogueDelay.get() && deployState == 0) {
            droguePyro.fireFor(pyroFireDuration.get());
            deployState = 1;
        }
        static Debounce mainDeployDebounce(200);
        if (mainDeployDebounce.check(state.state1D.altitudeM <= mainElevation.get(), state.timestamp) && deployState == 1) {
            mainPyro.fireFor(pyroFireDuration.get());
            deployState = 2;
        }
    } else if (state.flightState == POST_FLIGHT) {
        logger.disableLogging();
        static Debounce logTimer(5000);
        if (logTimer.check(true, state.timestamp)) {
            logger.logOnce();
            logTimer.reset();
        }
        indicatorManager.siren(state.timestamp);
        cliParser.runCli();
    } else {
        logger.enableLogging();
        indicatorManager.siren(state.timestamp);
        cliParser.runCli();
    }

    // Update any changes to the configuration
    configuration.pushUpdatesToMemory();
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
}
