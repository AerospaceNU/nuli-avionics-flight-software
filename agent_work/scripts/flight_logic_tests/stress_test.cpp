// Monte-Carlo stress test of the flight logic against flights where the truth is known.
//
// A 1-D physics model (thrust curve, Mach-dependent drag, ISA atmosphere, drogue/main descent) is
// flown closed-loop against the real StateEstimator1D + FlightStateDeterminer and a copy of the
// deployment logic in builds/SeriousGoose.cpp loop() (drogue DROGUE_DELAY after DESCENT, main at
// MAIN_ELEVATION with a 200 ms debounce). Sensors are synthesized from the truth with the artifacts
// that break real flight computers: transonic static-pressure dips/spikes, ejection-charge pressure
// spikes, noise, quantization, dropouts, accelerometer saturation at +-16 g. A reboot can be injected
// mid-flight: the firmware objects are rebuilt from scratch with only the FRAM-backed config kept.
//
// Each flight is judged against the truth:
//   FALSE_LAUNCH   ASCENT before liftoff
//   NO_LAUNCH      flew > 150 m but never left PRE_FLIGHT
//   EARLY_APOGEE   DESCENT more than 1 s before true apogee (drogue at speed)
//   LATE_APOGEE    DESCENT more than 6 s after true apogee
//   NO_APOGEE      never reached DESCENT
//   NO_DROGUE / NO_MAIN / LOW_MAIN (main fired below 80 m true AGL) / EARLY_MAIN (above 400 m)
//   FAST_DROGUE    drogue fired at > 60 m/s true speed
//   EARLY_LANDING  POST_FLIGHT before touchdown
//
// usage: stress_test [FLIGHTS] [SEED] [SCENARIO]     SCENARIO: all|clean|mach|spikes|faults|reboot
// Prints one line per failing flight (with its seed, to replay with FLIGHTS=1 SEED=<seed>) and a summary.
#include "Avionics.h"
#include "core/HardwareAbstraction.h"
#include "core/transform/DiscreteRotation.h"
#include "core/configuration/Configuration.h"
#include "core/state_estimation/FlightStateDeterminer.h"
#include "core/state_estimation/StateEstimator1D.h"
#include "util/Timer.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

// ---------------- harness hardware ----------------
struct SilentDebug : DebugStream {
    std::string log; // kept for printing a failing flight's messages
    size_t write(const void* b, size_t n) override {
        if (log.size() < 20000) log.append((const char*)b, n);
        return n;
    }
};
struct TickClock : SystemClock { // set by the harness each tick, so enforceLoopTime() never spins
    uint64_t us = 0;
    uint32_t currentRuntimeMs() override { return (uint32_t)(us / 1000); }
    uint32_t currentRuntimeUs() override { return (uint32_t)us; }
};
struct PersistentFram : FramMemory { // unlike VolatileConfigurationMemory, honours the address
    uint8_t mem[4096]{};
    void write(uint32_t a, const uint8_t* b, uint32_t n) override { if (a + n <= sizeof mem) memcpy(mem + a, b, n); }
    void read(uint32_t a, uint8_t* b, uint32_t n) override { if (a + n <= sizeof mem) memcpy(b, mem + a, n); }
};

static const DiscreteRotation identityRotation = DiscreteRotation::identity();
static ConfigurationID_t harnessRequiredConfigs[] = {BOARD_NAME_c, DROGUE_DELAY_c, MAIN_ELEVATION_c};

// One boot of the flight computer. Rebuilt on a reboot; `fram` is shared across boots.
struct Firmware {
    SilentDebug debug;
    TickClock clock;
    Barometer barometer;
    Accelerometer accelerometer{&identityRotation};
    Gyroscope gyroscope{&identityRotation};
    HardwareAbstraction hardware{debug, clock, 100};
    Configuration configuration{{harnessRequiredConfigs, Configuration::REQUIRED_CONFIGS, FlightStateDeterminer::REQUIRED_CONFIGS, StateEstimator1D::REQUIRED_CONFIGS}};
    StateEstimator1D estimator;
    FlightStateDeterminer determiner;
    ConfigurationData<uint32_t> drogueDelay;
    ConfigurationData<float> mainElevation;
    uint8_t deployState = 0;   // loop()'s function-static - reset by a reboot, like on the board
    Debounce mainDeployDebounce{200};
    uint64_t tick = 0;

    explicit Firmware(PersistentFram& fram) {
        configuration.setDefault<BOARD_NAME_c>("StressTest");
        const int16_t framID = hardware.appendFramMemory(&fram);
        hardware.appendBarometer(&barometer);
        hardware.appendAccelerometer(&accelerometer);
        hardware.appendGyroscope(&gyroscope);
        hardware.setup();
        configuration.setup(&hardware, framID);
        estimator.setup(&hardware, &configuration);
        determiner.setup(&configuration);
        drogueDelay = configuration.getConfigurable<DROGUE_DELAY_c>();
        mainElevation = configuration.getConfigurable<MAIN_ELEVATION_c>();
    }

    struct Out { FlightState_e state; bool fireDrogue; bool fireMain; float altitude; float velocity; float acceleration; float rawAlt; float groundElev; float groundTemp; };

    Out step(float pressurePa, float tempK, const Vector3D_s& accel, const Vector3D_s& gyro) {
        clock.us = tick * 10000ULL;
        tick++;
        Timestamp_s ts = hardware.enforceLoopTime();
        hardware.runAndReadAllHardware();
        barometer.inject(tempK, 0, pressurePa);
        accelerometer.inject(accel, 0);
        gyroscope.inject(gyro, 0);
        const State1D_s s = estimator.update(ts, determiner.getFlightState());
        const FlightState_e fs = determiner.update(ts, s);
        Out o{fs, false, false, s.altitudeM, s.velocityMS, s.accelerationMSS, s.unfilteredNoOffsetAltitudeM,
              configuration.getConfigurable<GROUND_ELEVATION_c>().get(), configuration.getConfigurable<GROUND_TEMPERATURE_c>().get()};
        if (fs == DESCENT) { // same rules as loop()
            if (determiner.getStateTimer()->getTimeElapsed(ts.runtime_ms) > drogueDelay.get() && deployState == 0) {
                o.fireDrogue = true;
                deployState = 1;
            }
            if (mainDeployDebounce.check(s.altitudeM <= mainElevation.get(), ts.runtime_ms) && deployState == 1) {
                o.fireMain = true;
                deployState = 2;
            }
        }
        configuration.pushUpdatesToMemory();
        return o;
    }
};

// ---------------- physics ----------------
static double isaPressure(double h) { // ISA to 32 km: troposphere, isothermal layer, +1 K/km layer; Pa
    if (h < 11000) return 101325.0 * pow(1 - 2.25577e-5 * h, 5.25588);
    if (h < 20000) return 22632.1 * exp(-1.57688e-4 * (h - 11000));
    return 5474.89 * pow(1 + (h - 20000) / 216650.0, -34.1632);
}
static double isaTemp(double h) { return h < 11000 ? 288.15 - 0.0065 * h : h < 20000 ? 216.65 : 216.65 + 0.001 * (h - 20000); }
static double isaDensity(double h) { return isaPressure(h) / (287.05 * isaTemp(h)); }
static double soundSpeed(double h) { return sqrt(1.4 * 287.05 * isaTemp(h)); }
static double cdOfMach(double m, double cd0) { // transonic drag rise, peak ~2.2x at M 1.05
    if (m < 0.8) return cd0;
    if (m < 1.05) return cd0 * (1 + 1.2 * (m - 0.8) / 0.25);
    return cd0 * (2.2 - 0.6 * std::min(1.0, (m - 1.05) / 1.5));
}

struct FlightParams {
    double padWaitS, groundElevM, groundTempK, burnS, thrustN, massKg, propKg, cd0, areaM2;
    double machDip, machSpike;        // transonic static-port error: fractional dip at M~0.95, spike at M~1.05
    double baroNoisePa, accelNoise, baroDropoutRate, ejectionSpikePa;
    double drogueRate, mainRate;      // descent speeds under each chute
    int gravityAxis, gravitySign;     // which board axis points up
    double rebootAtS;                 // < 0: no reboot
    double outageAtS, outageS;        // barometer outage (every read fails) from outageAtS for outageS; outageS 0: none
    bool accelSaturates;
};

struct Truth { double liftoffS = -1, apogeeS = -1, apogeeM = 0, touchdownS = -1, maxSpeed = 0, maxMach = 0; };
struct Result {
    std::vector<std::string> fails;
    double launchS = -1, descentS = -1, drogueS = -1, drogueSpeed = 0, mainS = -1, mainAltM = 0, landedS = -1, fwApogeeM = -1e9;
    std::string debugLog;
};

static FILE* g_emitLog = nullptr; // "emit" mode: the sensor stream as a board log, for the desktop build / CI

static Result fly(const FlightParams& p, std::mt19937_64& rng, Truth& truth) {
    std::normal_distribution<double> n01(0, 1);
    std::uniform_real_distribution<double> u01(0, 1);
    auto fram = std::make_unique<PersistentFram>();
    auto fw = std::make_unique<Firmware>(*fram);
    Result r;

    double h = 0, v = 0, t = 0;              // height above pad, m; up velocity; time since start
    bool drogueOut = false, mainOut = false, landed = false, rebooted = false;
    double lastEjectionS = -100;
    const double dtPhys = 0.001;
    const double liftoffS = p.padWaitS;
    truth.liftoffS = liftoffS;
    const double maxT = liftoffS + 2500; // a drogue descent from ~10 km takes several minutes
    double propLeft = p.propKg;

    for (long tick = 0; t < maxT; tick++) {
        // physics at 1 kHz for this 10 ms tick
        double specificForce = 0;            // what an accelerometer reads along the rocket axis (m/s^2)
        for (int i = 0; i < 10; i++) {
            const double mass = p.massKg + propLeft;
            const bool burning = t >= liftoffS && t < liftoffS + p.burnS;
            double thrust = 0;
            if (burning) {
                const double tb = (t - liftoffS) / p.burnS; // trapezoid thrust curve
                thrust = p.thrustN * (tb < 0.1 ? tb / 0.1 : tb > 0.85 ? (1 - tb) / 0.15 : 1.0);
                propLeft = std::max(0.0, propLeft - p.propKg / p.burnS * dtPhys);
            }
            const double hAbs = p.groundElevM + h;
            const double mach = fabs(v) / soundSpeed(hAbs);
            double drag = 0.5 * isaDensity(hAbs) * v * v * cdOfMach(mach, p.cd0) * p.areaM2;
            if (drogueOut && !mainOut) drag = mass * 9.80665 * (v * v) / (p.drogueRate * p.drogueRate);
            if (mainOut) drag = mass * 9.80665 * (v * v) / (p.mainRate * p.mainRate);
            const double dragSigned = v > 0 ? -drag : drag;
            double a = (thrust + dragSigned) / mass - 9.80665;
            if (t < liftoffS || landed) a = 0; // on the pad / on the ground
            if (h <= 0 && v <= 0 && t > liftoffS + 1) { a = 0; v = 0; landed = true; } // under a chute a ~ 0 at touchdown, so test position/velocity only
            specificForce = (t < liftoffS || landed) ? 9.80665 : (thrust + dragSigned) / mass; // gravity not felt in free flight
            v += a * dtPhys;
            h = std::max(0.0, h + v * dtPhys);
            t += dtPhys;
            if (t >= liftoffS) {
                truth.maxSpeed = std::max(truth.maxSpeed, fabs(v));
                truth.maxMach = std::max(truth.maxMach, mach);
                if (h > truth.apogeeM) { truth.apogeeM = h; truth.apogeeS = t; }
            }
            if (landed && truth.touchdownS < 0) truth.touchdownS = t;
        }

        // sensors
        const double hAbs = p.groundElevM + h;
        const double mach = fabs(v) / soundSpeed(hAbs);
        double pressure = isaPressure(hAbs) * (p.groundTempK / 288.15 > 0 ? 1 : 1);
        // transonic static-port error, both going up through Mach 1 and coming back down through it
        pressure *= 1 + p.machDip * exp(-pow((mach - 0.95) / 0.04, 2)) + p.machSpike * exp(-pow((mach - 1.05) / 0.03, 2));
        if (t - lastEjectionS < 0.15) pressure += p.ejectionSpikePa * exp(-(t - lastEjectionS) / 0.04); // charge pressurizes the bay
        pressure += p.baroNoisePa * n01(rng);
        pressure = round(pressure / 2.0) * 2.0; // ~2 Pa steps
        if (u01(rng) < p.baroDropoutRate) pressure = 0; // a failed read - the driver would invalidate it
        if (p.outageS > 0 && t >= p.outageAtS && t < p.outageAtS + p.outageS) pressure = 0;
        double sf = specificForce + p.accelNoise * n01(rng);
        if (t - lastEjectionS < 0.05) sf += 300 * (u01(rng) - 0.5); // ejection shock
        if (p.accelSaturates) sf = std::max(-156.9, std::min(156.9, sf)); // +-16 g
        Vector3D_s accel{0.05 * n01(rng), 0.05 * n01(rng), 0.05 * n01(rng)};
        (p.gravityAxis == 0 ? accel.x : p.gravityAxis == 1 ? accel.y : accel.z) = p.gravitySign * sf;
        const Vector3D_s gyro{0.01 * n01(rng), 0.01 * n01(rng), 0.01 * n01(rng)};

        if (p.rebootAtS >= 0 && !rebooted && t >= p.rebootAtS) { // reboot: fresh objects, same FRAM
            r.debugLog += fw->debug.log + "\n--- REBOOT ---\n";
            fw = std::make_unique<Firmware>(*fram);
            rebooted = true;
        }

        const float sensorTempK = (float)(isaTemp(hAbs) + (p.groundTempK - 288.15));
        const bool hasFailedReads = p.baroDropoutRate > 0 || p.outageS > 0; // a sparse log would interpolate ramps into the 0 Pa rows
        const bool emitDense = hasFailedReads || (t >= liftoffS - 36 && (v > -5 || h < 600 || tick % 10 == 0)); // sparse pad and high descent, like a board's log - the sim interpolates
        if (g_emitLog && (emitDense || tick % 500 == 0)) std::fprintf(g_emitLog, "%lld\t%.1f\t%.2f\t%.4f\t%.4f\t%.4f\t%.5f\t%.5f\t%.5f\n", (long long)tick * 10, (float)pressure, sensorTempK, accel.x, accel.y, accel.z, gyro.x, gyro.y, gyro.z);
        const Firmware::Out o = fw->step((float)pressure, sensorTempK, accel, gyro);
        static const double traceS = getenv("STRESS_TRACE") ? atof(getenv("STRESS_TRACE")) : -1;
        if (traceS > 0 && t < traceS && tick % 10 == 0)
            std::printf("  t=%6.2f state=%d alt=%8.2f vel=%7.2f acc=%7.2f raw-ground=%7.2f ground=%8.2f Tg=%6.2f | true h=%5.1f sf=%.2f\n", t, (int)o.state, o.altitude,
                        o.velocity, o.acceleration, o.rawAlt - o.groundElev, o.groundElev, o.groundTemp, h, sf);
        if (o.state == ASCENT || o.state == DESCENT) r.fwApogeeM = std::max(r.fwApogeeM, (double)o.altitude);
        if (o.state == ASCENT && r.launchS < 0) r.launchS = t;
        if (o.state == DESCENT && r.descentS < 0) r.descentS = t;
        if (o.state == POST_FLIGHT && r.landedS < 0) r.landedS = t;
        if (o.fireDrogue && !drogueOut) { drogueOut = true; r.drogueS = t; r.drogueSpeed = fabs(v); lastEjectionS = t; }
        if (o.fireMain && !mainOut) { mainOut = true; r.mainS = t; r.mainAltM = h; lastEjectionS = t; }
        if (landed && t > truth.touchdownS + 30) break;
    }
    r.debugLog += fw->debug.log;

    // judge
    if (truth.touchdownS < 0) { r.fails.push_back("HARNESS_NO_TOUCHDOWN"); return r; } // sim too short - not a firmware result
    if (r.launchS >= 0 && r.launchS < liftoffS) r.fails.push_back("FALSE_LAUNCH");
    if (truth.apogeeM > 150 && r.launchS < 0) r.fails.push_back("NO_LAUNCH");
    if (r.launchS >= liftoffS) {
        if (r.descentS < 0) r.fails.push_back("NO_APOGEE");
        else if (r.descentS < truth.apogeeS - 1.0) r.fails.push_back("EARLY_APOGEE");
        else if (r.descentS > truth.apogeeS + 6.0) r.fails.push_back("LATE_APOGEE");
        if (r.drogueS < 0) r.fails.push_back("NO_DROGUE");
        else if (r.drogueSpeed > 60) r.fails.push_back("FAST_DROGUE");
        if (truth.apogeeM > 250) {
            if (r.mainS < 0) r.fails.push_back("NO_MAIN");
            else if (r.mainAltM < 80) r.fails.push_back("LOW_MAIN");
            else if (r.mainAltM > 400) r.fails.push_back("EARLY_MAIN");
        }
        if (r.landedS >= 0 && truth.touchdownS >= 0 && r.landedS < truth.touchdownS) r.fails.push_back("EARLY_LANDING");
        if (r.landedS < 0) r.fails.push_back("NO_LANDING"); // still in DESCENT 30 s after touchdown
        else if (r.landedS > truth.touchdownS + 15) r.fails.push_back("LATE_LANDING");
    }
    return r;
}

// ---------------- scenarios ----------------
static FlightParams randomFlight(std::mt19937_64& rng, const std::string& scenario) {
    std::uniform_real_distribution<double> u(0, 1);
    auto in = [&](double a, double b) { return a + (b - a) * u(rng); };
    FlightParams p{};
    p.padWaitS = in(20, 60);
    p.groundElevM = in(0, 1500);
    p.groundTempK = in(268, 313);
    p.massKg = in(1.5, 30);
    p.areaM2 = M_PI * pow(in(0.04, 0.16) / 2, 2);
    p.cd0 = in(0.35, 0.6);
    p.burnS = in(0.8, 6);
    // pick a thrust-to-weight that gives the scenario's speed range
    const bool fast = scenario == "mach" || (scenario == "all" && u(rng) < 0.4);
    // fast: amateur supersonic (up to ~Mach 2); otherwise subsonic
    if (fast) p.burnS = in(1, 4);
    p.thrustN = p.massKg * 9.80665 * (fast ? in(8, 25) : in(4, 12));
    p.propKg = p.thrustN * p.burnS / (9.80665 * in(180, 230)); // Isp ~200 s
    p.machDip = fast ? -in(0.0, 0.25) : 0;
    p.machSpike = fast ? in(0.0, 0.3) : 0;
    p.baroNoisePa = in(1, 8);
    p.accelNoise = in(0.05, 0.5);
    p.ejectionSpikePa = 0;
    p.baroDropoutRate = 0;
    p.drogueRate = in(18, 35);
    p.mainRate = in(4, 8);
    p.gravityAxis = (int)(u(rng) * 3) % 3;
    p.gravitySign = u(rng) < 0.5 ? 1 : -1;
    p.rebootAtS = -1;
    p.outageAtS = p.outageS = 0;
    p.accelSaturates = true;
    if (scenario == "spikes" || (scenario == "all" && u(rng) < 0.3)) {
        p.machDip = -in(0.05, 0.35);
        p.machSpike = in(0.05, 0.45);
        p.ejectionSpikePa = in(500, 8000);
        p.burnS = in(1, 4);
        p.thrustN = p.massKg * 9.80665 * in(10, 25);
        p.propKg = p.thrustN * p.burnS / (9.80665 * 200);
    }
    if (scenario == "faults" || (scenario == "all" && u(rng) < 0.3)) {
        p.baroNoisePa = in(5, 40);
        p.accelNoise = in(0.3, 2);
        p.baroDropoutRate = in(0.001, 0.05);
    }
    if (scenario == "highalt") { // Mach 2-3, 15-32 km apogee (the 90k ft class): big propellant fraction, long burn
        const double frac = in(0.42, 0.56), isp = in(205, 235);
        p.massKg = in(15, 60);
        p.areaM2 = M_PI * pow(in(0.10, 0.16) / 2, 2);
        p.cd0 = in(0.3, 0.45);
        p.burnS = in(4, 10);
        p.propKg = p.massKg * frac / (1 - frac);
        p.thrustN = p.propKg * 9.80665 * isp / p.burnS;
        p.machDip = -in(0.0, 0.3);
        p.machSpike = in(0.0, 0.4);
        p.ejectionSpikePa = in(0, 3000);
    }
    if (scenario == "clean") {
        p.baroNoisePa = 2; p.accelNoise = 0.1; p.machDip = p.machSpike = 0;
    }
    // For isolating a failure: pin the launch site
    if (getenv("STRESS_GROUND_TEMP")) p.groundTempK = atof(getenv("STRESS_GROUND_TEMP"));
    if (getenv("STRESS_GROUND_ELEV")) p.groundElevM = atof(getenv("STRESS_GROUND_ELEV"));
    return p;
}

static int emit(int argc, char** argv) { // emit SEED SCENARIO LOG_PATH: one flight's sensor stream + its truth as JSON
    const uint64_t seed = strtoull(argv[2], nullptr, 10);
    const std::string scenario = argv[3];
    std::mt19937_64 rng(seed);
    FlightParams p = randomFlight(rng, scenario);
    if (scenario == "reboot" || scenario == "outage") {
        std::mt19937_64 probe(seed);
        Truth t0;
        fly(p, probe, t0);
        std::uniform_real_distribution<double> u(0, 1);
        const double end = t0.touchdownS > 0 ? t0.touchdownS : t0.apogeeS + 60;
        if (scenario == "reboot") p.rebootAtS = t0.liftoffS + (end - t0.liftoffS) * u(rng);
        else { p.outageAtS = t0.liftoffS + (end - t0.liftoffS) * u(rng); p.outageS = 1 + 14 * u(rng); }
    }
    g_emitLog = std::fopen(argv[4], "w");
    std::fprintf(g_emitLog, "timestampMs\tpressurePa\tbarometerTemperatureK\taccelerationMSS_x\taccelerationMSS_y\taccelerationMSS_z\tvelocityRadS_x\tvelocityRadS_y\tvelocityRadS_z\n");
    Truth truth;
    const Result r = fly(p, rng, truth);
    std::fclose(g_emitLog);
    std::string fails;
    for (auto& x : r.fails) fails += (fails.empty() ? "" : ",") + x;
    std::printf("{\"seed\": %llu, \"scenario\": \"%s\", \"liftoff_s\": %.2f, \"apogee_s\": %.2f, \"apogee_m\": %.1f, \"touchdown_s\": %.2f, "
                "\"max_speed_ms\": %.1f, \"max_mach\": %.2f, \"ground_elevation_m\": %.1f, \"ground_temperature_k\": %.1f, \"reboot_at_s\": %.3f, "
                "\"outage_at_s\": %.3f, \"outage_s\": %.3f, \"harness_fails\": \"%s\"}\n",
                (unsigned long long)seed, scenario.c_str(), truth.liftoffS, truth.apogeeS, truth.apogeeM, truth.touchdownS, truth.maxSpeed, truth.maxMach,
                p.groundElevM, p.groundTempK, p.rebootAtS, p.outageAtS, p.outageS, fails.c_str());
    return 0;
}

int main(int argc, char** argv) {
    if (argc == 5 && std::string(argv[1]) == "emit") return emit(argc, argv);
    const int flights = argc > 1 ? atoi(argv[1]) : 200;
    const uint64_t seed0 = argc > 2 ? strtoull(argv[2], nullptr, 10) : 1;
    const std::string scenario = argc > 3 ? argv[3] : "all";
    const bool verbose = flights == 1;
    std::map<std::string, int> counts;
    int failed = 0;
    double worstEarly = 0, worstApogeeErr = 0;
    for (int f = 0; f < flights; f++) {
        const uint64_t seed = seed0 + (uint64_t)f;
        std::mt19937_64 rng(seed);
        FlightParams p = randomFlight(rng, scenario);
        Truth truth;
        if (scenario == "outage") { // the barometer stops reading for 1-15 s somewhere in the flight
            std::mt19937_64 probe(seed);
            Truth t0;
            fly(p, probe, t0);
            std::uniform_real_distribution<double> u(0, 1);
            const double end = t0.touchdownS > 0 ? t0.touchdownS : t0.apogeeS + 60;
            p.outageAtS = t0.liftoffS + (end - t0.liftoffS) * u(rng);
            p.outageS = 1 + 14 * u(rng);
        }
        if (scenario == "reboot") { // reboot somewhere between liftoff and well after landing
            // first fly once without a reboot to learn the timeline, then reboot inside it
            std::mt19937_64 probe(seed);
            Truth t0;
            fly(p, probe, t0);
            std::uniform_real_distribution<double> u(0, 1);
            const double end = t0.touchdownS > 0 ? t0.touchdownS : t0.apogeeS + 60;
            p.rebootAtS = t0.liftoffS + (end - t0.liftoffS) * u(rng);
        }
        Result r = fly(p, rng, truth);
        if (!r.fails.empty()) {
            failed++;
            for (auto& x : r.fails) counts[x]++;
            std::printf("FAIL seed=%llu [%s] apogee %.0f m at %.1f s (max %.0f m/s, M%.2f) | launch %.2f descent %.2f drogue %.2f@%.0fm/s main %.2f@%.0fm landed %.1f touchdown %.1f fw-apogee %.0f%s\n",
                        (unsigned long long)seed, [&] { std::string s; for (auto& x : r.fails) s += (s.empty() ? "" : ",") + x; return s; }().c_str(),
                        truth.apogeeM, truth.apogeeS - truth.liftoffS, truth.maxSpeed, truth.maxMach,
                        r.launchS - truth.liftoffS, r.descentS - truth.liftoffS, r.drogueS - truth.liftoffS, r.drogueSpeed,
                        r.mainS - truth.liftoffS, r.mainAltM, r.landedS - truth.liftoffS, truth.touchdownS - truth.liftoffS, r.fwApogeeM,
                        p.rebootAtS >= 0 ? (" reboot@" + std::to_string(p.rebootAtS - truth.liftoffS).substr(0, 6)).c_str() : "");
            if (verbose) std::printf("%s\n", r.debugLog.c_str());
        }
        if (r.descentS >= 0 && r.launchS >= 0) worstEarly = std::max(worstEarly, truth.apogeeS - r.descentS);
        if (r.launchS >= truth.liftoffS && truth.apogeeM > 100) worstApogeeErr = std::max(worstApogeeErr, fabs(r.fwApogeeM / truth.apogeeM - 1));
        if ((verbose || getenv("STRESS_ALL")) && r.fails.empty())
            std::printf("ok seed=%llu apogee %.0f m at %.1f s (max %.0f m/s, M%.2f) launch +%.2f descent +%.2f drogue +%.2f@%.0fm/s main +%.2f@%.0fm landed +%.1f (touchdown +%.1f) fw-apogee %.0f\n",
                        (unsigned long long)seed, truth.apogeeM, truth.apogeeS - truth.liftoffS, truth.maxSpeed, truth.maxMach,
                        r.launchS - truth.liftoffS, r.descentS - truth.liftoffS, r.drogueS - truth.liftoffS, r.drogueSpeed, r.mainS - truth.liftoffS, r.mainAltM,
                        r.landedS - truth.liftoffS, truth.touchdownS - truth.liftoffS, r.fwApogeeM);
    }
    std::printf("SUMMARY scenario=%s flights=%d failed=%d", scenario.c_str(), flights, failed);
    for (auto& kv : counts) std::printf(" %s=%d", kv.first.c_str(), kv.second);
    std::printf(" worst_early_apogee=%.2fs worst_reported_apogee_error=%.0f%%\n", worstEarly, worstApogeeErr * 100);
    return failed ? 1 : 0;
}
