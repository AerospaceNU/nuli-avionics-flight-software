#include "Avionics.h"
AVIONICS_DESKTOP_MAIN
#include "drivers/desktop/DesktopDebug.h"
#include "drivers/desktop/DesktopSystemClock.h"
#include "drivers/desktop/DesktopSerialReader.h"
#include "core/HardwareAbstraction.h"
#include "core/Configuration.h"
#include "core/ConfigurationCliBinding.h"
#include "core/FlightStateDeterminer.h"
#include "core/BasicLogger.h"
#include "core/cli/IntegratedParser.h"
#include "core/filters/OrientationEstimator.h"
#include "core/filters/StateEstimatorBasic6D.h"
#include "core/filters/StateEstimator1D.h"
#include "core/transform/DiscreteRotation.h"

// Hardware
DesktopSystemClock desktopClock;
DesktopDebug debug;
Barometer barometer;
const DiscreteRotation imuRotation = DiscreteRotation::identity().rotateZNeg90().rotateX90().inverse();
Accelerometer accelerometer(&imuRotation);
Gyroscope gyroscope(&imuRotation);
VolatileConfigurationMemory<1000> fram;

// Core components
HardwareAbstraction hardware;
FlightStateDeterminer flightStateDeterminer;
StateEstimator1D stateEstimator1D;
OrientationEstimator orientationEstimator;
StateEstimatorBasic6D stateEstimatorBasic6D;
DesktopSerialReader<1000> serialReader;
IntegratedParser cliParser;
// // Configuration
ConfigurationID_t desktopRequiredConfigs[] = {BOARD_NAME_c};
Configuration configuration({
        desktopRequiredConfigs,
        Configuration::REQUIRED_CONFIGS,
        FlightStateDeterminer::REQUIRED_CONFIGS,
        StateEstimator1D::REQUIRED_CONFIGS,
        OrientationEstimator::REQUIRED_CONFIGS
    });
// // CLI -> configuration bindings. Generates a CLI command to get/set configuration value.
ConfigurationCliBindings<GROUND_ELEVATION_c, GROUND_TEMPERATURE_c, BOARD_NAME_c, CONFIGURATION_VERSION_c> configurationCliBindings;

void setup() {
    // Initialize
    configuration.setDefault<BOARD_NAME_c>("Desktop");

    // Setup Hardware
    const int16_t framID = hardware.appendFramMemory(&fram);
    hardware.appendBarometer(&barometer);
    hardware.appendAccelerometer(&accelerometer);
    hardware.appendGyroscope(&gyroscope);
    hardware.setup(&debug, &desktopClock, 100);

    // Setup components
    debug.message("SETTING UP COMPONENTS");
    configuration.setup(&hardware, framID); // Must be called first, for everything else to be able to use the configuration
    configurationCliBindings.setupAll(&configuration, &cliParser, &debug);
    cliParser.setup(&serialReader, &debug);
    stateEstimator1D.setup(&hardware, &configuration);
    orientationEstimator.setup(&hardware, &configuration);
    stateEstimatorBasic6D.setup(&hardware, &configuration);
    flightStateDeterminer.setup(&configuration);
    debug.message("COMPONENTS SET UP COMPLETE\r\n");
}

void loop() {
    // Run core hardware
    RocketState_s state{};
    state.timestamp = hardware.enforceLoopTime();
    hardware.readSensors();


    // Determine state
    // state.orientation = orientationEstimator.update(state.timestamp, flightStateDeterminer.getFlightState());
    state.state1D = stateEstimator1D.update(state.timestamp, flightStateDeterminer.getFlightState());
    // state.state6D = stateEstimatorBasic6D.update(state.timestamp, state.state1D, state.orientation);
    // state.flightState = flightStateDeterminer.update(state.timestamp, state.state1D);

    // Run CLI
    cliParser.runCli();

    // Update any changes to the configuration
    configuration.pushUpdatesToMemory();
}