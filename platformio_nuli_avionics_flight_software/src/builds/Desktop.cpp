#include "Avionics.h"
AVIONICS_DESKTOP_MAIN
#include "drivers/desktop/CSVParser.h"
#include "drivers/desktop/DesktopDebug.h"
#include "drivers/desktop/DummySystemClock.h"
#include "drivers/desktop/DesktopSerialReader.h"
#include "core/HardwareAbstraction.h"
#include "core/cli/IntegratedParser.h"
#include "core/transform/DiscreteRotation.h"
#include "core/configuration/Configuration.h"
#include "core/configuration/ConfigurationCliBinding.h"
#include "core/state_estimation/FlightStateDeterminer.h"
#include "core/state_estimation/OrientationEstimator.h"
#include "core/state_estimation/StateEstimatorBasic6D.h"
#include "core/state_estimation/StateEstimator1D.h"

// Desktop sim specific stuff
constexpr float FCB_V0_ACCEL_SCALE_FACTOR = 0.0071784678f;
constexpr float FCB_V0_GYRO_SCALE_FACTOR = 0.0012217305f;
CSVReader csvReader;

// Hardware
DummySystemClock desktopClock(100);
DesktopDebug debug;
Barometer barometer;
const DiscreteRotation imuRotation = DiscreteRotation::identity().rotateZ90local().rotateZ90local().rotateX90local().inverse();
Accelerometer accelerometer(&imuRotation);
Gyroscope gyroscope(&imuRotation);
VolatileConfigurationMemory<1000> fram;

// Core components
HardwareAbstraction hardware;
FlightStateDeterminer flightStateDeterminer;
StateEstimator1D stateEstimator1D;
OrientationEstimator orientationEstimator;
StateEstimatorBasic6D stateEstimatorBasic6D(true);
DesktopSerialReader<1000> serialReader;
IntegratedParser cliParser;
ConfigurationID_t desktopRequiredConfigs[] = {BOARD_NAME_c};
Configuration configuration({desktopRequiredConfigs, Configuration::REQUIRED_CONFIGS, FlightStateDeterminer::REQUIRED_CONFIGS, StateEstimator1D::REQUIRED_CONFIGS, OrientationEstimator::REQUIRED_CONFIGS});
ConfigurationCliBindings<GROUND_ELEVATION_c, GROUND_TEMPERATURE_c, BOARD_NAME_c, CONFIGURATION_VERSION_c> configurationCliBindings;

void setup() {
    // Setup sim input/output
    // csvReader.setup("../simulation/data/Avionics Flight Data - 2023-04-15-beanboozler-output-FCB.csv");
    csvReader.setup("../simulation/data/MBTA_FLIGHT_DATA.txt");
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
    hardware.readSensors(); // Has no effect because we are using simulated data
    // Read in the .csv data
    csvReader.interpolateNext(state.timestamp.runtime_ms); // The FCB recorded at ~50 hz, and our code will run at 100hz
    barometer.inject(csvReader.getKey<float>("barometerTemperatureK"), 0, csvReader.getKey<float>("pressurePa"));
    accelerometer.inject({csvReader.getKey<float>("accelerationMSS_x"), csvReader.getKey<float>("accelerationMSS_y"), csvReader.getKey<float>("accelerationMSS_z")}, 0);
    gyroscope.inject({csvReader.getKey<float>("velocityRadS_x"), csvReader.getKey<float>("velocityRadS_y"), csvReader.getKey<float>("velocityRadS_z")}, 0);


    // Determine state
    state.orientation = orientationEstimator.update(state.timestamp, flightStateDeterminer.getFlightState());
    state.state1D = stateEstimator1D.update(state.timestamp, flightStateDeterminer.getFlightState());
    state.state6D = stateEstimatorBasic6D.update(state.timestamp, state.state1D, state.orientation, flightStateDeterminer.getFlightState());
    state.flightState = flightStateDeterminer.update(state.timestamp, state.state1D);

    // Run CLI
    cliParser.runCli();

    // Update any changes to the configuration
    configuration.pushUpdatesToMemory();

    // Print out current values
    // debug.message("%d\t%.4f\t%.1f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.4f\t%.2f\t%.2f\t%.4f\t%.2f\t%d",
    //               state.timestamp.runtime_ms,
    //               barometer.getPressurePa(), barometer.getTemperatureK(),
    //               accelerometer.getAccelerationsMSS_sensor().x, accelerometer.getAccelerationsMSS_sensor().y, accelerometer.getAccelerationsMSS_sensor().z,
    //               gyroscope.getVelocitiesRadS_raw().x, gyroscope.getVelocitiesRadS_raw().y, gyroscope.getVelocitiesRadS_raw().z,
    //               state.state1D.altitudeM, state.state1D.velocityMS, state.state1D.accelerationMSS, state.state1D.unfilteredNoOffsetAltitudeM, state.flightState
    // );
    // debug.message("%.2f\t%.2f\t%.2f\t%d", state.orientation.tiltMagnitudeDeg, state.state1D.altitudeM,state.state1D.unfilteredNoOffsetAltitudeM, state.flightState);

    if (state.orientation.tiltMagnitudeDeg > 88 && state.timestamp.tick > 100) {
        exit(0);
    }

    debug.message("%d\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%d",
        state.timestamp.runtime_ms,
        state.state6D.position.x, state.state6D.position.y, state.state6D.position.z,
        state.state6D.velocity.x, state.state6D.velocity.y, state.state6D.velocity.z,
        state.state6D.acceleration.x, state.state6D.acceleration.y, state.state6D.acceleration.z,
        state.orientation.tiltMagnitudeDeg, state.flightState);

}
