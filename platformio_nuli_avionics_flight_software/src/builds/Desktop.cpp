#include "Avionics.h"
#include "drivers/desktop/DesktopDebug.h"
#include "drivers/desktop/DesktopFlash.h"
#include "drivers/desktop/DesktopPyro.h"
#include "drivers/desktop/DesktopSystemClock.h"
#include "drivers/desktop/SimulationLogFile.h"
#include "core/HardwareAbstraction.h"
#include "core/BasicLogger.h"
#include "core/BasicLoggerHelper.h"
#include "core/DualDeploy.h"
#include "core/cli/IntegratedParser.h"
#include "core/configuration/Configuration.h"
#include "core/state_estimation/FlightStateDeterminer.h"
#include "core/state_estimation/OrientationEstimator.h"
#include "core/state_estimation/StateEstimator1D.h"
#include "core/transform/DiscreteRotation.h"
#include <memory>

// clang-format off
LOG_DATA_AUTO_DEFINE(HEADER_OUTPUT,
    struct DesktopLogData {
        uint32_t timestampMs;
        float pressurePa, barometerTemperatureK;
        float accelerationMSS_x, accelerationMSS_y, accelerationMSS_z, velocityRadS_x, velocityRadS_y, velocityRadS_z;
        float altitudeM, velocityMS, accelerationMSS, unfilteredAltitudeM;
        int32_t flightState;
        bool barometerValid, drogueFired, mainFired;
        float tiltMagnitudeDeg;
    } remove_struct_padding;
)
// clang-format on
static constexpr const char* LOG_HEADER = BasicLoggerHelper::generateBasicLogHeader<HEADER_OUTPUT>();
void printLog(const DesktopLogData& d, DebugStream* debug) {
    BasicLoggerHelper::genericBasicLogDataPrinter<HEADER_OUTPUT>(d, debug);
}

constexpr uint32_t FLASH_SIZE_BYTES = 32 * 1024 * 1024;
const DiscreteRotation imuRotation = DiscreteRotation::identity();
ConfigurationID_t desktopRequiredConfigs[] = {FIRMWARE_VERSION_c, BOARD_NAME_c};

/**
 * @brief One boot of a dual deploy flight computer, fed from a log file instead of sensors
 * @details Rebuilt on a simulated reboot - the FRAM and flash it's given persist across boots, like on a board.
 */
class DesktopAvionics {
public:
    DesktopAvionics(DebugStream& debug, DesktopSystemClock& clock, FramMemory& fram, FlashMemory& flash) :
        droguePyro(clock), mainPyro(clock), hardware(debug, clock, 100) {
        configuration.setDefault<BOARD_NAME_c>("Desktop");

        const int16_t framID = hardware.appendFramMemory(&fram);
        const int16_t flashID = hardware.appendFlashMemory(&flash);
        hardware.appendPyro(&droguePyro);
        hardware.appendPyro(&mainPyro);
        hardware.appendBarometer(&barometer);
        hardware.appendAccelerometer(&accelerometer);
        hardware.appendGyroscope(&gyroscope);
        hardware.setup();

        configuration.setup(&hardware, framID);
        stateEstimator1D.setup(&hardware, &configuration);
        orientationEstimator.setup(&hardware, &configuration);
        flightStateDeterminer.setup(&configuration);
        dualDeploy.setup(&configuration, &droguePyro, &mainPyro);
        logger.setup(&hardware, &cliParser, flashID, LOG_HEADER, HEADER_OUTPUT, printLog, &configuration);
    }

    void loop(const SimulationLogFile::Sample& sample) {
        RocketState_s state{};
        state.timestamp = hardware.enforceLoopTime();
        hardware.runAndReadAllHardware();
        barometer.inject(sample[1], 0, sample[0]);
        accelerometer.inject({sample[2], sample[3], sample[4]}, 0);
        gyroscope.inject({sample[5], sample[6], sample[7]}, 0);

        state.orientation = orientationEstimator.update(state.timestamp, flightStateDeterminer.getFlightState());
        state.state1D = stateEstimator1D.update(state.timestamp, flightStateDeterminer.getFlightState());
        state.flightState = flightStateDeterminer.update(state.timestamp, state.state1D);
        dualDeploy.update(state, flightStateDeterminer);

        if (state.flightState == PRE_FLIGHT) {
            if (flightStateDeterminer.isStateTransitionTick()) logger.disableContinuousLogging();
            logger.setLogDelay(5000);
        } else if (state.flightState == ASCENT) {
            if (flightStateDeterminer.isStateTransitionTick()) logger.logConfig(&configuration);
            logger.enableContinuousLogging();
        } else if (state.flightState == POST_FLIGHT) {
            logger.disableContinuousLogging();
            logger.setLogDelay(5000);
        } else {
            logger.enableContinuousLogging();
        }

        configuration.pushUpdatesToMemory();
        logger.log({
                state.timestamp.runtime_ms, barometer.getPressurePa(), barometer.getTemperatureK(),
                accelerometer.getAccelerationsMSS_sensor().x, accelerometer.getAccelerationsMSS_sensor().y, accelerometer.getAccelerationsMSS_sensor().z,
                gyroscope.getVelocitiesRadS_raw().x, gyroscope.getVelocitiesRadS_raw().y, gyroscope.getVelocitiesRadS_raw().z,
                state.state1D.altitudeM, state.state1D.velocityMS, state.state1D.accelerationMSS, state.state1D.unfilteredNoOffsetAltitudeM, state.flightState,
                state.state1D.barometerValid, droguePyro.isFired(), mainPyro.isFired(),
                state.orientation.tiltMagnitudeDeg
            });
    }

    void offload(DebugStream* output) {
        logger.offloadCallback(output);
    }

private:
    Barometer barometer;
    Accelerometer accelerometer{&imuRotation};
    Gyroscope gyroscope{&imuRotation};
    DesktopPyro droguePyro;
    DesktopPyro mainPyro;
    HardwareAbstraction hardware;
    Configuration configuration{{
        desktopRequiredConfigs,
        Configuration::REQUIRED_CONFIGS,
        FlightStateDeterminer::REQUIRED_CONFIGS,
        DualDeploy::REQUIRED_CONFIGS,
        StateEstimator1D::REQUIRED_CONFIGS,
        OrientationEstimator::REQUIRED_CONFIGS
    }};
    FlightStateDeterminer flightStateDeterminer;
    DualDeploy dualDeploy;
    StateEstimator1D stateEstimator1D;
    OrientationEstimator orientationEstimator;
    BasicLogger<DesktopLogData> logger;
    IntegratedParser cliParser;
};

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("usage: desktop <input log> <output log> [--reboot-at <sim ms>]...\n");
        return 1;
    }
    SimulationLogFile input;
    if (!input.load(argv[1])) {
        printf("No data rows in %s, or they span more than 12 hours\n", argv[1]);
        return 1;
    }
    std::vector<uint32_t> rebootTimesMs;
    for (int i = 3; i + 1 < argc; i += 2) {
        if (strcmp(argv[i], "--reboot-at") == 0) rebootTimesMs.push_back((uint32_t)strtoul(argv[i + 1], nullptr, 10));
    }
    std::sort(rebootTimesMs.begin(), rebootTimesMs.end());
    FILE* output = fopen(argv[2], "w");
    if (!output) {
        printf("Can't write %s\n", argv[2]);
        return 1;
    }

    DesktopDebug debug;
    DesktopSystemClock clock;
    VolatileConfigurationMemory<MAX_CONFIGURATION_LENGTH * 2> fram;
    DesktopFlash flash(FLASH_SIZE_BYTES);
    std::unique_ptr<DesktopAvionics> avionics(new DesktopAvionics(debug, clock, fram, flash));
    uint32_t bootTimeMs = 0;
    size_t nextReboot = 0;
    for (size_t i = 0; i < input.getSamples().size(); i++) {
        const uint32_t simTimeMs = (uint32_t)i * 10;
        if (nextReboot < rebootTimesMs.size() && simTimeMs >= rebootTimesMs[nextReboot]) {
            debug.message("Rebooting at %u ms", simTimeMs);
            bootTimeMs = simTimeMs;
            clock.setRuntimeMs(0);
            avionics.reset();
            avionics.reset(new DesktopAvionics(debug, clock, fram, flash));
            nextReboot++;
        }
        clock.setRuntimeMs(simTimeMs - bootTimeMs);
        avionics->loop(input.getSamples()[i]);
    }

    DesktopDebug outputStream(output);
    avionics->offload(&outputStream);
    fclose(output);
    return 0;
}
