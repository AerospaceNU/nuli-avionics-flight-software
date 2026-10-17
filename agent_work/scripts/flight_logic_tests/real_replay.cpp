// Replays a real flight log through the same firmware stack as stress_test.cpp (StateEstimator1D,
// FlightStateDeterminer, deploy rules from loop()), to see what our board would have done on it.
// Input: a CSV from to_replay.py - "t_s,pressure_pa,axial_specific_force_mss[,ref_height_m]", any
// rate, t = 0 at launch. It's resampled to 100 Hz (averaged when the log is faster, linearly
// interpolated when slower), given 30 s of pad time first so the ground reference settles, and the
// axial accel is clipped to +-16 g like our IMU.
// Prints the firmware's events relative to the log's t = 0, next to the log's own apogee (from
// ref_height if given, else the highest 1 s median of baro altitude).
// usage: real_replay.exe FILE.csv [--reboot-at=S] [--trace]
#define main stress_test_main
#include "stress_test.cpp"
#undef main

#include <fstream>
#include <sstream>

static const char* stateName(int s) {
    static const char* n[] = {"PRE_FLIGHT", "ASCENT", "DESCENT", "POST_FLIGHT", "UNKNOWN"};
    return s >= 0 && s < 5 ? n[s] : "?";
}

int main(int argc, char** argv) {
    if (argc < 2) { std::printf("usage: real_replay FILE.csv [--reboot-at=S] [--trace]\n"); return 2; }
    double rebootAt = NAN;
    bool trace = false;
    for (int i = 2; i < argc; i++) {
        if (!strncmp(argv[i], "--reboot-at=", 12)) rebootAt = atof(argv[i] + 12);
        if (!strcmp(argv[i], "--trace")) trace = true;
    }
    struct Row { double t, p, a, ref; };
    std::vector<Row> rows;
    std::ifstream in(argv[1]);
    bool haveRef = false;
    for (std::string line; std::getline(in, line);) {
        std::vector<double> v;
        std::stringstream ss(line);
        for (std::string c; std::getline(ss, c, ',');) {
            try { v.push_back(std::stod(c)); } catch (...) { v.clear(); break; }
        }
        if (v.size() < 3 || !std::isfinite(v[0]) || !std::isfinite(v[1]) || !std::isfinite(v[2])) continue;
        rows.push_back({v[0], v[1], v[2], v.size() > 3 ? v[3] : NAN});
        if (v.size() > 3 && std::isfinite(v[3])) haveRef = true;
    }
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.t < b.t; });
    if (rows.size() < 10) { std::printf("too few rows\n"); return 2; }

    // 100 Hz resample over [first, last]
    const double t0 = rows.front().t, t1 = rows.back().t;
    std::vector<Row> s;
    size_t j = 0;
    for (double t = t0; t <= t1; t += 0.01) {
        while (j + 1 < rows.size() && rows[j + 1].t < t) j++;
        // average every row in this tick if there are several, else interpolate
        double sp = 0, sa = 0; int n = 0;
        for (size_t k = j; k < rows.size() && rows[k].t < t + 0.01; k++) if (rows[k].t >= t) { sp += rows[k].p; sa += rows[k].a; n++; }
        Row r{t, 0, 0, NAN};
        if (n > 0) { r.p = sp / n; r.a = sa / n; }
        else {
            const Row& a = rows[j];
            const Row& b = rows[std::min(j + 1, rows.size() - 1)];
            const double f = b.t > a.t ? std::min(1.0, std::max(0.0, (t - a.t) / (b.t - a.t))) : 0;
            r.p = a.p + (b.p - a.p) * f;
            r.a = a.a + (b.a - a.a) * f;
        }
        s.push_back(r);
    }
    // pad: 30 s at the first second's median pressure, 1 g
    std::vector<double> first;
    for (auto& r : rows) if (r.t < t0 + 0.3) first.push_back(r.p);
    std::sort(first.begin(), first.end());
    const double padP = first[first.size() / 2];
    const int padTicks = 3000;

    // the log's own apogee
    double logApogeeT = 0, logApogeeH = -1e9;
    if (haveRef) {
        for (auto& r : rows) if (std::isfinite(r.ref) && r.ref > logApogeeH) { logApogeeH = r.ref; logApogeeT = r.t; }
    } else {
        for (size_t i = 50; i + 50 < s.size(); i += 10) {
            std::vector<double> w;
            for (size_t k = i - 50; k < i + 50; k++) w.push_back(s[k].p);
            std::nth_element(w.begin(), w.begin() + 50, w.end());
            const double h = Barometer::calculateAltitudeM((float)w[50], 288.15f) - Barometer::calculateAltitudeM((float)padP, 288.15f);
            if (h > logApogeeH) { logApogeeH = h; logApogeeT = s[i].t; }
        }
    }

    PersistentFram fram;
    auto fw = std::make_unique<Firmware>(fram);
    int lastState = -1;
    bool rebooted = false, drogue = false, mainFired = false;
    double maxAlt = -1e9, maxAltT = 0;
    std::printf("log: %zu rows, %.1f..%.1f s, pad %.0f Pa; log apogee %.0f m at %.2f s (%s)\n", rows.size(), t0, t1, padP, logApogeeH,
                logApogeeT, haveRef ? "log's own height" : "1 s median baro");
    const size_t total = padTicks + s.size();
    for (size_t i = 0; i < total; i++) {
        const bool onPad = i < (size_t)padTicks;
        const double t = onPad ? t0 - (padTicks - (double)i) * 0.01 : s[i - padTicks].t;
        const double p = onPad ? padP : s[i - padTicks].p;
        double a = onPad ? 9.80665 : s[i - padTicks].a;
        a = std::max(-156.9, std::min(156.9, a));
        if (!rebooted && std::isfinite(rebootAt) && t >= rebootAt) {
            fw = std::make_unique<Firmware>(fram);
            rebooted = true;
            std::printf("  t=%7.2f REBOOT\n", t);
        }
        const Firmware::Out o = fw->step((float)p, 293.15f, Vector3D_s{0, 0, a}, Vector3D_s{0, 0, 0});
        if (o.state != lastState) {
            std::printf("  t=%7.2f %-11s alt=%8.1f vel=%7.1f\n", t, lastState < 0 ? "(boot)" : stateName(o.state), o.altitude, o.velocity);
            lastState = o.state;
        }
        if (o.state == ASCENT || o.state == DESCENT) if (o.altitude > maxAlt) { maxAlt = o.altitude; maxAltT = t; }
        if (o.fireDrogue && !drogue) { drogue = true; std::printf("  t=%7.2f DROGUE      alt=%8.1f vel=%7.1f  (%+.2f s from log apogee)\n", t, o.altitude, o.velocity, t - logApogeeT); }
        if (o.fireMain && !mainFired) { mainFired = true; std::printf("  t=%7.2f MAIN        alt=%8.1f vel=%7.1f\n", t, o.altitude, o.velocity); }
        if (trace && !onPad && i % 10 == 0) std::printf("    %7.2f st=%d alt=%8.1f vel=%7.1f acc=%7.1f p=%.0f a=%.1f\n", t, (int)o.state, o.altitude, o.velocity, o.acceleration, p, a);
    }
    std::printf("firmware max altitude %.0f m at %.2f s; end state %s; drogue %s, main %s\n", maxAlt, maxAltT, stateName(lastState), drogue ? "fired" : "NOT fired",
                mainFired ? "fired" : "NOT fired");
    return 0;
}
