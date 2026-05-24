#include "Arduino.h"
#include "Avionics.h"
#include "pinmaps/SillyGoosePinmap.h"
#include "util/Timer.h"
#include "drivers/arduino/ArduinoAvionicsHelper.h"
#include "drivers/arduino/SerialDebug.h"
#include "drivers/arduino/ArduinoSystemClock.h"
#include "drivers/arduino/MS5607Sensor.h"
#include "drivers/arduino/ICM20602Sensor.h"
#include "drivers/arduino/ICM42605Sensor.h"
#include "drivers/arduino/MX25L256.h"
#include "drivers/arduino/S25FL512.h"
#include "drivers/arduino/ArduinoPyro.h"
#include "drivers/arduino/ArduinoFram.h"
#include "drivers/arduino/ArduinoVoltageSensor.h"
#include "drivers/arduino/IndicatorLED.h"
#include "drivers/arduino/ArduinoSerialReader.h"
#include "drivers/arduino/IndicatorBuzzer.h"
#include "drivers/arduino/ArduinoDigitalInput.h"
#include "core/HardwareAbstraction.h"
#include "core/configuration/Configuration.h"
#include "core/configuration/ConfigurationCliBinding.h"
#include "core/state_estimation/FlightStateDeterminer.h"
#include "core/IndicatorManager.h"
#include "core/BasicLogger.h"
#include "core/cli/SimpleFlag.h"
#include "core/cli/IntegratedParser.h"
#include "core/cli/SimulationParser.h"
#include "core/state_estimation/OrientationEstimator.h"
#include "core/state_estimation/StateEstimatorBasic6D.h"
#include "core/state_estimation/StateEstimator1D.h"
#include "core/transform/DiscreteRotation.h"
#include "util/StringHelper.h"

// @todo Have barometer re-init in code/figure out I2C bus lock
// @todo Disable write in flash driver
// @todo Fix low pass implementation with dt included
// @todo Fix stuck in ascent while not moving bug
// @todo Drogue deployment failure detection
// @todo Merge into orientation code, fix the hardcoded transform
// @todo Update firmware from website
// @todo Firmware version tracking
// @todo Intelligent log memory usage
// @todo Copy over new uf2 stuff
// @todo Copy over AVIONICS_ARGUMENT_boardVersion macro stuff


// @todo Offload -> simulation data pipeline, fix sim2
// @todo Make a data repo
// @todo Save fram to flash
// @todo Kalman gains for mach
// @todo Enable/disable buzzer in config


// clang-format off
struct SillyGooseLogData {
    uint32_t timestampMs;
    float pressurePa, barometerTemperatureK;
    float accelerationMSS_x, accelerationMSS_y, accelerationMSS_z, velocityRadS_x, velocityRadS_y, velocityRadS_z, imuTemperatureK;
    float batteryVoltageV, altitudeM, velocityMS, accelerationMSS, unfilteredAltitudeM;
    int32_t flightState;
    bool drogueContinuity, drogueFired, mainContinuity, mainFired;
} remove_struct_padding;
#define LOG_HEADER "timestampMs\tpressurePa\tbarometerTemperatureK\taccelerationMSS_x\taccelerationMSS_y\taccelerationMSS_z\tvelocityRadS_x\tvelocityRadS_y\tvelocityRadS_z\timuTemperatureK\tbatteryVoltageV\taltitudeM\tvelocityMS\taccelerationMSS\tunfilteredAltitudeM\tflightState\tdrogueContinuity\tdrogueFired\tmainContinuity\tmainFired"
void printLog(const SillyGooseLogData &d, DebugStream *debug) { debug->data("%lu\t%.6f\t%.2f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.6f\t%d\t%d\t%d\t%d\t%d",d.timestampMs,d.pressurePa,d.barometerTemperatureK,d.accelerationMSS_x,d.accelerationMSS_y,d.accelerationMSS_z,d.velocityRadS_x,d.velocityRadS_y,d.velocityRadS_z,d.imuTemperatureK,d.batteryVoltageV,d.altitudeM,d.velocityMS,d.accelerationMSS,d.unfilteredAltitudeM,d.flightState,d.drogueContinuity?1:0,d.drogueFired?1:0,d.mainContinuity?1:0,d.mainFired?1:0); };
void printConfig(Configuration* config, char* buf, size_t bufSize) { mini_snprintf(buf, (int)bufSize, "CONFIG\tBOARD_NAME=%s\tDROGUE_DELAY=%u\tMAIN_ELEVATION=%.2f\tBATTERY_VOLTAGE_SENSOR_SCALE_FACTOR=%.4f\tGROUND_ELEVATION=%.2f\tGROUND_TEMPERATURE=%.2f\tPYRO_FIRE_DURATION=%u\tBUZZER_ENABLED=%u\tFLIGHT_STATE=%d\tBOARD_ORIENTATION=%d\tCONFIGURATION_VERSION=%u", config->getConfigurable<BOARD_NAME_c>().get().str, (unsigned int)config->getConfigurable<DROGUE_DELAY_c>().get(), (double)config->getConfigurable<MAIN_ELEVATION_c>().get(), (double)config->getConfigurable<BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c>().get(), (double)config->getConfigurable<GROUND_ELEVATION_c>().get(), (double)config->getConfigurable<GROUND_TEMPERATURE_c>().get(), (unsigned int)config->getConfigurable<PYRO_FIRE_DURATION_c>().get(), (unsigned int)config->getConfigurable<BUZZER_ENABLED_c>().get(), (int)config->getConfigurable<FLIGHT_STATE_c>().get(), (int)config->getConfigurable<BOARD_ORIENTATION_c>().get(), (unsigned int)config->getConfigurable<CONFIGURATION_VERSION_c>().get()); }
// clang-format on

// Hardware
ArduinoSystemClock arduinoClock;
SerialDebug serialDebug(AVIONICS_ARGUMENT_isDev); // Only wait for serial connection if in dev mode
MS5607Sensor barometer;
#if AVIONICS_ARGUMENT_boardVersion == 1
const DiscreteRotation imuRotation = DiscreteRotation::identity().rotateZNeg90local().rotateX90local().inverse();
ICM20602Sensor imu(&imuRotation);
S25FL512 flash(FLASH_CS_PIN);
#elif AVIONICS_ARGUMENT_boardVersion == 2
const DiscreteRotation imuRotation = DiscreteRotation::identity().rotateZNeg90local().rotateX90local().inverse();
ICM42605Sensor imu(&imuRotation);
MX25L256 flash(FLASH_CS_PIN);
#endif
ArduinoPyro droguePyro(PYRO1_GATE_PIN, PYRO1_SENSE_PIN, PYRO_SENSE_THRESHOLD);
ArduinoPyro mainPyro(PYRO2_GATE_PIN, PYRO2_SENSE_PIN, PYRO_SENSE_THRESHOLD);
ArduinoVoltageSensor batteryVoltageSensor(VOLTAGE_SENSE_PIN, VOLTAGE_SENSE_SCALE);
ArduinoFram fram(FRAM_CS_PIN);
IndicatorLED led(LIGHT_PIN);
IndicatorBuzzer buzzer(BUZZER_PIN, 4000, 1000);
ArduinoDigitalInput powerStatus(STATUS_PIN);

// Core components
HardwareAbstraction hardware(serialDebug, arduinoClock, 100);
FlightStateDeterminer flightStateDeterminer;
StateEstimator1D stateEstimator1D;
BasicLogger<SillyGooseLogData> logger;
ArduinoSerialReader<500> serialReader(!AVIONICS_ARGUMENT_isSim);
IndicatorManager indicatorManager;
IntegratedParser cliParser;
SimulationParser<8> simulationParser;
// Configuration
ConfigurationID_t sillyGooseRequiredConfigs[] = {BOARD_NAME_c, DROGUE_DELAY_c, MAIN_ELEVATION_c, BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c, PYRO_FIRE_DURATION_c, BUZZER_ENABLED_c};
Configuration configuration({
        sillyGooseRequiredConfigs,
        Configuration::REQUIRED_CONFIGS,
        FlightStateDeterminer::REQUIRED_CONFIGS,
        StateEstimator1D::REQUIRED_CONFIGS,
        OrientationEstimator::REQUIRED_CONFIGS
    });
// Locally used configuration data
ConfigurationData<float> mainElevation;
ConfigurationData<uint32_t> drogueDelay;
ConfigurationData<uint32_t> pyroFireDuration;
// CLI -> configuration bindings. Generates a CLI command to get/set configuration value.
ConfigurationCliBindings<DROGUE_DELAY_c,
                         MAIN_ELEVATION_c,
                         BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c,
                         GROUND_ELEVATION_c,
                         GROUND_TEMPERATURE_c,
                         PYRO_FIRE_DURATION_c,
                         BOARD_NAME_c,
                         BUZZER_ENABLED_c,
                         CONFIGURATION_VERSION_c> configurationCliBindings;
// CLI
SimpleFlag resetBoard("--reset", "Send start", true, 255, []() { NVIC_SystemReset(); });
SimpleFlag testfire("--fire", "Send start", true, 255, []() {});
SimpleFlag testDrogue("-d", "Send start", false, 255, []() {
    serialDebug.message("Firing drogue");
    droguePyro.fireFor(pyroFireDuration.get());
});
SimpleFlag testMain("-m", "Send start", false, 255, []() {
    serialDebug.message("Firing main");
    mainPyro.fireFor(pyroFireDuration.get());
});
BaseFlag* testfireGroup[] = {&testfire, &testDrogue, &testMain};
BaseFlag* resetBoardGroup[] = {&resetBoard};

void setup() {
    // Initialize
    disableChipSelectPins({FRAM_CS_PIN, FLASH_CS_PIN}); // All CS pins must disable prior to SPI device setup on multi device buses to prevent one device from locking the bus
    configuration.setDefault<BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c>(VOLTAGE_SENSE_SCALE); // Configuration defaults MUST be called prior to configuration.setup() for it to have effect
    configuration.setDefault<BOARD_NAME_c>(SILLY_GOOSE_NAME);
    if (AVIONICS_ARGUMENT_isDev) led.setOutputPercent(6.0); // Lower the LED Power

    // Setup Hardware
    int16_t framID = hardware.appendFramMemory(&fram);
    int16_t flashID = hardware.appendFlashMemory(&flash);
    int16_t drogueID = hardware.appendPyro(&droguePyro);
    int16_t mainID = hardware.appendPyro(&mainPyro);
    hardware.appendVoltageSensor(&batteryVoltageSensor);
    hardware.appendBarometer(&barometer);
    hardware.appendGenericHardware(&imu);
    hardware.appendAccelerometer(imu.getAccelerometer());
    hardware.appendGyroscope(imu.getGyroscope());
    hardware.appendIndicator(&led);
    hardware.appendIndicator(&buzzer);
    hardware.appendDigitalInput(&powerStatus);
    hardware.setup();

    // Setup components
    serialDebug.message("SETTING UP COMPONENTS");
    configuration.setup(&hardware, framID); // Must be called first, for everything else to be able to use the configuration
    configurationCliBindings.setupAll(&configuration, &cliParser, &serialDebug);
    cliParser.addFlagGroup(testfireGroup);
    cliParser.addFlagGroup(resetBoardGroup);
    cliParser.setup(&serialReader, &serialDebug);
    simulationParser.setup(&cliParser, &serialDebug);
    stateEstimator1D.setup(&hardware, &configuration);
    flightStateDeterminer.setup(&configuration);
    indicatorManager.setup(&hardware, drogueID, mainID);
    logger.setup(&hardware, &cliParser, flashID, LOG_HEADER, printLog, &configuration, printConfig);
    // Locally used configuration variables
    drogueDelay = configuration.getConfigurable<DROGUE_DELAY_c>();
    mainElevation = configuration.getConfigurable<MAIN_ELEVATION_c>();
    pyroFireDuration = configuration.getConfigurable<PYRO_FIRE_DURATION_c>();
    batteryVoltageSensor.setScaleFactor(configuration.getConfigurable<BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c>().get());
    serialDebug.message("COMPONENTS SET UP COMPLETE\r\n");
}

void loop() {
    // Run core hardware
    RocketState_s state{};
    state.timestamp = hardware.enforceLoopTime();
    hardware.runAndReadAllHardware(); // Reads sensors, runs any background code for every hardware device

    // Read in sim data. This should be optimized out by the compiler in the final deployment
    if (AVIONICS_ARGUMENT_isSim) {
        simulationParser.waitForEntry();
        // inject(temperatureK, humidityPercent, pressurePa) — python sends pressure first, temp second
        barometer.inject(simulationParser.getValue(1), 0, simulationParser.getValue(0));
        imu.getAccelerometer()->inject({simulationParser.getValue(2), simulationParser.getValue(3), simulationParser.getValue(4)}, 0);
        imu.getGyroscope()->inject({simulationParser.getValue(5), simulationParser.getValue(6), simulationParser.getValue(7)}, 0);
        simulationParser.releaseEntry();
    }

    // Determine state
    state.state1D = stateEstimator1D.update(state.timestamp, flightStateDeterminer.getFlightState());
    state.flightState = flightStateDeterminer.update(state.timestamp, state.state1D);

    // Turn on/off the buzzer
    buzzer.setEnabled(configuration.getConfigurable<BUZZER_ENABLED_c>().get());

    // State machine to determine when to do what
    if (state.flightState == PRE_FLIGHT) {
        // Disable logging when transition into PRE_FLIGHT, but allow for continues logging to manually be enabled through the cli
        if (flightStateDeterminer.isStateTransitionTick()) logger.disableContinuousLogging();
        // Set default log rate
        logger.setLogDelay(5000);
        cliParser.runCli();
        indicatorManager.beepContinuity(state.timestamp);
    } else if (state.flightState == ASCENT) {
        logger.enableContinuousLogging();
        indicatorManager.keepAliveBeep(state.timestamp);
    } else if (state.flightState == DESCENT) {
        logger.enableContinuousLogging();
        indicatorManager.keepAliveBeep(state.timestamp);
        // Fire both pyros at the appropriate conditions
        static uint8_t deployState = 0; // Ensure each is only fired once
        if (flightStateDeterminer.getStateTimer()->getTimeElapsed(state.timestamp.runtime_ms) > drogueDelay.get() && deployState == 0) {
            droguePyro.fireFor(pyroFireDuration.get());
            deployState = 1;
        }
        static Debounce mainDeployDebounce(200);
        if (mainDeployDebounce.check(state.state1D.altitudeM <= mainElevation.get(), state.timestamp.runtime_ms) && deployState == 1) {
            mainPyro.fireFor(pyroFireDuration.get());
            deployState = 2;
        }
    } else if (state.flightState == POST_FLIGHT) {
        logger.disableContinuousLogging();
        logger.setLogDelay(5000);
        cliParser.runCli();
        indicatorManager.siren(state.timestamp);
    } else {
        logger.enableContinuousLogging();
        cliParser.runCli();
        indicatorManager.siren(state.timestamp);
    }

    // Update any changes to the configuration
    configuration.pushUpdatesToMemory();
    // Run logging
    logger.log({
            state.timestamp.runtime_ms, barometer.getPressurePa(), barometer.getTemperatureK(),
            imu.getAccelerometer()->getAccelerationsMSS_sensor().x, imu.getAccelerometer()->getAccelerationsMSS_sensor().y, imu.getAccelerometer()->getAccelerationsMSS_sensor().z,
            imu.getGyroscope()->getVelocitiesRadS_raw().x, imu.getGyroscope()->getVelocitiesRadS_raw().y, imu.getGyroscope()->getVelocitiesRadS_raw().z,
            imu.getGyroscope()->getTemperatureK(),
            batteryVoltageSensor.getVoltage(), state.state1D.altitudeM, state.state1D.velocityMS, state.state1D.accelerationMSS, state.state1D.unfilteredNoOffsetAltitudeM, state.flightState,
            droguePyro.hasContinuity(), droguePyro.isFired(), mainPyro.hasContinuity(), mainPyro.isFired()
        });
}
