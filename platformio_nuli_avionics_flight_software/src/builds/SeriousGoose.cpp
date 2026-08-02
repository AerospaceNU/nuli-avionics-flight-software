#include "Arduino.h"
#include "Avionics.h"
#include "pinmaps/SeriousGoosePinmap.h"
#include "util/Timer.h"
#include "drivers/arduino/ArduinoAvionicsHelper.h"
#include "drivers/arduino/SerialDebug.h"
#include "drivers/arduino/ArduinoSystemClock.h"
#include "drivers/arduino/UBloxV2.h"
#include "drivers/arduino/SX1262Radio.h"
#include "drivers/arduino/Ms5607Mmc5603SensorPackage.h"
#include "drivers/arduino/MX25L256.h"
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
#include "core/IndicatorManager.h"
#include "core/BasicLogger.h"
#include "core/GroundStationRelay.h"
#include "core/cli/SimpleFlag.h"
#include "core/cli/ArgumentFlag.h"
#include "core/cli/IntegratedParser.h"
#include "core/cli/SimulationParser.h"
#include "core/state_estimation/OrientationEstimator.h"
#include "core/state_estimation/StateEstimatorBasic6D.h"
#include "core/state_estimation/StateEstimator1D.h"
#include "core/transform/DiscreteRotation.h"
#include "core/triggers/ExpressionStore.h"
#include "core/triggers/TriggerConditionStore.h"

// clang-format off
struct SillyGooseLogData {
    uint32_t timestampMs;
    float pressurePa, barometerTemperatureK;
    float accelerationMSS_x, accelerationMSS_y, accelerationMSS_z, velocityRadS_x, velocityRadS_y, velocityRadS_z, imuTemperatureK;
    float magFieldTeslaRaw_x, magFieldTeslaRaw_y, magFieldTeslaRaw_z;
    float batteryVoltageV, altitudeM, velocityMS, accelerationMSS, unfilteredAltitudeM;
    int32_t flightState;
    uint8_t drogueState; bool drogueFired;
    uint8_t mainState; bool mainFired;
    uint8_t auxState; bool auxFired;
    float tiltMagnitudeDeg, angularVelRadS_x, angularVelRadS_y, angularVelRadS_z, quaternion_a, quaternion_b, quaternion_c, quaternion_d;
    float gpsLatitudeDeg, gpsLongitudeDeg, gpsAltitudeM;
    uint32_t gpsUnixTimeS;
    uint16_t gpsHdop, gpsVdop; // raw, scaled by 100 - matches the GPS module's native representation
    uint8_t gpsFixQuality, gpsSatellitesTracked;
} remove_struct_padding;
#define LOG_HEADER "timestampMs\tpressurePa\tbarometerTemperatureK\taccelerationMSS_x\taccelerationMSS_y\taccelerationMSS_z\tvelocityRadS_x\tvelocityRadS_y\tvelocityRadS_z\timuTemperatureK\tmagFieldTeslaRaw_x\tmagFieldTeslaRaw_y\tmagFieldTeslaRaw_z\tbatteryVoltageV\taltitudeM\tvelocityMS\taccelerationMSS\tunfilteredAltitudeM\tflightState\tdrogueState\tdrogueFired\tmainState\tmainFired\tauxState\tauxFired\ttiltMagnitudeDeg\tangularVelRadS_x\tangularVelRadS_y\tangularVelRadS_z\tquaternion_a\tquaternion_b\tquaternion_c\tquaternion_d\tgpsLatitudeDeg\tgpsLongitudeDeg\tgpsAltitudeM\tgpsUnixTimeS\tgpsHdop\tgpsVdop\tgpsFixQuality\tgpsSatellitesTracked"
void printLog(const SillyGooseLogData &d, DebugStream *debug) { debug->data("%lu\t%.6f\t%.2f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.2f\t%.9f\t%.9f\t%.9f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%.2f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.2f\t%lu\t%u\t%u\t%d\t%d",d.timestampMs,d.pressurePa,d.barometerTemperatureK,d.accelerationMSS_x,d.accelerationMSS_y,d.accelerationMSS_z,d.velocityRadS_x,d.velocityRadS_y,d.velocityRadS_z,d.imuTemperatureK,d.magFieldTeslaRaw_x,d.magFieldTeslaRaw_y,d.magFieldTeslaRaw_z,d.batteryVoltageV,d.altitudeM,d.velocityMS,d.accelerationMSS,d.unfilteredAltitudeM,d.flightState,d.drogueState,d.drogueFired?1:0,d.mainState,d.mainFired?1:0,d.auxState,d.auxFired?1:0,d.tiltMagnitudeDeg,d.angularVelRadS_x,d.angularVelRadS_y,d.angularVelRadS_z,d.quaternion_a,d.quaternion_b,d.quaternion_c,d.quaternion_d,d.gpsLatitudeDeg,d.gpsLongitudeDeg,d.gpsAltitudeM,d.gpsUnixTimeS,d.gpsHdop,d.gpsVdop,d.gpsFixQuality,d.gpsSatellitesTracked); };
// clang-format on

// Hardware
ArduinoSystemClock arduinoClock;
ArduinoWatchdog watchdog; // must precede serialDebug - petted during its dev-mode wait below
SerialDebug<500> serialDebug(AVIONICS_ARGUMENT_isDev, &watchdog, !AVIONICS_ARGUMENT_isSim); // Only wait for serial connection if in dev mode
const DiscreteRotation imuRotation = DiscreteRotation::identity().rotateZ90local().rotateZ90local().rotateX90local();
const DiscreteRotation magRotation = DiscreteRotation::identity().rotateZNeg90local().rotateX90local().inverse();
Ms5607Mmc5603SensorPackage sensorPackage(&imuRotation, Ms5607Mmc5603SensorPackage::ImuType::ICM42605, true, &magRotation);
MX25L256 flash(FLASH_CS_PIN);
UBloxV2 gps(&Serial1);
SX1262Radio radio(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RESET_PIN, RADIO_BUSY_PIN, RADIO_RX_EN_PIN, RADIO_TX_EN_PIN, 915.0f);
Alarm radioTransmitTimer;
ArduinoPyro mainPyro(PYRO1_GATE_PIN, PYRO1_SENSE_PIN, PYRO_CONTINUITY_THRESHOLD, PYRO_ARMED_THRESHOLD);
ArduinoPyro droguePyro(PYRO2_GATE_PIN, PYRO2_SENSE_PIN, PYRO_CONTINUITY_THRESHOLD, PYRO_ARMED_THRESHOLD);
ArduinoPyro auxPyro(PYRO3_GATE_PIN, PYRO3_SENSE_PIN, PYRO_CONTINUITY_THRESHOLD, PYRO_ARMED_THRESHOLD);
ArduinoVoltageSensor batteryVoltageSensor(VOLTAGE_SENSE_PIN, VOLTAGE_SENSE_SCALE);
ArduinoFram fram(FRAM_CS_PIN);
IndicatorLED led(LIGHT_PIN);
IndicatorBuzzer buzzer(BUZZER_PIN, 4000, 1000);

// Core components
HardwareAbstraction hardware(serialDebug, arduinoClock, 100);
FlightStateDeterminer flightStateDeterminer;
StateEstimator1D stateEstimator1D;
OrientationEstimator orientationEstimator;
BasicLogger<SillyGooseLogData> logger;
IndicatorManager indicatorManager;
IntegratedParser cliParser;
SimulationParser<8> simulationParser;
GroundStationRelay groundStationRelay;
// Expression-based pyro triggers (arbitrary CLI/FRAM-configured conditions over RocketState_s).
// Only the aux pyro is wired to one so far - drogue/main keep their existing hardcoded
// delay/elevation logic below, untouched.
ExpressionStore expressionStore;
TriggerConditionStore triggerConditionStore;
constexpr uint8_t AUX_TRIGGER_ID = 0;
uint8_t auxTriggerRootId = ExpressionStore::INVALID_ID;
// Configuration
ConfigurationID_t sillyGooseRequiredConfigs[] = {
        FIRMWARE_VERSION_c, RADIO_FREQUENCY_c, LORA_SPREADING_FACTOR_c, RADIO_TRANSMIT_INTERVAL_c, BOARD_NAME_c, DROGUE_DELAY_c, MAIN_ELEVATION_c, BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c,
        PYRO_FIRE_DURATION_c, BUZZER_ENABLED_c, GROUND_STATION_MODE_c
    };
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
ConfigurationData<uint32_t> radioTransmitDelay;
ConfigurationData<uint32_t> groundStationMode;
// CLI -> configuration bindings. Generates a CLI command to get/set configuration value.
ConfigurationCliBindings<FIRMWARE_VERSION_c,
                         RADIO_FREQUENCY_c,
                         LORA_SPREADING_FACTOR_c,
                         RADIO_TRANSMIT_INTERVAL_c,
                         DROGUE_DELAY_c,
                         MAIN_ELEVATION_c,
                         BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c,
                         GROUND_ELEVATION_c,
                         GROUND_TEMPERATURE_c,
                         PYRO_FIRE_DURATION_c,
                         BOARD_NAME_c,
                         BUZZER_ENABLED_c,
                         GROUND_STATION_MODE_c,
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
SimpleFlag testAux("-a", "Fires aux", false, [](DebugStream* debugStream) {
    debugStream->message("Firing aux");
    auxPyro.fireFor(pyroFireDuration.get());
});
SimpleFlag helpFlag("--help", "Prints all commands", true, [](DebugStream* debugStream) { cliParser.printHelp(debugStream); });
BaseFlag* testfireGroup[] = {&testfire, &testDrogue, &testMain, &testAux};
BaseFlag* resetBoardGroup[] = {&resetBoard};
BaseFlag* helpGroup[] = {&helpFlag};
// Aux pyro trigger condition, in notation form (e.g. "((altitudeM < 50) and (velocityMS < 0))").
// Mirrors ConfigurationCliBinding's get/set shape, but isn't one - the condition string is
// validated by ExpressionStore::compile() before being kept, which a plain ConfigurationString
// can't do on its own.
ArgumentFlag<const char*> auxTriggerSetFlag("-set", "New trigger condition, in notation form", false, [](DebugStream*) {});
SimpleFlag auxTriggerFlag("-auxTrigger", "Gets/sets the aux pyro's trigger condition", true, [](DebugStream* debugStream) {
    if (auxTriggerSetFlag.isSet()) {
        uint8_t newRootId;
        if (expressionStore.compile(AUX_TRIGGER_ID, auxTriggerSetFlag.getValueDerived(), &newRootId) == ExpressionValueType_e::Boolean) {
            auxTriggerRootId = newRootId;
            triggerConditionStore.setCondition(AUX_TRIGGER_ID, auxTriggerSetFlag.getValueDerived());
            debugStream->message("Aux trigger condition set");
        } else {
            debugStream->message("Aux trigger: invalid condition, unchanged");
        }
    } else {
        char buffer[TriggerConditionStore::CONDITION_LEN];
        expressionStore.conditionToString(auxTriggerRootId, buffer, sizeof(buffer));
        debugStream->message("Aux trigger condition: %s", buffer);
    }
});
BaseFlag* auxTriggerGroup[] = {&auxTriggerFlag, &auxTriggerSetFlag};

void setup() {
    // Initialize
    watchdog.disable(); // SAMD's WDT survives NVIC_SystemReset()
    watchdog.enable(4000);
    disableChipSelectPins({FRAM_CS_PIN, FLASH_CS_PIN, RADIO_CS_PIN}); // All CS pins must disable prior to SPI device setup on multi-device buses to prevent one device from locking the bus
    configuration.setDefault<BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c>(VOLTAGE_SENSE_SCALE); // Configuration defaults MUST be called prior to configuration.setup() for it to have effect
    configuration.setDefault<BOARD_NAME_c>(SERIOUS_GOOSE_NAME);
    if (AVIONICS_ARGUMENT_isDev) led.setOutputPercent(6.0f); // Lower the LED Power

    // Setup Hardware
    int16_t framID = hardware.appendFramMemory(&fram);
    int16_t flashID = hardware.appendFlashMemory(&flash);
    int16_t drogueID = hardware.appendPyro(&droguePyro);
    int16_t mainID = hardware.appendPyro(&mainPyro);
    hardware.appendPyro(&auxPyro); // without this, run() never fires -> fireFor() never times out and continuity never updates
    hardware.appendVoltageSensor(&batteryVoltageSensor);
    hardware.appendGenericHardware(&sensorPackage);
    hardware.appendBarometer(sensorPackage.getBarometer());
    hardware.appendAccelerometer(sensorPackage.getAccelerometer());
    hardware.appendGyroscope(sensorPackage.getGyroscope());
    hardware.appendMagnetometer(sensorPackage.getMagnetometer());
    hardware.appendIndicator(&led);
    hardware.appendIndicator(&buzzer);
    hardware.appendRadioLink(&radio);
    hardware.appendGPS(&gps);
    hardware.setWatchdogTimer(&watchdog);
    hardware.setup();

    // Setup components
    serialDebug.message("SETTING UP COMPONENTS");
    configuration.setup(&hardware, framID); // Must be called first, for everything else to be able to use the configuration
    groundStationMode = configuration.getConfigurable<GROUND_STATION_MODE_c>();
    configurationCliBindings.setupAll(&configuration, &cliParser);
    cliParser.addFlagGroup(testfireGroup);
    groundStationRelay.setup(&cliParser, &radio, &gps);
    cliParser.addFlagGroup(resetBoardGroup);
    cliParser.addFlagGroup(helpGroup);
    // Trigger conditions live in the same FRAM chip as configuration, just past its region
    // (TriggerConditionStore::FRAM_OFFSET == MAX_CONFIGURATION_LENGTH) - independent of Configuration
    // so an invalid/uncompilable condition can never corrupt or invalidate the real configuration.
    triggerConditionStore.setup(hardware.getFramMemory(framID), &serialDebug);
    expressionStore.compile(AUX_TRIGGER_ID, triggerConditionStore.getCondition(AUX_TRIGGER_ID), &auxTriggerRootId);
    cliParser.addFlagGroup(auxTriggerGroup);
    cliParser.addStream(&serialDebug);
    simulationParser.setup(&cliParser, &serialDebug, &hardware);
    stateEstimator1D.setup(&hardware, &configuration);
    orientationEstimator.setup(&hardware, &configuration);
    flightStateDeterminer.setup(&configuration);
    indicatorManager.setup(&hardware, drogueID, mainID);
    logger.setup(&hardware, &cliParser, flashID, LOG_HEADER, printLog, &configuration);
    // Locally used configuration variables
    drogueDelay = configuration.getConfigurable<DROGUE_DELAY_c>();
    mainElevation = configuration.getConfigurable<MAIN_ELEVATION_c>();
    pyroFireDuration = configuration.getConfigurable<PYRO_FIRE_DURATION_c>();
    radioTransmitDelay = configuration.getConfigurable<RADIO_TRANSMIT_INTERVAL_c>();
    batteryVoltageSensor.setScaleFactor(configuration.getConfigurable<BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c>().get());
    // Radio
    radio.setFrequency(configuration.getConfigurable<RADIO_FREQUENCY_c>().get());
    radio.setSpreadingFactor(configuration.getConfigurable<LORA_SPREADING_FACTOR_c>().get());
    radioTransmitTimer.startAlarm(0, 0); // Trigger right away
    // All done
    serialDebug.message("COMPONENTS SET UP COMPLETE\r\n");
    if (watchdog.causedLastReset()) logger.logMessage("Previous run was ended by the watchdog (main loop stalled past its timeout)");
    watchdog.enable(100);
    watchdog.pet();
}

void loop() {
    // Run core hardware
    RocketState_s state{};
    state.timestamp = hardware.enforceLoopTime();
    hardware.runAndReadAllHardware(); // Reads sensors, runs any background code for every hardware device

    // Read in sim data. This should be optimized out by the compiler in the final deployment
    if (AVIONICS_ARGUMENT_isSim) {
        simulationParser.waitForEntry();
        sensorPackage.getBarometer()->inject(simulationParser.getValue(1), 0, simulationParser.getValue(0));
        sensorPackage.getAccelerometer()->inject({simulationParser.getValue(2), simulationParser.getValue(3), simulationParser.getValue(4)}, 0);
        sensorPackage.getGyroscope()->inject({simulationParser.getValue(5), simulationParser.getValue(6), simulationParser.getValue(7)}, 0);
        simulationParser.releaseEntry();
    }

    if (!groundStationMode.get()) {
        // Determine state
        state.rawGps = gps.getCoordinates();
        state.orientation = orientationEstimator.update(state.timestamp, flightStateDeterminer.getFlightState());
        state.state1D = stateEstimator1D.update(state.timestamp, flightStateDeterminer.getFlightState());
        state.flightState = flightStateDeterminer.update(state.timestamp, state.state1D);

        // Aux pyro trigger: fires once when its (CLI/FRAM-configured) condition first evaluates
        // true. Independent of flightState by design - if you want it gated to a phase of
        // flight, put that in the condition itself (e.g. "((flightState == 2) and ...)").
        expressionStore.tick(state);
        static bool auxTriggerFired = false;
        if (!auxTriggerFired && expressionStore.getBooleanValue(auxTriggerRootId)) {
            auxPyro.fireFor(pyroFireDuration.get());
            auxTriggerFired = true;
        }

        // Turn on/off the buzzer
        buzzer.setEnabled(configuration.getConfigurable<BUZZER_ENABLED_c>().get() && !USBDevice.configured());

        // State machine to determine when to do what
        if (state.flightState == PRE_FLIGHT) {
            // Disable logging when transition into PRE_FLIGHT, but allow for continues logging to manually be enabled through the cli
            if (flightStateDeterminer.isStateTransitionTick()) logger.disableContinuousLogging();
            logger.setLogDelay(5000); // Set default log rate
            cliParser.runCli();
            indicatorManager.beepContinuity(state.timestamp);
        } else if (state.flightState == ASCENT) {
            if (flightStateDeterminer.isStateTransitionTick()) logger.logConfig(&configuration);     // Log config again
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
        triggerConditionStore.pushUpdatesToMemory();
        // Run logging
        SillyGooseLogData logData = {
                state.timestamp.runtime_ms, sensorPackage.getBarometer()->getPressurePa(), sensorPackage.getBarometer()->getTemperatureK(),
                sensorPackage.getAccelerometer()->getAccelerationsMSS_sensor().x, sensorPackage.getAccelerometer()->getAccelerationsMSS_sensor().y,
                sensorPackage.getAccelerometer()->getAccelerationsMSS_sensor().z,
                sensorPackage.getGyroscope()->getVelocitiesRadS_raw().x, sensorPackage.getGyroscope()->getVelocitiesRadS_raw().y, sensorPackage.getGyroscope()->getVelocitiesRadS_raw().z,
                sensorPackage.getGyroscope()->getTemperatureK(),
                sensorPackage.getMagnetometer()->getMagneticFieldTesla_sensor().x, sensorPackage.getMagnetometer()->getMagneticFieldTesla_sensor().y,
                sensorPackage.getMagnetometer()->getMagneticFieldTesla_sensor().z,
                batteryVoltageSensor.getVoltage(), state.state1D.altitudeM, state.state1D.velocityMS, state.state1D.accelerationMSS, state.state1D.unfilteredNoOffsetAltitudeM, state.flightState,
                droguePyro.stateByte(), droguePyro.isFired(), mainPyro.stateByte(), mainPyro.isFired(), auxPyro.stateByte(), auxPyro.isFired(),
                state.orientation.tiltMagnitudeDeg,
                state.orientation.angularVelocity.x, state.orientation.angularVelocity.y, state.orientation.angularVelocity.z,
                state.orientation.angleQuaternion.a, state.orientation.angleQuaternion.b, state.orientation.angleQuaternion.c, state.orientation.angleQuaternion.d,
                state.rawGps.latitudeDeg, state.rawGps.longitudeDeg, state.rawGps.altitudeM,
                gps.getUnixTimeS(), gps.getHDOP(), gps.getVDOP(), gps.getFixQuality(), gps.getSatellitesTracked()
            };
        logger.log(logData);

        if (radioTransmitTimer.isAlarmFinished(state.timestamp.runtime_ms) && radio.startTransmit(&logData, sizeof(logData))) {
            radioTransmitTimer.startAlarm(state.timestamp.runtime_ms, radioTransmitDelay.get());
        }
        if (radio.isMessageAvailable()) {
            const RadioLink::RadioMessage receivedMessage = radio.readMessage(); // assumes a null-terminated C string for now
            serialDebug.message("Radio: %s (RSSI: %d, SNR: %.2f)", (const char*)receivedMessage.data, receivedMessage.rssi, receivedMessage.snr);
        }
    } else {
        indicatorManager.keepAliveBeep(state.timestamp);
        cliParser.runCli();
        configuration.pushUpdatesToMemory();
        groundStationRelay.tick(state.timestamp, &serialDebug);
    }
}
