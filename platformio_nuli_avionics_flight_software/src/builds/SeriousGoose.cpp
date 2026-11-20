#include "Arduino.h"
#include "Avionics.h"
#include "pinmaps/SeriousGoosePinmap.h"
#include "util/Timer.h"
#include "drivers/arduino/ArduinoAvionicsHelper.h"
#include "drivers/arduino/SerialDebug.h"
#include "drivers/arduino/ArduinoSystemClock.h"
#include "drivers/arduino/UBloxV2.h"
#include "drivers/arduino/UBloxM10.h"
#include "drivers/samd/SercomUart.h"
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
#include "core/DualDeploy.h"
#include "core/IndicatorManager.h"
#include "core/BasicLogger.h"
#include "core/BasicLoggerHelper.h"
#include "builds/SeriousGooseLogData.h"
#include "core/GroundStationRelay.h"
#include "core/cli/SimpleFlag.h"
#include "core/cli/ArgumentFlag.h"
#include "core/cli/DebugStreamQueue.h"
#include "core/cli/IntegratedParser.h"
#include "core/cli/SimulationParser.h"
#include "core/state_estimation/OrientationEstimator.h"
#include "core/state_estimation/StateEstimatorBasic6D.h"
#include "core/state_estimation/StateEstimator1D.h"
#include "core/transform/DiscreteRotation.h"

static constexpr const char* LOG_HEADER = BasicLoggerHelper::generateBasicLogHeader<HEADER_OUTPUT>();
void printLog(const SillyGooseLogData &d, DebugStream *debug) {
    BasicLoggerHelper::genericBasicLogDataPrinter<HEADER_OUTPUT>(d, debug);
}

// Hardware
ArduinoSystemClock arduinoClock;
ArduinoWatchdog watchdog; // must precede serialDebug - petted during its dev-mode wait below
SerialDebug<500> serialDebug(AVIONICS_ARGUMENT_isDev, &watchdog, !AVIONICS_ARGUMENT_isSim); // Only wait for serial connection if in dev mode
const DiscreteRotation imuRotation = DiscreteRotation::identity().rotateZ90local().rotateZ90local().rotateX90local();
const DiscreteRotation magRotation = DiscreteRotation::identity().rotateZNeg90local().rotateX90local().inverse();
Ms5607Mmc5603SensorPackage sensorPackage(&imuRotation, Ms5607Mmc5603SensorPackage::ImuType::ICM42605, true, &magRotation);
MX25L256 flash(FLASH_CS_PIN);
SercomUart auxSerial(&sercom0, AUX_1_PIN, AUX_2_PIN, SERCOM_RX_PAD_1, UART_TX_PAD_0);
SERCOM_UART_HANDLERS(0, auxSerial)
UBloxV2 onboardGps(&Serial1);
UBloxM10 auxGps(&auxSerial);
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
HardwareAbstraction hardware(serialDebug, arduinoClock, watchdog, 100);
FlightStateDeterminer flightStateDeterminer;
DualDeploy dualDeploy;
StateEstimator1D stateEstimator1D;
OrientationEstimator orientationEstimator;
BasicLogger<SillyGooseLogData> logger;
IndicatorManager indicatorManager;
IntegratedParser cliParser;
SimulationParser<8> simulationParser;
GroundStationRelay groundStationRelay;
DebugStreamQueue<128, 1024> radioCli; // CLI channel over the radio link - see loop()'s radio TX/RX handling
// Configuration
ConfigurationID_t sillyGooseRequiredConfigs[] = {
        FIRMWARE_VERSION_c, RADIO_FREQUENCY_c, LORA_SPREADING_FACTOR_c, RADIO_TRANSMIT_INTERVAL_c, BOARD_NAME_c, DROGUE_DELAY_c, MAIN_ELEVATION_c, BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c,
        PYRO_FIRE_DURATION_c, BUZZER_ENABLED_c, GROUND_STATION_MODE_c, ARMED_c, GPS_SOURCE_c
    };
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
ConfigurationData<uint32_t> armed;
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
                         cliReadOnly(GROUND_ELEVATION_c),
                         cliReadOnly(GROUND_TEMPERATURE_c),
                         PYRO_FIRE_DURATION_c,
                         BOARD_NAME_c,
                         BUZZER_ENABLED_c,
                         GROUND_STATION_MODE_c,
                         GPS_SOURCE_c,
                         CONFIGURATION_VERSION_c> configurationCliBindings;
// CLI
SimpleFlag resetBoard("--reset", "Reboots the board", true, [](DebugStream*) { NVIC_SystemReset(); });
SimpleFlag testfire("--fire", "Test-fires a pyro", true, [](DebugStream*) {});
SimpleFlag testOn("-on", "Holds the pyro on until -off, a reboot or --disarm instead of firing it for the pyro fire duration", false, [](DebugStream*) {});
SimpleFlag testOff("-off", "Turns the pyro off", false, [](DebugStream*) {});
void testFirePyro(Pyro& pyro, const char* name, DebugStream* debugStream) {
    if (testOn.isSet() && testOff.isSet()) {
        debugStream->error("Use -on or -off, not both");
        return;
    }
    if (testOff.isSet()) {
        debugStream->message("Turning off %s", name);
        pyro.disable();
        return;
    }
    if (pyro.isLocked()) {
        debugStream->error("Disarmed: not firing %s", name);
        return;
    }
    debugStream->message(testOn.isSet() ? "Turning on %s" : "Firing %s", name);
    if (testOn.isSet()) {
        pyro.turnOn();
    } else {
        pyro.fireFor(pyroFireDuration.get());
    }
}
SimpleFlag testDrogue("-d", "Fires drogue", false, [](DebugStream* debugStream) { testFirePyro(droguePyro, "drogue", debugStream); });
SimpleFlag testMain("-m", "Fires main", false, [](DebugStream* debugStream) { testFirePyro(mainPyro, "main", debugStream); });
SimpleFlag testAux("-a", "Fires aux", false, [](DebugStream* debugStream) { testFirePyro(auxPyro, "aux", debugStream); });
SimpleFlag helpFlag("--help", "Prints all commands", true, [](DebugStream* debugStream) { cliParser.printHelp(debugStream); });
BaseFlag* testfireGroup[] = {&testfire, &testOn, &testOff, &testDrogue, &testMain, &testAux};
SimpleFlag armFlag("--arm", "Arms the board: pyros can fire", true, [](DebugStream* debugStream) {
    armed.set(true);
    debugStream->message("ARMED has been set to: %d", (int)armed.get());
});
SimpleFlag disarmFlag("--disarm", "Disarms the board: pyros are locked off, the flight is still tracked and logged", true, [](DebugStream* debugStream) {
    armed.set(false);
    debugStream->message("ARMED has been set to: %d", (int)armed.get());
});
SimpleFlag armedFlag("--armed", "Reports whether the board is armed", true, [](DebugStream* debugStream) {
    debugStream->message("ARMED is set to: %d", (int)armed.get());
});
BaseFlag* resetBoardGroup[] = {&resetBoard};
BaseFlag* armGroup[] = {&armFlag};
BaseFlag* disarmGroup[] = {&disarmFlag};
BaseFlag* armedGroup[] = {&armedFlag};
BaseFlag* helpGroup[] = {&helpFlag};

void setup() {
    // Initialize
    watchdog.disable(); // SAMD's WDT survives NVIC_SystemReset()
    watchdog.enable(4000);
    disableChipSelectPins({FRAM_CS_PIN, FLASH_CS_PIN, RADIO_CS_PIN}); // All CS pins must disable prior to SPI device setup on multi-device buses to prevent one device from locking the bus
    configuration.setDefault<BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c>(VOLTAGE_SENSE_SCALE); // Configuration defaults MUST be called prior to configuration.setup() for it to have effect
    configuration.setDefault<BOARD_NAME_c>(SERIOUS_GOOSE_NAME);
    if (AVIONICS_ARGUMENT_isDev) led.setOutputPercent(6.0f); // Lower the LED Power

    // Setup Hardware
    hardware.setup();
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

    // Setup components
    serialDebug.message("SETTING UP COMPONENTS");
    configuration.setup(&hardware, framID); // Must be called first, for everything else to be able to use the configuration
    if (configuration.getConfigurable<GPS_SOURCE_c>().get() == 1) {
        hardware.appendGPS(&auxGps);
    } else {
        hardware.appendGPS(&onboardGps);
    }
    configurationCliBindings.setupAll(&configuration, &cliParser);
    cliParser.addFlagGroup(testfireGroup);
    groundStationRelay.setup(&cliParser, &radio, hardware.getGPS(0));
    cliParser.addFlagGroup(resetBoardGroup);
    cliParser.addFlagGroup(armGroup);
    cliParser.addFlagGroup(disarmGroup);
    cliParser.addFlagGroup(armedGroup);
    cliParser.addFlagGroup(helpGroup);
    cliParser.addStream(&serialDebug);
    cliParser.addStream(&radioCli);
    simulationParser.setup(&cliParser, &serialDebug, &hardware);
    stateEstimator1D.setup(&hardware, &configuration);
    orientationEstimator.setup(&hardware, &configuration);
    flightStateDeterminer.setup(&configuration);
    dualDeploy.setup(&configuration, &droguePyro, &mainPyro);
    indicatorManager.setup(&hardware, drogueID, mainID);
    logger.setup(&hardware, &cliParser, flashID, LOG_HEADER, HEADER_OUTPUT, printLog, &configuration);
    // Locally used configuration variables
    pyroFireDuration = configuration.getConfigurable<PYRO_FIRE_DURATION_c>();
    armed = configuration.getConfigurable<ARMED_c>();
    radioTransmitDelay = configuration.getConfigurable<RADIO_TRANSMIT_INTERVAL_c>();
    batteryVoltageSensor.setScaleFactor(configuration.getConfigurable<BATTERY_VOLTAGE_SENSOR_SCALE_FACTOR_c>().get());
    groundStationMode = configuration.getConfigurable<GROUND_STATION_MODE_c>();
    // Radio
    radio.setFrequency(configuration.getConfigurable<RADIO_FREQUENCY_c>().get());
    radio.setSpreadingFactor(configuration.getConfigurable<LORA_SPREADING_FACTOR_c>().get());
    radioTransmitTimer.startAlarm(0, 0); // Trigger right away
    // All done
    serialDebug.message("COMPONENTS SET UP COMPLETE\r\n");
    if (watchdog.causedLastReset()) logger.logMessage("Previous run was ended by the watchdog (main loop stalled past its timeout)");
    watchdog.enable(125);
    watchdog.pet();
}

void loop() {
    // Run core hardware
    RocketState_s state{};
    state.timestamp = hardware.enforceLoopTime();
    hardware.setArmed(armed.get());
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
        state.rawGps = hardware.getGPS(0)->getCoordinates();
        state.orientation = orientationEstimator.update(state.timestamp, flightStateDeterminer.getFlightState());
        state.state1D = stateEstimator1D.update(state.timestamp, flightStateDeterminer.getFlightState());
        state.flightState = flightStateDeterminer.update(state.timestamp, state.state1D);

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
            if (flightStateDeterminer.isStateTransitionTick()) radioCli.clearIncoming();
            if (flightStateDeterminer.isStateTransitionTick()) logger.logConfig(&configuration);     // Log config again
            logger.enableContinuousLogging();
            indicatorManager.keepAliveBeep(state.timestamp);
        } else if (state.flightState == DESCENT) {
            if (flightStateDeterminer.isStateTransitionTick()) radioCli.clearIncoming();
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

        // Update any changes to the configuration
        configuration.pushUpdatesToMemory();
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
                hardware.getGPS(0)->getUnixTimeS(), hardware.getGPS(0)->getHDOP(), hardware.getGPS(0)->getVDOP(), hardware.getGPS(0)->getFixQuality(), hardware.getGPS(0)->getSatellitesTracked()
            };
        logger.log(logData);

        if (radioTransmitTimer.isAlarmFinished(state.timestamp.runtime_ms)) {
            // A pending CLI response (from a command received over radioCli below) takes this
            // transmit slot instead of telemetry - it'll resume telemetry once drained.
            bool sent;
            if (radioCli.hasOutgoing()) {
                uint8_t txBuffer[129]; // 1 type byte + up to 128 payload bytes
                txBuffer[0] = RADIO_MSG_CLI_RESPONSE;
                uint8_t n = radioCli.popOutgoing(txBuffer + 1, sizeof(txBuffer) - 1);
                sent = radio.startTransmit(txBuffer, n + 1);
            } else {
                struct { uint8_t type; SillyGooseLogData payload; } remove_struct_padding txBuffer = {RADIO_MSG_TELEMETRY, logData};
                sent = radio.startTransmit(&txBuffer, sizeof(txBuffer));
            }
            if (sent) radioTransmitTimer.startAlarm(state.timestamp.runtime_ms, radioTransmitDelay.get());
        }
        if (radio.isMessageAvailable()) {
            const RadioLink::RadioMessage receivedMessage = radio.readMessage();
            if (receivedMessage.length >= 1 && receivedMessage.data[0] == RADIO_MSG_CLI_COMMAND) {
                if (state.flightState != ASCENT && state.flightState != DESCENT) radioCli.pushIncoming(receivedMessage.data + 1, receivedMessage.length - 1);
            } else {
                serialDebug.warn("Radio: unrecognized message type %u (RSSI: %d, SNR: %.2f)", receivedMessage.data[0], receivedMessage.rssi, receivedMessage.snr);
            }
        }
    } else {
        buzzer.setEnabled(false);
        stateEstimator1D.reset();
        indicatorManager.keepAliveBeep(state.timestamp);
        cliParser.runCli();
        configuration.pushUpdatesToMemory();
        groundStationRelay.tick(state.timestamp, &serialDebug);
    }
}
