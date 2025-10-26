#include "Avionics.h"
AVIONICS_DESKTOP_MAIN
#include "drivers/desktop/CSVParser.h"
#include "drivers/desktop/DesktopDebug.h"
#include "drivers/desktop/DesktopSystemClock.h"
#include "drivers/desktop/DummySystemClock.h"
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

CSVReader csvReader;

// Hardware
DummySystemClock desktopClock(100);
DesktopDebug debug;
Barometer barometer;
const DiscreteRotation imuRotation = DiscreteRotation::identity();
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
ConfigurationID_t desktopRequiredConfigs[] = {BOARD_NAME_c};
Configuration configuration({desktopRequiredConfigs, Configuration::REQUIRED_CONFIGS, FlightStateDeterminer::REQUIRED_CONFIGS, StateEstimator1D::REQUIRED_CONFIGS, OrientationEstimator::REQUIRED_CONFIGS});
ConfigurationCliBindings<GROUND_ELEVATION_c, GROUND_TEMPERATURE_c, BOARD_NAME_c, CONFIGURATION_VERSION_c> configurationCliBindings;

void setup() {
    csvReader.setup("../simulation/data/Avionics Flight Data - 2023-04-15-beanboozler-output-FCB.csv");
    debug.outputToFile("../simulation/output.txt");

    // Initialize
    configuration.setDefault<BOARD_NAME_c>("Desktop");
    configuration.setDefault<FLIGHT_STATE_c>(PRE_FLIGHT);

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

    // Read in the .csv data
    csvReader.interpolateNext(state.timestamp.runtime_ms);
    barometer.inject((csvReader.getKey<float>("baro1_temp") - 32) * 5.0f / 9.0f + 273.15f, 0, csvReader.getKey<float>("baro1_pres") * 101325);
    accelerometer.inject({csvReader.getKey<float>("imu1_accel_x") / 128.0f, csvReader.getKey<float>("imu1_accel_y") / 128.0f, csvReader.getKey<float>("imu1_accel_z") / 128.0f}, 0);
    gyroscope.inject({csvReader.getKey<float>("imu1_gyro_x") / 128.0f, csvReader.getKey<float>("imu1_gyro_y") / 128.0f, csvReader.getKey<float>("imu1_gyro_z") / 128.0f}, 0);

    // Determine state
    state.orientation = orientationEstimator.update(state.timestamp, flightStateDeterminer.getFlightState());
    state.state1D = stateEstimator1D.update(state.timestamp, flightStateDeterminer.getFlightState());
    state.state6D = stateEstimatorBasic6D.update(state.timestamp, state.state1D, state.orientation);
    state.flightState = flightStateDeterminer.update(state.timestamp, state.state1D);

    // Run CLI
    cliParser.runCli();

    // Update any changes to the configuration
    configuration.pushUpdatesToMemory();

    // Print out current values
    debug.message("%d\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%d",
                  state.timestamp.runtime_ms,
                  barometer.getPressurePa(), barometer.getTemperatureK(),
                  accelerometer.getAccelerationsMSS_sensor().x, accelerometer.getAccelerationsMSS_sensor().y, accelerometer.getAccelerationsMSS_sensor().z,
                  gyroscope.getVelocitiesRadS_raw().x, gyroscope.getVelocitiesRadS_raw().y, gyroscope.getVelocitiesRadS_raw().z,
                  state.state1D.altitudeM, state.state1D.velocityMS, state.state1D.accelerationMSS, state.state1D.unfilteredNoOffsetAltitudeM, state.flightState
    );
}
