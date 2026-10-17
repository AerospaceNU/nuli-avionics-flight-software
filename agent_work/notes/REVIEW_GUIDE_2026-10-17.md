# Review guide for the overnight firmware commits (2026-10-17)

## Commits on `patrick-dev`

| commit | what | review tip |
|---|---|---|
| 1. code | every code change of the session: sections 1-6 and 9 below | the careful review; comment lines in it are only ones attached to code it adds, removes or replaces |
| 2. comments | the comment pass (section 8) | `python -I agent_work/scripts/comment_only_check.py HEAD~2` from the branch tip must print "0 with code changes" |
| 3. agent_work | all tooling and notes of the session | no firmware |

Test evidence lives in `flight_STRESS_TEST_FIXES_2026-10-16.md` (harness, hardware) and in CI:
`ctest -R ^sim_` (40 flights, see `tests/sim/cases.json`). Against the pre-session flight code 16 of
the first 29 fail; the 5 transonic-reboot cases (section 10) also fail on the morning push; with this
commit all pass, and the three real logs in `agent_work/.../run.sh` change state on exactly the same
ticks as before. Section 10 is what changed after the morning push (Mach flag, review fixes, CI).

## 1. Flight logic

### `StateEstimator1D`
- **Ground reference filtered as pressure** (`updateGroundElevationReference`). Altitude scales with
  the temperature it's computed at. The old code low-passed *altitudes computed with the previous
  ground temperature* (273.15 K on a fresh config) and switched the temperature in the same tick, so
  AGL stepped by elevation x dT / T (~100 m at a 1200 m, 22 C site). With the Kalman overshoot that
  crossed the 100 m launch threshold: launch on the pad, drogue a few seconds later. Seen on every
  Spaceport-America real log replayed (`syn_hot_site_fresh_board`, `prometheus_*` fail on old code).
  `altitudeRawM` is now computed after the reference update so both use the same temperature.
- **No filter until the first valid barometer reading** (`m_barometerEverValid`). Before it, the held
  pressure is sea level; feeding that and then the real ~1 km a tick later read as +100 m/s, and a
  board booting UNKNOWN took it as flight.
- **Barometer outage = coast** (`accelerationDataUpdate` when invalid). The held pressure pinned the
  altitude: early apogee on the way up, a "landing" 3 s into an outage under the drogue (no main).
  Coasts on the accelerometer in ascent and at constant velocity under a chute (accel is 0 there).
- Warnings: "No valid barometer reading" after 10 ticks, "Barometer reading valid again" after.
  Options considered: two messages by flight state (was), one message (chosen), none.

### `FlightStateDeterminer`
- `hasLaunched`: no valid barometer, no launch (from the earlier launch-gate change). An invalid tick
  is skipped, not a debounce reset (section 10).
- `hasLanded`: the raw barometric altitude (0.1 s low-pass) must also be steady. When the main opens
  the filter overshoots the deceleration and sits still for seconds while the measurement catches
  up - that alone looked like a landing ~150 m up (POST_FLIGHT, logging off, CLI on, mid-air).
  Invalid-barometer ticks are skipped, not counted and not a reset (a reset never let a board with
  scattered failed reads land).
- UNKNOWN (boot after an unfinished flight):
  - waits for valid-barometer ticks: an invalid tick returns before the window or its timers advance;
  - `highAboveGround` = the highest valid altitude seen > LAUNCH_ALTITUDE_THRESHOLD_M: keep watching
    for motion up to UNKNOWN_STATE_HIGH_TIMER_MS (10 s) instead of deciding PRE_FLIGHT at 1 s. A
    reboot at apogee (ejection brownout) is nearly motionless for a second; it used to go PRE_FLIGHT
    and fire nothing. Motion is still required, so a board powered up at a higher site stays put.
  - `m_machLockoutClearedAtBoot`: the lockout debounce counts from boot, so a resumed flight waited
    5 s before apogee could be called (drogue at 60 m/s, main under 80 m). It's cleared when the
    stored state was DESCENT (the board already called apogee before the reboot), until the speed
    next exceeds the lockout speed; PRE_FLIGHT drops it. See section 10 for why not the window speed.
  - `resetUnknownStateDetection()` replaces the duplicated reset block.

### `KalmanFilter1D`
- `P` starts at 1000*I instead of 0. With 0 it trusted its made-up initial state: after a mid-flight
  reboot it held the boot altitude for seconds while falling at 30+ m/s (boot check: "on the pad").
  Only the first ticks after boot are affected; real logs are tick-identical.

### `Barometer::calculateAltitudeM`
- Isothermal layer above the tropopause (22632 Pa). Troposphere-only read 27 km as ~23 km.
  Options: three ISA layers (11-20 km isothermal, then +1 K/km) vs two (chosen). Two layers read
  124 m low at 27.4 km and 320 m low at 32 km - below the sensor's noise up there - for 5 fewer
  lines and one fewer branch. Identical below the tropopause.

### `DualDeploy` (new, `core/DualDeploy.h`)
- The drogue/main rules moved out of each build's `loop()` (they were function statics, so they
  couldn't be tested or shared). Same behaviour, same order of checks; `deployState` became two bools.
- `MAIN_DEPLOY_DEBOUNCE_TIMER_MS` (200) moved to `Avionics.h` with the other thresholds.
- Its REQUIRED_CONFIGS are already in every board's list; Configuration de-duplicates, so the stored
  ID list (and every board's FRAM config) is unchanged.

## 2. Drivers
- `Ms5607Mmc5603SensorPackage` + `I2cBusRecovery.h`: the I2C lock-up recovery from earlier in the
  session (`i2c_BUS_LOCKUP_RECOVERY_2026-10-16.md`). Warnings collapsed to one line each.
  The Dev-only `--i2cLockTest` aid was removed from the tree (it's only ever run by an agent); it's
  kept as `agent_work/patches/i2c_lock_test_aid.patch` (reference diff, includes cleanup context).
- `VolatileConfigurationMemory` honours its address (it ignored it - harmless until the desktop
  build used it for the real config layout).

## 3. Logging (`BasicLoggerHelper.h`, `BasicLogger.h`, builds)
- From earlier in the session: one struct definition generates the header and printer; boards send
  `LOG_FORMAT\t<definition>`. Comments reduced to doxygen; `auto&` in the builds became
  `const char*`; one `reinterpret_cast` and one `const auto&` became C-style / explicit types.
- Binary offload: only empty entries count toward the 4-strike end (same rule as text offload).

## 4. Desktop build (`builds/Desktop.cpp`, `drivers/desktop/*`) and CI
- `desktop <input log> <output log> [--reboot-at <sim ms>]...`. Input: any SillyGoose/SeriousGoose
  log or offload capture (first 9 columns, trimmed and interpolated exactly like SillyGooseTool's
  sim - `SimulationLogFile`). Output: the same text a board prints on `--offload` (loads in the tool).
- Mirrors SeriousGoose's flight path minus radio, GPS, aux, indicators and CLI: 1D estimator,
  orientation, FlightStateDeterminer, DualDeploy, logger state machine, real config in FRAM, NOR flash.
  Boots like a freshly flashed board (config defaults -> UNKNOWN), which is what exposed the
  ground-temperature bug on real logs.
- `DesktopAvionics` is a class rather than globals so `--reboot-at` can rebuild it while FRAM and
  flash persist - the physics-sim hookup later can drive the same `loop(sample)`.
- Deleted: FlightDataReader, CustomCsvParser, DummySystemClock, the real-time DesktopSystemClock,
  `AVIONICS_DESKTOP_MAIN`.
- CI: `.github/workflows/desktop-sim.yml` (same branches as cli-tests) -> `ctest -R ^sim_` -> one
  test per case in `tests/sim/cases.json`, checked by `tests/sim/check_sim.py`: windows for launch,
  apogee, drogue, main (+ altitude), landing on the input log's clock, plus invariants on every case
  (no pyro in PRE_FLIGHT, drogue 0.9-1.2 s after apogee, main after drogue, no backwards states).
- Data (`tests/sim/data`, 10.7 MB gz): 9 AerospaceNU SillyGoose logs (GPL-3 like this repo, incl. the
  real Haybales mid-descent reboot), 6 more AerospaceNU logs from FCB v0/v2 and a ground station plus
  the Haybales post-reboot log alone (converted by `agent_work/scripts/ci_data/convert_rocket_flight_data.py`),
  4 RocketPy logs (Prometheus x2, Hedy, Juno III; MIT, notice in `tests/sim/data/README.md`),
  20 synthetic weird flights (Mach 1-3, 27 and 30 km, hot high site, dead/dying barometer, outages,
  dropouts, reboots during boost, near apogee, under the drogue and in the transonic/supersonic climb,
  garbled log file). Regenerate with `agent_work/scripts/ci_data/make_ci_cases.py`.

## 5. USB CLI echo (`drivers/arduino/SerialDebug.h`)
- Release builds echo every received character. The echo was one `Serial.print(c)` per character
  inside the read loop, so a long line (~500+ characters, e.g. a paste or line noise) blocked the loop
  past the 100 ms watchdog and **rebooted the board** (fuzzed on SillyGoose V1: reboots at 498, 520,
  600, 800, 1000, 5000 characters; none with the echo off). Options: echo up to the buffer limit
  (still 499 prints), skip when the TX buffer is full (depends on the core's availableForWrite),
  buffer and cap (chosen): at most 256 characters per tick, echoed in one write.

## 6. Second commit: fixes in code the first commit doesn't touch
Found by a review pass over the untouched code; each verified by reading the code.
- **`IntegratedParser::runCli`: flags survived a failed parse.** `--fire -d -x -y` errors out but
  leaves `-d` set; the next `--fire -m` then fires the drogue too. Regression test
  `ParserTest.TestFailedParseLeavesNoFlagSet` fails on the old code. Now `resetFlags()` always runs.
- `StringHelper` `%s` copied into a 32-byte stack buffer bounded only by the output size - a
  BOARD_NAME of 32+ characters (settable over the CLI) overflowed the stack on config printing.
- `Configuration::setup` FRAM index check `<` -> `<=` (off by one).
- `ArgumentFlag` members value-initialized; init list in declaration order.
- `ReturnCodes`: two codes were both -14.
- `Parser::addFlagGroup`: `static_assert(n <= MAX_FLAGS)` (an oversized group indexed past the array).
- `DebugStream` float formatting prints nan/inf instead of converting them to an integer (UB).
- `SX1262Radio::setFrequency/setSpreadingFactor`: a failed set left the radio in standby, deaf,
  while its status said listening - it now restarts receive either way.
- `ConfigurationCliBinding`: include guard; a rejected `-set` now says "rejected, still set to".
- `OrientationEstimator`: BOARD_ORIENTATION_c listed in its REQUIRED_CONFIGS (already in every
  build's set via StateEstimator1D, so the stored ID list doesn't change); `int32_t` compare.
- `ArduinoWatchdog`: pet interval from the timeout the hardware actually applied.
- Typos/consts: garbled `///` on LAUNCH_ACCELERATION_THRESHOLD_MSS, "resting" -> "resetting",
  `const` casts in CRC.

## 7. Proposed, not applied (owner's call)
- `Parser::parse` leader handling duplicates the general loop and ignores the leader flag's parse
  result (~35 lines shorter as one loop; behaviour identical except leader parse errors surface).
- `SimpleFlag::parse` accepts and ignores an argument: `--fire -d -x` fires the drogue.
- `Timer.h`: `Alarm::isAlarmFinished` / FlashMemoryCommon timeouts aren't wrap-safe (49.7 days).
- Dead code: `core/cli/CLIEnums.h` (only listed in CMake), BaseFlag `m_dependency`, unused StopWatch /
  Alarm members, `Accelerometer::getFullScaleMSS`, `US_TIMER_*` macros, StateEstimatorBasic6D (never
  instantiated), and ~2700 lines of drivers in no build (`AprsModulation`, `ICM20602/ICM42605/ICM20948Sensor`,
  `MS5607Sensor`, `MS8607Sensor`, `RFM9xRadio`, `PayloadFlightData.h`, `drivers/samd21/*`).
- `IndicatorManager`: the on/off loop repeated 4x, `goto` + fallthrough in beepNums.
- `OrientationEstimator::update`: two lines duplicated in both branches; one shared motion debounce
  for all gyros.

## 8. Third commit: comment pass over all of src/
86 files, ~2350 comment lines removed, **no code changes**: `agent_work/scripts/comment_only_check.py
<base>` strips comments (respecting string/char literals) and compares every changed file - 0 differ.
Wrong comments were corrected rather than deleted where the information mattered (e.g. Barometer
"atmospheres", Magnetometer "gauss", MS5607Sensor's swapped D1/D2, the IndicatorManager siren
period, ArduinoPyro's continuity value). TODOs moved to PENDING.md. Worth a glance:
- `pinmaps/SeriousGoosePinmap.h`: five commented-out `PBxx`-style pin defines deleted (looked copied
  from another board).
- `drivers/arduino/ArduinoSystemClock.h`: the commented-out `+ 4274967` clock-wrap test offsets deleted.
- Left: OrientationEstimator debug prints (the only caller of quaternionToEuler), an alternative
  integration in StateEstimatorBasic6D, `samd21/APRSSinTables.h` (all commented-out tables; delete with
  AprsTransmitterOld.cpp if wanted), `builds/EByteTests/E22Test.ino` not touched.

## 9. Board orientation kept until a 1 g sample (`StateEstimator1D::updateBoardOrientationReference`)
A forced reference (every boot) recomputed the axis from the accelerometer low-pass even when it had
never accepted a sample (it only takes readings within 1 m/s^2 of 1 g), so a reboot under thrust set
the axis from an empty filter (NEG_Z) and overwrote the one stored in FRAM - the accel launch check
then missed the burn. Now nothing is updated until the low-pass has a sample (`LowPass::isInitialized`).
Stress `reboot` 3000 flights: NO_LAUNCH 2 -> 0; every other scenario identical; real logs same ticks.

## 10. After the morning push (2026-10-17 afternoon)
Firmware, all in `FlightStateDeterminer.cpp`:
- **Mach-lockout clear after a reboot = stored DESCENT.** The pushed version cleared it when the
  UNKNOWN window's velocity low-pass was below 150 m/s. Right after a reboot the filter starts
  uncertain (P = 1000 I) and takes the first transonic pressure samples at face value, so the low-pass
  read slow while the rocket was at Mach ~1: drogue 21-33 s before apogee at 250-330 m/s.
  Stress, 3000 flights each (`agent_work/logs/stress/*_reboot{mach,highalt,}.txt`):

  | clear rule | rebootmach (transonic reboot) | reboot (subsonic) |
  |---|---|---|
  | window velocity < 150 (pushed) | 276 fail (drogue at 250-330 m/s) | 0 FAST_DROGUE, 0 LOW_MAIN |
  | window altitude range (tried) | 14 fail | - |
  | never clear | 0 | 60 FAST_DROGUE (60-74 m/s), 51 LOW_MAIN |
  | **stored state was DESCENT (chosen)** | **0** | 22 FAST_DROGUE (61-68 m/s), 0 LOW_MAIN |

  The 22 are reboots in the ~0.5 s between true apogee and the board calling it (stored ASCENT): the
  5 s lockout then delays the drogue to ~5 s past apogee. Options if that matters: shorter lockout
  after a reboot when the window is slow *and* below a speed the transonic artifacts can't fake.
- **Launch gate skips invalid barometer ticks** instead of resetting the debounce (review H1). With
  20-30 % failed reads a reset never let the 0.5 s debounce finish, while PRE_FLIGHT kept re-learning
  the ground under the climbing rocket: main fired up to 3.5 km up, 7/40 launches never detected.
  Now 0/40 at 20 % and 30 %; `faults` identical.
- **UNKNOWN waits for a valid barometer** before its window/timers advance (review C2). The MS5607
  driver retries calibration every 100 failed ticks, one tick after the 1 s decision, so a barometer
  unreadable for >1 s after a reboot decided PRE_FLIGHT mid-flight: 40/40 no landing, 34 no main.
  Now 1/40 (an unrelated hop), also at 2 s; 5 s outage: 5/40.

CI and test data:
- 5 transonic/supersonic reboot cases (`syn_*_reboot_<time>`, reboot times from the sweep) - each
  fails on the pushed code (drogue 21-33 s early) and passes now. They share the base flight's data file.
- `syn_one_bad_read_per_second` had its 0 Pa rows in sparse stretches too, which the sim interpolated
  into valid-looking 79 -> 0 -> 79 kPa ramps (that was the sweep's "+32 s late drogue"); failed reads are
  now only placed in dense stretches.
- Raw SillyGoose logs go from a pad row every 5 s straight into boost; the sim interpolated a 2-5 s
  climb and launched 1.3-3.6 s early. A row holding the pad values 40 ms before fast logging starts is
  inserted (real-case windows moved by that amount).
- 6 cases from the rest of rocket-flight-data (FCB v0 x3, FCB v2, ground station, Haybales post-reboot
  log alone), converted by `convert_rocket_flight_data.py` (formats/units in its manifest).
- `check_sim.py`: an empty offload fails; `known_issue` cases report KNOWN and fail once they pass.
- `desktop-sim.yml`: `ctest --no-tests=error` (no Python -> no sim tests -> used to pass);
  `cli-tests.yml`: `-E "^sim_"` (it doesn't build the desktop target) - verified on GitHub on a
  temporary branch.

Not applied (review findings with data, owner's call) - see PENDING.md: KalmanFilter1D masked update
(H2), backup apogee timer for a dead I2C bus (C3), UNKNOWN on the pad at a high site (C4).
