# Adversarial review of `422116f9` (flight logic, drivers, CLI, desktop sim, CI) - 2026-10-17

Scope: `git diff 4131044b 422116f9` (firmware, CMake, CI, tests). The working tree differs from `422116f9`
only by the comment pass (`54a767bc`); line numbers below are for the current tree (`3acaad00`).
Nothing tracked was edited. All experiments were run from copies in the session scratchpad.

Severity scale: **critical** = can fire a pyro at the wrong time or fail to fire one; **high**; **medium**; **low**.
"Confirmed" = reproduced; "suspected" = code reasoning only.

## How the evidence was produced

- `st.exe`: `agent_work/scripts/flight_logic_tests/stress_test.cpp` (working-tree version, which has the
  `rebootmach` / `reboothighalt` scenarios) built with the same flags as `build_stress.sh` against the current `src/core`.
- Variants built against **copies** of `src` (via `git archive`), each a one-line change:
  - `old`: the parent `4131044b` flight code.
  - `noclear`: current code with `FlightStateDeterminer.cpp:71` changed to `m_machLockoutClearedAtBoot = false;`.
  - `fix`: current code, the clear additionally requires `max |velocity|` over the UNKNOWN window < 150 m/s and
    `max - min altitude` < 150 m (new member `m_unknownStateMaxAbsVelocity`, reset in `resetUnknownStateDetection()`).
  - `acc`: current code with `UNKNOWN_FLIGHT_STATE` added to the condition in `StateEstimator1D::getAccelerationMSS`.
  - `skip`: current code with `m_launchDebounce.reset();` removed from `hasLaunched()` (invalid ticks skipped, not a reset).
- `st_rb.exe`: the harness plus the patch at the end of this file (scenarios `rebootbaro`, `boostout`, env `DROP`, `FREEZE`, `OUT`).
- `car.cpp`: a 30-line driver around the harness's `Firmware` struct (at the end of this file).
- `desk.exe`: `builds/Desktop.cpp` built with host g++ (builds cleanly with `-Wall -Wextra`, only library warnings).

---

## Critical

### C1. Mach lockout cleared from a barometer-only velocity -> drogue at Mach ~1 after a reboot in transonic/supersonic flight (confirmed, regression)

`FlightStateDeterminer.cpp:71` (`m_machLockoutClearedAtBoot = lowpass velocity < MACH_LOCKOUT_THRESHOLD_MS`), together with
`StateEstimator1D.cpp:92` (no accelerometer in UNKNOWN) and `KalmanFilter1D.cpp:20` (P0 = 1000 I).

The author knew this hole exists. What it costs, and the fact that both suggested remedies work, had not been measured:

| scenario (300 flights, seed 1) | old `4131044b` | current | `noclear` | `fix` (max\|v\| + span) | `acc` (accel in UNKNOWN) |
|---|---|---|---|---|---|
| `rebootmach`: FAST_DROGUE | 0 | **31** (drogue at 246-332 m/s) | 0 | 0 | 32 |
| `reboothighalt`, apogee <= 32 km: FAST_DROGUE | 1 | **25** (up to 309 m/s) | 0 | 5 | 27 |
| `reboot` (600, seed 100000): failures | - | 129 (all NO_LANDING / 2 NO_DROGUE) | 152 (+12 FAST_DROGUE at 60-74 m/s, +7 LOW_MAIN 30-68 m) | 129 (same as current) | 132 |

Mechanism (trace: `STRESS_TRACE=200 ./st.exe 1 157 rebootmach`). The board reboots at Mach 1.1. P0 = 1000, so the first
barometer samples are fully trusted, and the transonic static-pressure error (raw altitude -1810 m in one tick)
drives the Kalman velocity to **-976 m/s** inside the 1 s window. The window's low-passed velocity is "below
150", so the lockout is cleared, and the rocket goes ASCENT -> DESCENT within 0.2 s. The drogue fires at 307 m/s. Feeding the
accelerometer in UNKNOWN (`acc`) doesn't help, because the barometer artifact dominates. `desk.exe` on the same flight shows
the same thing (`ASCENT at 1000 ms, velocity -941.75`). That run happened to re-lock because the velocity went back above 150.

There is a second, independent path to the same result. `m_machLockDebounce` is primed with `check(true, 0)` in
`setup()` and is never called during UNKNOWN. So if UNKNOWN lasts >= 5 s (now possible, see C4), the first ASCENT tick
with v < 150 passes the lockout immediately, even if the clear were removed.

Fix: remove the clear (`noclear`: 0 supersonic drogues, at the cost of 60-74 m/s drogues after near-apogee reboots), or
use `fix`. `fix` keeps every benefit of the current code in `reboot` (identical 129), removes all `rebootmach` failures, and
cuts highalt from 25 to 5. That isn't sufficient alone, so add a check that is independent of the barometer. Also restart
`m_machLockDebounce` (`reset()`) on the UNKNOWN -> ASCENT transition so that time spent in UNKNOWN doesn't count.
Add a supersonic-reboot case to CI (see M1).

### C2. No valid barometer reading in the first 1 s after a reboot -> PRE_FLIGHT mid-flight -> no main (confirmed)

`FlightStateDeterminer.cpp:65-75`: `m_unknownStateTimer.check(true, ...)` runs on every tick, but max/min altitude
and velocity only accumulate on valid ticks. With zero valid ticks, `highAboveGround` is false (max = -1e9), so the
decision is **PRE_FLIGHT**. The board then re-learns the ground at its current altitude (`m_needNewGroundReference`), and
nothing fires for the rest of the descent.

This lines up exactly with the driver. If the MS5607 PROM read fails at boot (an ejection brownout is the reboot cause the author
gives), `recoverBus()` retries the calibration on recoveries 1, 101, 201, ...
(`Ms5607Mmc5603SensorPackage.cpp:401`, `m_busRecoveries % 100 == 1`). If the first retry fails, the next one is on
tick 101, which is one tick *after* the 1.0 s UNKNOWN decision.

```
OUT=0.95 ./st_rb.exe 40 500 rebootbaro  -> failed=1 (a 110 m hop, unrelated)
OUT=1.05 ./st_rb.exe 40 500 rebootbaro  -> failed=40 NO_LANDING=40 NO_MAIN=34
```
(Reboot under the drogue; the barometer reads invalid for OUT s after the reboot.)

Fix: start the UNKNOWN window at the first valid barometer tick, or require at least N valid ticks before deciding.
With no valid barometer at all, stay UNKNOWN (siren) rather than choosing PRE_FLIGHT. Also make the
recalibration retry cadence much shorter than the UNKNOWN window (e.g. every 10 recoveries).

### C3. Permanent barometer/I2C loss in flight -> drogue never fires (shared bus freezes the accelerometer); main never fires even with a good accelerometer (confirmed for the new code; the comparison with old is unverified)

`StateEstimator1D.cpp:52-56` coasts on `accelerationDataUpdate`. The accelerometer has no validity flag, and
`Ms5607Mmc5603SensorPackage::read()` (`:441`) leaves the last IMU sample injected when its transfer fails. The barometer,
IMU and magnetometer share one I2C bus, so a dead bus freezes the boost acceleration and the filter integrates it forever.
In DESCENT, `getAccelerationMSS` returns 0, so with the barometer gone the altitude coasts at the apogee velocity (~0) and never
reaches MAIN_ELEVATION. Nothing provides a time-based backup.

```
OUT=3000 ./st_rb.exe 20 900 boostout            -> 20/20 fail: NO_MAIN=20, EARLY_APOGEE=6 (accelerometer still working)
FREEZE=1 OUT=3000 ./st_rb.exe 20 900 boostout   -> 20/20 fail: NO_APOGEE=20 NO_DROGUE=20 NO_MAIN=20 (fw-apogee 166-967 km)
FREEZE=1 OUT=10 ./st_rb.exe 30 900 boostout     -> FAST_DROGUE 66/77 m/s, LOW_MAIN 35 m
```
The old code isn't comparable in the harness (its driver injected garbage, the harness injects 0 Pa). "Barometer outage =
coast" is only safe for short outages.

Fix: give `Accelerometer` a valid flag (invalidate on a failed IMU read), and predict-only when both sensors are invalid. Add a
backup: drogue at max(burnout + expected coast, launch + T_max) when the barometer has been invalid for > N s in ASCENT, and
main on a time-since-apogee estimate or a ballistic/drogue-rate dead reckoning in DESCENT. At minimum, log and beep.

### C4. The 11 s UNKNOWN wait fires the drogue on the ground when the board is moved vertically (confirmed)

`FlightStateDeterminer.cpp:68-73` (highAboveGround -> wait up to `UNKNOWN_STATE_HIGH_TIMER_MS`) combined with the lockout clear
(C1) and the 2 m / 150 ms apogee test. A board boots UNKNOWN on a fresh or reset config (`FLIGHT_STATE` defaults to
UNKNOWN, `ConfigurationRegistry.h:49`) or after an unfinished flight. If that happens at a site > 100 m above the stored ground
(default 0 m) and the board moves down at >= 3 m/s for ~2 s at any time in the first 11 s (vehicle on a grade, lift),
it goes UNKNOWN -> ASCENT -> DESCENT and **fires the drogue**.

```
car_cur.exe -4 0.2 1200 : t=2.21 ASCENT, t=2.83 DESCENT, t=3.84 DROGUE FIRED (true height change -14.6 m)
car_cur.exe -4 8 1200   : t=10.44 ASCENT, t=11.03 DESCENT, t=12.04 DROGUE FIRED
car_cur.exe -4 12 1200  : PRE_FLIGHT at 11.00, nothing fired;   car_cur.exe 0 0 1200: nothing fired
car_cur.exe -4 0.2 50   : (low site, 1 s window) PRE_FLIGHT, nothing fired
```
(Arguments: vertical speed, start of motion in s after power-on, site elevation. `car_old.exe` fires even standing still,
which is the ground-temperature bug the commit fixed, so this is a narrowed hole, not a regression.) "Motion is still required, so a board
powered up at a higher site stays put" holds only while the board is not moved.

Fix: in the extended window require a flight-like speed (e.g. |v| > 10-15 m/s), never clear the lockout from it, and
consider not allowing UNKNOWN -> ASCENT -> DESCENT to fire anything until a minimum descent rate or altitude drop (e.g. > 50 m) is
confirmed.

---

## High

### H1. Launch gate resets the launch debounce on every invalid barometer tick -> launch detected seconds late -> main fired 1-3.5 km up (confirmed)

`FlightStateDeterminer.cpp:85-89`. 15 consecutive valid ticks are needed, and while launch is undetected, PRE_FLIGHT keeps
re-learning the ground reference under the climbing rocket (`StateEstimator1D.cpp:38`, every 2 s, Kalman re-initialized).
So late detection turns into a wrong AGL zero.

```
DROP=0.2 ./st_rb.exe 40 700 mach  -> 4 fail: EARLY_MAIN at 1050 / 1691 m AGL, NO_LAUNCH
DROP=0.3 ./st_rb.exe 40 700 mach  -> 26 fail: EARLY_MAIN=19 (main up to 3529 m), NO_LAUNCH=7
skip variant, DROP=0.2 / 0.3      -> 0 / 0 fail;  faults 150 flights: identical to current (1 LATE_LANDING)
```
A 20-30 % failed-read rate is far beyond what the harness's `faults` scenario (<= 5 %) exercises. But the failure is a main at
3.5 km, and the fix is one line: skip invalid ticks (as `hasLanded()` already does) instead of resetting. Also consider not
moving the ground reference when |Kalman velocity| or acceleration is large.

### H2. `KalmanFilter1D::genericUpdate` applies the mask to the innovation only: K and P are computed as if every state were measured (confirmed, pre-existing, but the new logic depends on it)

`KalmanFilter1D.cpp:82-95`. `S = P + R` always includes `R_pito` (velocity) and, in `accelerationDataUpdate`, `R_baro`.
Velocity is therefore pseudo-measured as "equal to its current estimate" on every tick, and during a barometer outage the position
is too. Measured with a 20-line driver (`kf.cpp`, filter only):

```
P after predict (P0 = 1000 I): pp=1000.1 vv=1000.1
P after 1st position+accel update: pp=0.999 vv=4.975        <- velocity was not measured
steady state: pp=0.0130; after 5 s of accel-only (barometer outage) updates: pp=0.0123     <- should grow
```
So "P starts at 1000 I" only lasts one tick for velocity. During an outage the filter becomes *more* certain while blind, and when the
barometer returns its gain is ~0.012/tick, so re-convergence after a long outage takes seconds. The outage "coast" (C3) and the
outage LOW_MAIN / LATE_LANDING results in the author's own tables rest on this.

Fix: update only the measured rows: a sequential scalar update per measured component, or build H/R from the mask.
Re-tune afterwards; the filter's current behaviour is partly a product of this bug.

---

## Medium

### M1. CI (`tests/sim/check_sim.py`, `desktop-sim.yml`) cannot catch several of the failures above (confirmed by reading; partly run)
- `main_altitude` (`check_sim.py:80`) is the **firmware's own** `altitudeM`. Main fires when that estimate is <= MAIN_ELEVATION, so the
  140-210 m window is nearly tautological: an estimator offset (H1, C3) passes. For synthetic cases, check the true AGL
  (it can be recomputed from the input pressure and the case's ground elevation).
- Reboot cases skip the transition invariants and the drogue-delay check (`:91`, `:115`), and `events()` keeps only the
  **first** occurrence of each event. A post-reboot premature DESCENT/drogue is invisible once a pre-reboot event exists
  (e.g. `syn_reboot_under_drogue`: any post-reboot drogue timing passes).
- No case reboots in transonic/supersonic flight (the reboot cases are all `syn_subsonic`-based), so C1 is not covered. The
  synthetic cases have no `drogue` window, and drogue speed is never checked.
- The desktop sim's clock is `sample index x 10 ms` (`Desktop.cpp:161`). Input rows 10-30 ms apart are not re-timed
  (`SimulationLogFile`), so jittery or 20 ms logs are time-compressed relative to the "input log's own clock" the windows assume.
- Board logs record the held pressure on a failed read and have no validity column, so the real-log cases never exercise the
  invalid-barometer paths (only the synthetic 0 Pa cases do).
- A case expecting only `null`s (`syn_dead_barometer`, `syn_barometer_dies_on_pad`) passes on an empty or broken offload.
- If CMake doesn't find Python3, no `sim_` tests are registered and `ctest -R "^sim_"` reports "No tests were found" and exits 0
  (ctest default; add `--no-tests=error`). The workflow only runs for `main` / `not-flight-tested`, not `patrick-dev`.

### M2. `Barometer::calculateAltitudeM` derives the stratosphere temperature from the ground temperature (confirmed numerically)
`Barometer.h:46-51`. The isothermal layer uses `T0 + L*h_trop`, so a warm sensor makes the stratosphere warm. Against ISA, at a
1200 m site: T0 = 288 K reads 27 km as -111 m; **T0 = 313 K (40 C die temperature) reads it as +2105 m**, and 323 K as +2996 m (troposphere
part alone: +845 m at 11 km, which is pre-existing). The "124 m low at 27.4 km" claim holds only at T0 = 288.15 K. Velocity scales
by the same ~8 % (the mach-ramp and lockout thresholds compare against it). Fix: fixed 216.65 K above the tropopause (and
arguably a fixed tropopause pressure/height).

### M3. MS5607 ADC result 0 (conversion not finished) becomes a plausible "valid" reading (suspected; computed)
`Ms5607Mmc5603SensorPackage.cpp:295-308`, `:443-445`. The MS5607 returns 0 when the ADC is read before the conversion completes.
`TEMP_CONV_DELAY_US = 600` equals the datasheet maximum for OSR 256. With the datasheet example coefficients, `rawTemperature = 0`
gives TEMP = -251 C and **P = 44 708 Pa**, which passes `isPlausiblePressure`: a single-tick ~6 km altitude spike plus a 22 K
temperature sample into the ground-temperature low-pass. (A zero pressure ADC gives a negative P and is rejected.) Fix:
treat a raw ADC value of 0 as a failed read, and add ~10 % margin to the conversion waits.

### M4. The "100 ms watchdog" is really 62.5 ms (confirmed from the library source)
Adafruit SleepyDog `WatchdogSAMD::enable(100)`: 100 ms -> 102 cycles -> rounds *down* to 64 cycles = 62.5 ms (it returns 63).
The commit's `ArduinoWatchdog::enable` now stores the real value, but the notes' timing arguments (USB echo "past the 100 ms
watchdog", I2C recovery budget, worst-case tick) assume 100 ms. Re-check the worst-case tick (logConfig on the ASCENT
transition tick, offload-free paths, recovery with a calibration retry: delay(3) + PROM reads) against 62 ms.

---

## Low

- **L1. DualDeploy equivalence: no defect found.** In all three builds the old `deployState`/static-debounce code and
  `DualDeploy::update` evaluate the same conditions in the same order (debounce checked every DESCENT tick before the
  drogue flag; drogue and main can fire on the same tick). Desktop calls `update()` every tick, but it returns unless in DESCENT.
  The config ID list is sorted (`sortConfigs()`) and the three IDs were already in every board's list, so the FRAM layout is unchanged.
- **L2. Ground-temperature low-pass is never reset** (`StateEstimator1D.cpp:121`, `reset()` resets only the pressure filter).
  One non-finite temperature sample poisons it for the session. `set()` rejects NaN (range check), so the ground reference silently
  freezes until reboot.
- **L3. Board orientation guard** (`StateEstimator1D.cpp:147`): on a fresh config (`ERROR_AXIS_DIRECTION`), if no sample is ever
  within 1 m/s^2 of g (scale error, vibration on the pad), the accelerometer launch criterion is silently disabled and only the
  100 m altitude criterion remains, which H1's ground re-learning can defeat. Warn or beep when the orientation is unset in PRE_FLIGHT.
- **L4. I2C hangs aren't covered by the recovery**: SAMD `SERCOM::startTransmissionWIRE/readDataWIRE` spin without timeout on
  MB/SB. A slave holding SCL low hangs the loop until the watchdog (63 ms, or 4 s during setup) and causes a reboot loop. The
  recovery only acts on transfers that *return* an error. Consider SERCOM `INACTOUT`/`LOWTOUTEN`.
- **L5. `recoverBus()` runs on every tick forever** if the PROM never reads or the magnetometer is dead
  (`!othersOk || !m_baroCalibrated`). Each run does Wire.end/begin, ~0.2-1.4 ms, plus delay(3) every 100th. A dead magnetometer
  (not used in flight) costs the barometer nothing, but it permanently suppresses "I2C reads working again" and inflates the tick.
- **L6. Binary offload** (`BasicLogger.h:257-261`): CRC-failed entries are no longer counted and `failCount` never resets (matches
  text offload). A garbage, non-0xFF tail now walks all `m_numEntries` (~220 k on 32 MB). Slow, not unsafe.
- **L7. `DebugStream::format_float`**: NaN and inf are handled, but `(unsigned long long)val` is still UB for finite values >= 2^64.
- **L8. Not a defect: CLI fixes**. `IntegratedParser` reset after a failed parse, `StringHelper %s` bound,
  `ArgumentFlag` init, `ReturnCodes`, `static_assert` on the group size and the SX1262 restart-receive-on-failure are all correct as
  far as read. `SerialDebug` echo cap: one 256-byte stack buffer and one `Serial.write`, no defect found. `BasicLoggerHelper`:
  memcpy reads of packed fields, sign extension, buffer bounds (float <= 21 chars, double <= 23) and the
  `#__VA_ARGS__` stringification (comments stripped, `remove_struct_padding` left unexpanded and ignored after `}`) all check out.
- **L9. Desktop build** compiles cleanly with host g++ `-std=gnu++20 -Wall -Wextra` (warnings only in libraries and
  unused-parameter). `Desktop.cpp` relies on transitive includes for `<algorithm>`/`<cstring>`/`<cstdio>`, which is fragile on other
  toolchains.

---

## Reproduction

Build (from the scratch dir; `FW` = the firmware dir, or a `git archive` copy for a variant):
```
export PATH=/c/Users/patri/AppData/Local/Programs/CLion/bin/mingw/bin:$PATH
L=$FW/lib; g++ -std=gnu++17 -O2 -w -I $FW/src -I $L/Quaternion -I $L/Eigen -I "$L/Embedded Template Library/include" \
  <harness.cpp> $(find $FW/src/core -name '*.cpp') $L/Quaternion/Quaternion.cpp -o <out.exe>
```
Runs: `st.exe 300 1 rebootmach`, `st.exe 300 1 reboothighalt`, `st.exe 600 100000 reboot` (and the same with the variant builds);
`OUT=1.05 st_rb.exe 40 500 rebootbaro`; `DROP=0.3 st_rb.exe 40 700 mach`; `FREEZE=1 OUT=3000 st_rb.exe 20 900 boostout`.

Harness patch (`st_rb.cpp` = `stress_test.cpp` + this):
```diff
@@ fly(): after the accelerometer vector is built
+        { // shared-bus failure: the IMU read fails too during the barometer outage, so the driver keeps the last accelerometer sample
+            static Vector3D_s heldAccel{0, 0, 0};
+            const bool out = p.outageS > 0 && t >= p.outageAtS && t < p.outageAtS + p.outageS;
+            if (getenv("FREEZE") && out) accel = heldAccel; else heldAccel = accel;
+        }
@@ randomFlight(): before STRESS_GROUND_TEMP
+    if (getenv("DROP")) p.baroDropoutRate = atof(getenv("DROP"));
@@ main(): scenario -> base flight
-        ... scenario == "reboothighalt" ? "highalt" : scenario);
+        ... scenario == "reboothighalt" ? "highalt" : scenario == "rebootbaro" ? "clean" : scenario == "boostout" ? "mach" : scenario);
@@ main(): before the "reboot" scenario block
+        if (scenario == "rebootbaro") { // reboot under the drogue; barometer unreadable for OUT s after it
+            std::mt19937_64 probe(seed); Truth t0; fly(p, probe, t0);
+            std::uniform_real_distribution<double> u(0, 1);
+            const double end = t0.touchdownS > 0 ? t0.touchdownS : t0.apogeeS + 60;
+            p.rebootAtS = t0.apogeeS + 3 + (end - t0.apogeeS - 3) * 0.6 * u(rng);
+            p.outageAtS = p.rebootAtS; p.outageS = getenv("OUT") ? atof(getenv("OUT")) : 1.05;
+        }
+        if (scenario == "boostout") { // whole I2C bus fails mid-boost for OUT s
+            std::mt19937_64 probe(seed); Truth t0; fly(p, probe, t0);
+            p.outageAtS = t0.liftoffS + p.burnS * 0.5; p.outageS = getenv("OUT") ? atof(getenv("OUT")) : 3;
+        }
```

`car.cpp` (C4):
```cpp
#define main stress_main
#include "st_rb.cpp"
#undef main
int main(int argc, char** argv) { // VZ m/s vertical from START s after power-on, at SITE m; fresh config -> UNKNOWN, ground 0
    const double vz = argc > 1 ? atof(argv[1]) : -4.0, start = argc > 2 ? atof(argv[2]) : 3.0, site = argc > 3 ? atof(argv[3]) : 1200;
    PersistentFram fram; Firmware fw(fram); std::mt19937_64 rng(1); std::normal_distribution<double> n(0, 1);
    double h = site; int last = -1;
    for (int tick = 0; tick < 6000; tick++) {
        const double t = tick * 0.01; if (t > start) h += vz * 0.01;
        const float p = (float)(round((isaPressure(h) + 2 * n(rng)) / 2) * 2);
        Vector3D_s a{0.05f * (float)n(rng), 0.05f * (float)n(rng), 9.80665f + 0.3f * (float)n(rng)};
        auto o = fw.step(p, 300.0f, a, {0, 0, 0});
        if (o.state != last) { printf("t=%6.2f state %d alt=%.1f vel=%.2f\n", t, o.state, o.altitude, o.velocity); last = o.state; }
        if (o.fireDrogue) printf("t=%6.2f DROGUE FIRED (true height change %.1f m)\n", t, h - site);
        if (o.fireMain) printf("t=%6.2f MAIN FIRED\n", t);
    }
}
```

Caveat: the harness's transonic model (static-pressure dip up to -35 %, spike up to +45 %) is harsher than most real data. The
C1 comparison is still valid, because the old code and `noclear` pass the same model.
