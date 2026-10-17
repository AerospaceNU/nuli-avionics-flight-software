# Flight logic stress test: findings and fixes (2026-10-16, overnight)

`scripts/flight_logic_tests/stress_test.cpp` (build: `build_stress.sh`; run:
`./stress_test.exe FLIGHTS SEED SCENARIO`) flies Monte-Carlo rockets through the real `src/core`
flight code (StateEstimator1D, KalmanFilter1D, FlightStateDeterminer, Configuration on a fake FRAM)
plus the drogue/main rules copied from the builds' `loop()`. Physics at 1 kHz (trapezoid thrust,
transonic drag rise, ISA to 32 km, drogue/main descent rates), sensors at 100 Hz with transonic
static-port dip/spike, ejection-charge spikes, noise, 2 Pa steps, dropouts, +-16 g clipping, and in
`reboot` a reboot at a random time (new objects, same FRAM). Scenarios: `clean`, `mach`, `spikes`,
`faults`, `reboot`, `highalt` (Mach 2-3+, 15-50 km), `all`. Judges each flight against the truth:
FALSE_LAUNCH, NO_LAUNCH, EARLY/LATE/NO_APOGEE, NO/FAST_DROGUE, NO/LOW/EARLY_MAIN, EARLY/LATE/NO_LANDING.
`STRESS_TRACE=<s>` prints the estimator per 0.1 s, `STRESS_ALL=1` prints passing flights too,
`STRESS_GROUND_TEMP` / `STRESS_GROUND_ELEV` pin the launch site.

## Results

| scenario (seeds) | before (launch-gate tree) | after |
|---|---|---|
| clean (400) | 91 fail: 50 FALSE_LAUNCH, 41 EARLY_LANDING | 0 (1000 flights: 0) |
| mach (400) | 96: 51 FALSE_LAUNCH, 45 EARLY_LANDING | 0 (1000: 0) |
| spikes (400) | ~95 | 0 (1000: 0) |
| faults (400) | 94: 56 FALSE_LAUNCH, 38 EARLY_LANDING | 5 LATE_LANDING (1000: 10) |
| reboot (400) | most NO_MAIN / NO_DROGUE | 95 NO_LANDING (reboot under the main, see below), 2 sub-100 m hops |
| outage (400) | 268: 116 NO_MAIN, 246 EARLY_LANDING, 47 EARLY_APOGEE | 16 (14 LATE_LANDING) |
| highalt <= 32 km apogee (797, Mach 2-3.4) | (FALSE_LAUNCH as above) | 0 |
| highalt > 32 km (203) | | 58 - barometer out of range, see below |

Real logs (`run.sh`, BASELINE=HEAD): MBTA, Crocket, McGuire cert change state on the identical
ticks as HEAD; altitudes within 1 m. All 12 envs build (+~4 KB flash for the stratosphere math).

## Bugs found and fixed (firmware, uncommitted)

1. **False launch on the pad from the ground temperature** (`StateEstimator1D`). GROUND_TEMPERATURE
   starts at 273.15 K. The first ground reference filtered *altitudes computed at the old temperature*
   and switched the temperature at the same moment; altitude scales with T, so AGL stepped by
   elevation*dT/T (~97 m at 1200 m, 22 C) and the Kalman overshoot crossed 100 m: launch, drogue on
   the pad. ~12 % of random sites (elevation >~1200 m and warm). Fix: low-pass the *pressure*,
   convert with the temperature it's stored with, and compute the tick's altitude after the
   reference update.
2. **Landing declared ~150 m up after the main opens** (`FlightStateDeterminer::hasLanded`). The
   filter overshoots the deceleration and sits within 3 m for 3 s while the measurement catches up:
   POST_FLIGHT, logging stops, CLI opens mid-air. Fix: the raw barometric altitude (0.1 s low-pass)
   must also be steady. Cost: under very noisy baro (25-40 Pa) landing can be 15-25 s late.
3. **Kalman filter started with zero covariance** (`KalmanFilter1D` ctor, `P.setZero()`). It trusted
   its made-up initial state: after a mid-flight reboot it held the boot altitude for seconds while
   falling at 30+ m/s, so the UNKNOWN check called it the pad -> no main (181/400 reboots). Fix: P
   starts at 1000*I. Only affects the first ticks after boot.
4. **First failed barometer read at boot fed sea level into the filter** (`StateEstimator1D`). Then
   the real ~1 km a tick later read as +100 m/s; a board booting in UNKNOWN (powered off before
   POST_FLIGHT) went to ASCENT -> drogue on the ground. Fix: the filter doesn't run until a barometer
   has read valid once.
5. **Reboot near apogee -> no drogue/main** (UNKNOWN check). 1 s window, nearly no motion at apogee
   -> PRE_FLIGHT. Fix: if the Kalman altitude is > LAUNCH_ALTITUDE_THRESHOLD_M above the stored
   ground elevation, keep watching up to `UNKNOWN_STATE_HIGH_TIMER_MS` (10 s) for motion. Motion is
   still required, so a board powered up at a higher site stays in PRE_FLIGHT.
6. **Mach lockout delayed apogee 5 s after a reboot** (debounce counts from boot): drogue at 60 m/s,
   main at 28-73 m. Fix: when UNKNOWN resumes the flight with the window's low-passed velocity
   below the lockout speed, the lockout is cleared until the speed next exceeds it.
7. **Altitude above 11 km** (`Barometer::calculateAltitudeM`): troposphere-only formula read 27 km as
   ~23 km. Now isothermal above the tropopause (two layers; cleanup chose this over three ISA layers -
   124 m low at 27.4 km, 320 m low at 32 km, 5 lines shorter). Unchanged below the tropopause.
   Also `abs()` -> `std::fabs()` on the ground-reference change checks (host builds took the int one).
8. **Barometer outage in flight** (new `outage` scenario: every read fails for 1-15 s at a random
   time). The estimator fed the held last-good pressure to the filter as a measurement, pinning the
   altitude: early apogee on the way up, and under the drogue a "landing" 3 s in - POST_FLIGHT, no
   main. 400 flights: 268 failed (116 NO_MAIN, 246 EARLY_LANDING). Fix: no barometer update while
   invalid (the filter coasts - accelerometer in ascent, constant velocity under a chute), and
   hasLanded() skips invalid ticks (skips, not resets: with scattered failed reads a reset kept it
   from ever landing). Now 16, all landing late when the outage spans touchdown. The one-bad-read-
   per-second case (`run.sh glitch_in_flight`) now detects apogee on the same tick as a clean flight;
   HEAD never detected it.

## Real flight logs (`real_replay.exe`, `replay_all.sh`)

24 real flights from `flight_data_SOURCES_2026-10-17.md` (AltOS CSVs, ISS TARS, OSU's MS5607 log),
resampled to 100 Hz with 30 s of pad first, accel clipped at 16 g. Highlights:

- **HEAD false-launches on the pad on every high-site flight** (Intrepid III x2, Prometheus x2, PSAS
  L-12, Intrepid II - Spaceport America / ~1.3 km): ASCENT ~27 s before liftoff, drogue and main at
  ~92 m. That's bug 1 on real data. The working tree flies all of them.
- Transonic artifacts didn't cause a false apogee: Intrepid III (Mach 1.75, pressure rising 9 kPa
  during boost) apogee +1.6/+2.4 s, drogue at 13-15 m/s; Intrepid III test (pressure above pad
  pressure at Mach 1), Kairos (+-250 m chatter at burnout), Nomad SRAD (MS5607, Mach 1.5) all clean.
  Prometheus' 1.7 kPa main-ejection spike: main at 198.8 m.
- Where HEAD didn't false-launch, apogee/main/landing match HEAD within 0.1-0.2 s.
- SG1.2 sustainer: ground-received telemetry with the pressure frozen for 5+ s at a time - lands
  at 840 m under the drogue. A frozen-but-valid barometer isn't something the driver produces
  (failed reads are invalidated), so left alone (PENDING).
- Nomad EasyMini (baro-only - its "acceleration" is from its baro filter, +-130 m/s^2 through the
  supersonic part): apogee 5 s late - the velocity estimate stays > 150 m/s until ~2 s before
  apogee, then the Mach lockout's 5 s debounce. With a real accelerometer this didn't show up.

## Hardware (`logs/hardware/2026-10-17_stressfix`)

SeriousGooseV1Sim (COM39) and SillyGooseV2Sim (COM54), pyros open. `sim_send.py --to=S`, `--reset`,
`--from=S`, then offload (`state_changes.py`):
- normal MBTA sim: identical to before (DESCENT 20.67 s, drogue 21.68, main 83.90, landed 114.18,
  1 overrun); Crocket on V2 as before.
- reboot under the drogue (launch + 50 s, 913 m): UNKNOWN -> ASCENT in 1.0 s, DESCENT 0.26 s
  later, drogue, main at 196.5 m, landed.
- reboot ~1 s after the drogue fired (ejection brownout): same, drogue refired, main 196.5 m.
- reboot just before apogee detection (V2): UNKNOWN held 2.4 s (high above ground, not yet
  moving), then ASCENT -> DESCENT, drogue at 12 m/s, main 196.5 m, landed.
  HEAD goes PRE_FLIGHT in all three (no main / no pyros).

## Cleanup pass (2026-10-17 ~01:30)

Comments moved to `REVIEW_GUIDE_2026-10-17.md`. Two behaviour changes came with it, both re-tested
(all scenarios, 26 CI cases, real-log ticks): the UNKNOWN wait uses the highest valid altitude seen,
so a failed read during the wait at apogee no longer ends it (reboot failures 95 -> 93); and the
I2C `--i2cLockTest` Dev aid left the tree (`agent_work/patches/i2c_lock_test_aid.patch`).

## Limits found, not fixed

- **Reboot under the main** (< 200 m, 4-8 m/s): UNKNOWN often sees < 5 m / < 3 m/s in its window and
  picks PRE_FLIGHT. Both pyros already fired; only the end of the log and the POST_FLIGHT beeps are
  lost. Loosening the thresholds risks a false ASCENT on a noisy pad, so left alone.
- **Reboot below 100 m AGL** with no motion (tiny hops) still goes PRE_FLIGHT.
- **Apogee > ~32 km**: below the MS5607's 10 mbar range floor, and 2 Pa steps are ~50 m there:
  early/late apogee. The 90k ft (27 km) target is inside the range.
- harness drogue/main inflate instantly (harsher than real).
- A board booting UNKNOWN at a site > 100 m above its stored ground elevation (e.g. a fresh config,
  ground 0) waits 11 s in UNKNOWN instead of 1 s before settling in PRE_FLIGHT.
- Above ~48 km pressure drops under the 100 Pa plausibility floor and the filter coasts on the
  accelerometer; out of range anyway.

## Final numbers (final code, 2026-10-17 ~03:00, 3000 flights per scenario, seeds 100000+)

| scenario | failed | what |
|---|---|---|
| clean / mach / spikes | 0 / 0 / 0 | |
| faults (5-40 Pa noise, 0.1-5 % failed reads) | 22 | all LATE_LANDING |
| outage (1-15 s, random time) | 85 | 77 LATE_LANDING, 8 LOW_MAIN (~300 m hops, PENDING) |
| reboot (random time) | 653 | 651 NO_LANDING (reboot under the main), 12 sub-100 m hops, 2 NO_LAUNCH (reboot before launch detection, PENDING) |
| all | 9 | all LATE_LANDING |
| highalt <= 32 km (2414 flights, Mach 2-3.4) | 0 | |

Desktop build end to end (`ci_data/desktop_monte_carlo.py`, logger and offload included), 150 per
scenario: clean, mach, spikes, faults, outage, highalt 0 failed; reboot 1 (a hop whose apogee is
below MAIN_ELEVATION - main fires with the drogue, as designed).

Hardware with the final firmware: MBTA/Crocket sims identical to before; reboot during boost, under
the drogue, at apogee and just after the drogue all resume and fire both pyros; 8 s outage under the
drogue: main at 196.5 m; 27-31 km Mach 3 flights on the M0+ boards with 1 overrun in 20 min; radio CLI
fuzz 0 reboots; USB CLI fuzz 0 reboots after the echo fix (40/40 batches rebooted before); 140
reset-mid-config-write cycles, config intact; boot to first loop 374 ms (SeriousGoose).
