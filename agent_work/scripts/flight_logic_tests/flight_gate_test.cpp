// Host test of the launch gate: drives the real StateEstimator1D + FlightStateDeterminer with a
// flight log's sensor samples (100 Hz, from make_samples.py) and a scenario that breaks the
// barometer, the way a Sim build is fed. Prints every flight-state change, then PASS/FAIL against the
// scenario's expectation. One scenario per run, so every global starts fresh.
// usage: flight_gate_test SAMPLES_FILE SCENARIO
//   normal                - unmodified log: must reach ASCENT, DESCENT and POST_FLIGHT
//   dead_baro             - pressure 0 from the start (an uncalibrated MS5607): must never leave PRE_FLIGHT
//   baro_dies_on_pad      - valid for 10 s, then 0: must never leave PRE_FLIGHT
//   dead_baro_unknown_boot - booted as if mid-flight (UNKNOWN), pressure 0: must settle in PRE_FLIGHT, never ASCENT
//   glitch_in_flight      - one 0 Pa tick per second after launch: the flight must still complete
#include "Avionics.h"
#include "drivers/desktop/DesktopDebug.h"
#include "drivers/desktop/DesktopSystemClock.h"
#include "core/HardwareAbstraction.h"
#include "core/transform/DiscreteRotation.h"
#include "core/configuration/Configuration.h"
#include "core/state_estimation/FlightStateDeterminer.h"
#include "core/state_estimation/StateEstimator1D.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

DesktopSystemClock desktopClock;
DesktopDebug debug(nullptr);
Barometer barometer;
const DiscreteRotation imuRotation = DiscreteRotation::identity();
Accelerometer accelerometer(&imuRotation);
Gyroscope gyroscope(&imuRotation);
VolatileConfigurationMemory<1000> fram;
HardwareAbstraction hardware(debug, desktopClock, 100);
FlightStateDeterminer flightStateDeterminer;
StateEstimator1D stateEstimator1D;
ConfigurationID_t testRequiredConfigs[] = {BOARD_NAME_c};
Configuration configuration({testRequiredConfigs, Configuration::REQUIRED_CONFIGS, FlightStateDeterminer::REQUIRED_CONFIGS, StateEstimator1D::REQUIRED_CONFIGS});

static const char* stateName(int s) {
    switch (s) {
        case PRE_FLIGHT: return "PRE_FLIGHT";
        case ASCENT: return "ASCENT";
        case DESCENT: return "DESCENT";
        case POST_FLIGHT: return "POST_FLIGHT";
        default: return "UNKNOWN";
    }
}

int main(int argc, char** argv) {
    if (argc < 3) { std::printf("usage: flight_gate_test SAMPLES SCENARIO\n"); return 2; }
    const std::string scenario = argv[2];

    // pressurePa, tempK, accel x,y,z, gyro x,y,z per line - same order a Sim build reads
    std::vector<std::vector<float>> samples;
    std::ifstream in(argv[1]);
    for (std::string line; std::getline(in, line);) {
        std::vector<float> v;
        std::stringstream ss(line);
        for (std::string cell; std::getline(ss, cell, ',');) v.push_back(std::stof(cell));
        if (v.size() == 8) samples.push_back(v);
    }

    configuration.setDefault<BOARD_NAME_c>("FlightGateTest");
    configuration.setDefault<FLIGHT_STATE_c>(scenario == "dead_baro_unknown_boot" ? ASCENT : PRE_FLIGHT);
    const int16_t framID = hardware.appendFramMemory(&fram);
    hardware.appendBarometer(&barometer);
    hardware.appendAccelerometer(&accelerometer);
    hardware.appendGyroscope(&gyroscope);
    hardware.setup();
    configuration.setup(&hardware, framID);
    stateEstimator1D.setup(&hardware, &configuration);
    flightStateDeterminer.setup(&configuration);

    int lastState = -1;
    bool reached[5] = {};
    long launchTick = -1;
    for (size_t i = 0; i < samples.size(); i++) {
        RocketState_s state{};
        desktopClock.setRuntimeMs((uint32_t)i * 10);
        state.timestamp = hardware.enforceLoopTime();
        hardware.runAndReadAllHardware();

        float pressure = samples[i][0];
        if (scenario == "dead_baro" || scenario == "dead_baro_unknown_boot") pressure = 0;
        if (scenario == "baro_dies_on_pad" && i >= 1000) pressure = 0;
        if (scenario == "glitch_in_flight" && launchTick >= 0 && (i - launchTick) % 100 == 50) pressure = 0;

        barometer.inject(samples[i][1], 0, pressure);
        accelerometer.inject({samples[i][2], samples[i][3], samples[i][4]}, 0);
        gyroscope.inject({samples[i][5], samples[i][6], samples[i][7]}, 0);

        state.state1D = stateEstimator1D.update(state.timestamp, flightStateDeterminer.getFlightState());
        state.flightState = flightStateDeterminer.update(state.timestamp, state.state1D);
        if (state.flightState != lastState) {
#ifdef BASELINE_BUILD // the old State1D_s has no barometerValid
            const int baroValid = -1;
#else
            const int baroValid = state.state1D.barometerValid;
#endif
            std::printf("  t=%7.2f s  %-11s -> %-11s alt=%8.1f m  baroValid=%d\n", i / 100.0, lastState < 0 ? "(boot)" : stateName(lastState),
                        stateName(state.flightState), state.state1D.altitudeM, baroValid);
            lastState = state.flightState;
            if (state.flightState == ASCENT && launchTick < 0) launchTick = (long)i;
        }
        if (state.flightState >= 0 && state.flightState < 5) reached[state.flightState] = true;
    }

    bool pass;
    if (scenario == "normal" || scenario == "glitch_in_flight") {
        pass = reached[ASCENT] && reached[DESCENT] && reached[POST_FLIGHT];
    } else if (scenario == "dead_baro_unknown_boot") {
        pass = !reached[ASCENT] && !reached[DESCENT] && lastState == PRE_FLIGHT;
    } else {
        pass = !reached[ASCENT] && !reached[DESCENT] && !reached[POST_FLIGHT];
    }
    std::printf("%s: %s (%zu samples)\n", scenario.c_str(), pass ? "PASS" : "FAIL", samples.size());
    return pass ? 0 : 1;
}
