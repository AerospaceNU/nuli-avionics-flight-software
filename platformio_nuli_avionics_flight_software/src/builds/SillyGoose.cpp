#include "Arduino.h"
#include "Avionics.h"
#include "pinmaps/SillyGoosePinmap.h"
#include "util/Timer.h"
#include "drivers/arduino/ArduinoAvionicsHelper.h"
#include "drivers/arduino/SerialDebug.h"
#include "drivers/arduino/ArduinoSystemClock.h"
#include "drivers/arduino/Ms5607Mmc5603SensorPackage.h"
#include "drivers/arduino/MX25L256.h"
#include "drivers/arduino/S25FL512.h"
#include "drivers/arduino/ArduinoPyro.h"
#include "drivers/arduino/ArduinoFram.h"
#include "drivers/arduino/ArduinoVoltageSensor.h"
#include "drivers/arduino/IndicatorLED.h"
#include "drivers/arduino/IndicatorBuzzer.h"
#include "drivers/arduino/ArduinoDigitalInput.h"
#include "drivers/arduino/ArduinoWatchdog.h"
#include "core/HardwareAbstraction.h"
#include "core/configuration/Configuration.h"
#include "core/configuration/ConfigurationCliBinding.h"
#include "core/state_estimation/FlightStateDeterminer.h"
#include "core/DualDeploy.h"
#include "core/IndicatorManager.h"
#include "core/BasicLogger.h"
#include "core/BasicLoggerHelper.h"
#include "core/cli/SimpleFlag.h"
#include "core/cli/IntegratedParser.h"
#include "core/cli/SimulationParser.h"
#include "core/state_estimation/OrientationEstimator.h"
#include "core/state_estimation/StateEstimatorBasic6D.h"
#include "core/state_estimation/StateEstimator1D.h"
#include "core/transform/DiscreteRotation.h"

// clang-format off
LOG_DATA_AUTO_DEFINE(HEADER_OUTPUT,
    struct SillyGooseLogData {
        uint32_t timestampMs;
        float pressurePa, barometerTemperatureK;
        float accelerationMSS_x, accelerationMSS_y, accelerationMSS_z, velocityRadS_x, velocityRadS_y, velocityRadS_z, imuTemperatureK;
        float batteryVoltageV, altitudeM, velocityMS, accelerationMSS, unfilteredAltitudeM;
        int32_t flightState;
        bool drogueContinuity, drogueFired, mainContinuity, mainFired;
        float tiltMagnitudeDeg, angularVelRadS_x, angularVelRadS_y, angularVelRadS_z, quaternion_a, quaternion_b, quaternion_c, quaternion_d;
    } remove_struct_padding;
)
// clang-format on
static constexpr const char* LOG_HEADER = BasicLoggerHelper::generateBasicLogHeader<HEADER_OUTPUT>();
void printLog(const SillyGooseLogData &d, DebugStream *debug) {
    BasicLoggerHelper::genericBasicLogDataPrinter<HEADER_OUTPUT>(d, debug);
}

// Hardware
ArduinoSystemClock arduinoClock;
ArduinoWatchdog watchdog; // must precede serialDebug, which pets it during its dev-mode wait
SerialDebug<500> serialDebug(AVIONICS_ARGUMENT_isDev, &watchdog, !AVIONICS_ARGUMENT_isSim); // waits for a serial connection only in dev mode
const DiscreteRotation imuRotation = DiscreteRotation::identity().rotateZNeg90local().rotateX90local().inverse();
#if IS_BOARD_VERSION(1)
S25FL512 flash(FLASH_CS_PIN);
constexpr Ms5607Mmc5603SensorPackage::ImuType SILLY_GOOSE_IMU_TYPE = Ms5607Mmc5603SensorPackage::ImuType::ICM20602;
#elif IS_BOARD_VERSION(2)
MX25L256 flash(FLASH_CS_PIN);
constexpr Ms5607Mmc5603SensorPackage::ImuType SILLY_GOOSE_IMU_TYPE = Ms5607Mmc5603SensorPackage::ImuType::ICM42605;
#endif
Ms5607Mmc5603SensorPackage sensorPackage(&imuRotation, SILLY_GOOSE_IMU_TYPE, false); // no magnetometer on SillyGoose
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
DualDeploy dualDeploy;
StateEstimator1D stateEstimator1D;
OrientationEstimator orientationEstimator;
BasicLogger<SillyGooseLogData> logger;
IndicatorManager indicatorManager;
IntegratedParser cliParser;
SimulationParser<8> simulationParser;
// Configuration
ConfigurationID_t sillyGooseRequiredConfigs[] = {FIRMWARE_VERSION_c, BOARD_NAME_c, DROGUE_DELAY_c, MAIN_ELEVATION_c, BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c, PYRO_FIRE_DURATION_c, BUZZER_ENABLED_c};
Configuration configuration({
        sillyGooseRequiredConfigs,
        Configuration::REQUIRED_CONFIGS,
        FlightStateDeterminer::REQUIRED_CONFIGS,
        DualDeploy::REQUIRED_CONFIGS,
        StateEstimator1D::REQUIRED_CONFIGS,
        OrientationEstimator::REQUIRED_CONFIGS
    });
// Locally used configuration data
ConfigurationData<uint32_t> pyroFireDuration;
// CLI get/set command for each configuration value
ConfigurationCliBindings<FIRMWARE_VERSION_c,
                         DROGUE_DELAY_c,
                         MAIN_ELEVATION_c,
                         BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c,
                         GROUND_ELEVATION_c,
                         GROUND_TEMPERATURE_c,
                         PYRO_FIRE_DURATION_c,
                         BOARD_NAME_c,
                         BUZZER_ENABLED_c,
                         CONFIGURATION_VERSION_c> configurationCliBindings;
// CLI
SimpleFlag resetBoard("--reset", "Reboots the board", true, [](DebugStream*) { NVIC_SystemReset(); });
SimpleFlag testfire("--fire", "Test-fires a pyro", true, [](DebugStream*) {});
SimpleFlag testDrogue("-d", "Fires drogue", false, [](DebugStream* debugStream) {
    debugStream->message("Firing drogue");
    droguePyro.fireFor(pyroFireDuration.get());
});
SimpleFlag testMain("-m", "Fires main", false, [](DebugStream* debugStream) {
    debugStream->message("Firing main");
    mainPyro.fireFor(pyroFireDuration.get());
});
SimpleFlag helpFlag("--help", "Prints all commands", true, [](DebugStream* debugStream) { cliParser.printHelp(debugStream); });
BaseFlag* testfireGroup[] = {&testfire, &testDrogue, &testMain};
BaseFlag* resetBoardGroup[] = {&resetBoard};
BaseFlag* helpGroup[] = {&helpFlag};

void setup() {
    watchdog.disable(); // SAMD's WDT survives NVIC_SystemReset()
    watchdog.enable(4000);
    disableChipSelectPins({FRAM_CS_PIN, FLASH_CS_PIN}); // before any SPI setup, so no device holds the shared bus
    configuration.setDefault<BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c>(VOLTAGE_SENSE_SCALE); // defaults only apply if set before configuration.setup()
    configuration.setDefault<BOARD_NAME_c>(SILLY_GOOSE_NAME);
    if (AVIONICS_ARGUMENT_isDev) led.setOutputPercent(6.0f);

    // Setup Hardware
    int16_t framID = hardware.appendFramMemory(&fram);
    int16_t flashID = hardware.appendFlashMemory(&flash);
    int16_t drogueID = hardware.appendPyro(&droguePyro);
    int16_t mainID = hardware.appendPyro(&mainPyro);
    hardware.appendVoltageSensor(&batteryVoltageSensor);
    hardware.appendGenericHardware(&sensorPackage);
    hardware.appendBarometer(sensorPackage.getBarometer());
    hardware.appendAccelerometer(sensorPackage.getAccelerometer());
    hardware.appendGyroscope(sensorPackage.getGyroscope());
    hardware.appendIndicator(&led);
    hardware.appendIndicator(&buzzer);
    hardware.appendDigitalInput(&powerStatus);
    hardware.setWatchdogTimer(&watchdog);
    hardware.setup();

    // Setup components
    serialDebug.message("SETTING UP COMPONENTS");
    configuration.setup(&hardware, framID); // must be first: everything below uses the configuration
    configurationCliBindings.setupAll(&configuration, &cliParser);
    cliParser.addFlagGroup(testfireGroup);
    cliParser.addFlagGroup(resetBoardGroup);
    cliParser.addFlagGroup(helpGroup);
    cliParser.addStream(&serialDebug);
    simulationParser.setup(&cliParser, &serialDebug, &hardware);
    stateEstimator1D.setup(&hardware, &configuration);
    orientationEstimator.setup(&hardware, &configuration);
    flightStateDeterminer.setup(&configuration);
    dualDeploy.setup(&configuration, &droguePyro, &mainPyro);
    indicatorManager.setup(&hardware, drogueID, mainID);
    logger.setup(&hardware, &cliParser, flashID, LOG_HEADER, HEADER_OUTPUT, printLog, &configuration);
    pyroFireDuration = configuration.getConfigurable<PYRO_FIRE_DURATION_c>();
    batteryVoltageSensor.setScaleFactor(configuration.getConfigurable<BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c>().get());
    serialDebug.message("COMPONENTS SET UP COMPLETE\r\n");
    if (watchdog.causedLastReset()) logger.logMessage("Last reset was caused by the watchdog");
    watchdog.enable(100); // covers most cases; USB work still sometimes disconnects
    watchdog.pet();
}

void loop() {
    // Run core hardware
    RocketState_s state{};
    state.timestamp = hardware.enforceLoopTime();
    hardware.runAndReadAllHardware();

    // Sim data in place of sensors; compiled out of flight builds
    if (AVIONICS_ARGUMENT_isSim) {
        simulationParser.waitForEntry();
        sensorPackage.getBarometer()->inject(simulationParser.getValue(1), 0, simulationParser.getValue(0));
        sensorPackage.getAccelerometer()->inject({simulationParser.getValue(2), simulationParser.getValue(3), simulationParser.getValue(4)}, 0);
        sensorPackage.getGyroscope()->inject({simulationParser.getValue(5), simulationParser.getValue(6), simulationParser.getValue(7)}, 0);
        simulationParser.releaseEntry();
    }

    // Determine state
    state.orientation = orientationEstimator.update(state.timestamp, flightStateDeterminer.getFlightState());
    state.state1D = stateEstimator1D.update(state.timestamp, flightStateDeterminer.getFlightState());
    state.flightState = flightStateDeterminer.update(state.timestamp, state.state1D);

    buzzer.setEnabled(configuration.getConfigurable<BUZZER_ENABLED_c>().get() && !USBDevice.configured());

    if (state.flightState == PRE_FLIGHT) {
        // Only on the transition, so continuous logging can still be enabled from the CLI
        if (flightStateDeterminer.isStateTransitionTick()) logger.disableContinuousLogging();
        logger.setLogDelay(5000);
        cliParser.runCli();
        indicatorManager.beepContinuity(state.timestamp);
    } else if (state.flightState == ASCENT) {
        if (flightStateDeterminer.isStateTransitionTick()) logger.logConfig(&configuration);
        logger.enableContinuousLogging();
        indicatorManager.keepAliveBeep(state.timestamp);
    } else if (state.flightState == DESCENT) {
        logger.enableContinuousLogging();
        indicatorManager.keepAliveBeep(state.timestamp);
        dualDeploy.update(state, flightStateDeterminer);
    } else if (state.flightState == POST_FLIGHT) {
        if (flightStateDeterminer.isStateTransitionTick()) {
            uint32_t maxAltitude = max(flightStateDeterminer.getMaxAltitude(), 0);
            uint32_t maxVelocity = max(flightStateDeterminer.getMaxVelocity(), 0);
            uint32_t maxAcceleration = max(flightStateDeterminer.getMaxAcceleration(), 0);
            indicatorManager.configureBeepNums(state.timestamp, {maxAltitude, maxVelocity, maxAcceleration});
        }
        logger.disableContinuousLogging();
        logger.setLogDelay(5000);
        cliParser.runCli();
        indicatorManager.beepNums(state.timestamp);
    } else {
        logger.enableContinuousLogging();
        cliParser.runCli();
        indicatorManager.siren(state.timestamp);
    }

    configuration.pushUpdatesToMemory();
    logger.log({
            state.timestamp.runtime_ms, sensorPackage.getBarometer()->getPressurePa(), sensorPackage.getBarometer()->getTemperatureK(),
            sensorPackage.getAccelerometer()->getAccelerationsMSS_sensor().x, sensorPackage.getAccelerometer()->getAccelerationsMSS_sensor().y,
            sensorPackage.getAccelerometer()->getAccelerationsMSS_sensor().z,
            sensorPackage.getGyroscope()->getVelocitiesRadS_raw().x, sensorPackage.getGyroscope()->getVelocitiesRadS_raw().y, sensorPackage.getGyroscope()->getVelocitiesRadS_raw().z,
            sensorPackage.getGyroscope()->getTemperatureK(),
            batteryVoltageSensor.getVoltage(), state.state1D.altitudeM, state.state1D.velocityMS, state.state1D.accelerationMSS, state.state1D.unfilteredNoOffsetAltitudeM, state.flightState,
            droguePyro.hasContinuity(), droguePyro.isFired(), mainPyro.hasContinuity(), mainPyro.isFired(),
            state.orientation.tiltMagnitudeDeg,
            state.orientation.angularVelocity.x, state.orientation.angularVelocity.y, state.orientation.angularVelocity.z,
            state.orientation.angleQuaternion.a, state.orientation.angleQuaternion.b, state.orientation.angleQuaternion.c, state.orientation.angleQuaternion.d
        });
}
