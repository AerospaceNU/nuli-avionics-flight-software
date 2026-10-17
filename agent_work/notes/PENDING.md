# Still pending

Everything known-but-not-done, in one place. Anything fixed should be deleted from here, not
annotated.

## Firmware

### Updating a board from firmware older than `e25e4171` resets its whole config

`e25e4171` added `FIRST_STAGE_BURNOUT_c` to `FlightStateDeterminer::REQUIRED_CONFIGS`. That changes
the config-ID list, so `Configuration::hasError()` rejects the stored FRAM config ("invalid ID
list") and every value goes back to its default - including `GROUND_STATION_MODE`,
`DROGUE_DELAY`, `BUZZER_ENABLED` and calibrated values. Working as designed, but nothing warns the
person flashing. Seen on both SeriousGooses on 2026-10-16; record a board's settings before
updating it.

### PlatformIO's 1200-baud reset often doesn't reach the SillyGoose bootloader

`pio run -t upload` opens the port straight at 1200 baud; on the SillyGooses (old `c4d9423` and the
current build) that usually leaves the app running, and bossac's SAM-BA commands land in the CLI
as "Invalid message" errors. Opening at 115200 with DTR set, then dropping to 1200, works -
`scripts/hardware/touch.py` does that. The SeriousGooses take PlatformIO's touch fine.

### A board with a dead barometer can't fly

Since the launch gate (`flight_LAUNCH_GATE_2026-10-16.md`) a board without a valid barometer never
leaves PRE_FLIGHT - deliberately, acceleration-detected launches included, since apogee and the
pyro logic depend on barometric altitude. It warns on the debug stream ("No valid barometer
reading - launch detection held"), but the LED/buzzer don't say so: someone on the pad has no cue.
An indicator pattern for "not armed: no barometer" would close that.

### A board that thinks it's flying ignores USB

`cliParser.runCli()` only runs in PRE_FLIGHT and POST_FLIGHT, so a false launch can't be stopped or
diagnosed over USB - it looks hung (the loop is fine and feeds the watchdog). Consider a narrow
CLI path (status/reset only) in flight states, or leaving ASCENT on a timeout.

### The SAMD core's I2C waits are unbounded

`SERCOM::startTransmissionWIRE()`'s write-mode `while(!MB)` (no bus-error check) and
`readDataWIRE()`'s `while(SB == 0)` spin forever if a device holds SCL low mid-transfer. The bus
recovery can't help with that case; only the 100 ms watchdog gets out, and the board then needs the
boot-time clear (which handles SDA, not a permanently held SCL).

### Airbreaks logs the servo position in `auxState`

`SeriousGooseAirbreaks.cpp` writes `airbrakeServo.isAtPosition2() ? 2 : 1` into the struct's
`auxState`. Every host reads `auxState`/`auxFired` as the aux pyro channel, so the tool's Live Map
shows the aux badge as ARMED/UNARMED for servo deployed/stowed. Renaming the field (e.g.
`airbrakeServoPosition`) would make the tool stop treating it as a pyro without any tool change.


## Log printing (`BasicLoggerHelper.h`)

### Doubles aren't correctly rounded in the last digit

17 significant digits are computed with `(v - intPart) * scale` in double, so a double can print
1 ulp off (~1e-16 relative) and not read back bit-exact; one within a few ulps of `DBL_MAX` can
read back as inf. No log struct has a double today. Floats are exact.

### Text rows keep 9 decimal places below 1

Values below 1 print as fixed 9 decimals, so magnetometer readings (~5e-5 T) keep ~5 significant
digits and anything under 5e-10 prints as `0`. Binary offloads keep full precision.

## SillyGooseTool

### `CONFIG_FIELD_DEFS` doesn't know `FIRST_STAGE_BURNOUT`

The binary CONFIG record decoder (`02-protocol.js`) lays out fields from its own list, which ends at
`GROUND_STATION_MODE`. The new field sorts after it, so everything before still decodes; the new
field just never shows.

## Flight logic (from `flight_STRESS_TEST_FIXES_2026-10-16.md`)

### Reboot under the main resumes as PRE_FLIGHT

A reboot below ~200 m under the main sees too little motion in the 1 s UNKNOWN window, so the rest
of the descent isn't logged and POST_FLIGHT never beeps. Pyros already fired, so not a safety issue.

### `deployState` and `mainDeployDebounce` are function-statics in each build's loop()

They reset on reboot (intended: a resumed flight can fire again) but can't be reset or tested
without a reboot; the stress harness copies the logic instead of calling it.

### `VolatileConfigurationMemory` ignores its address argument

Found writing the harness's FRAM stand-in; harmless today.

### A frozen-but-valid barometer reads as a landing

If a barometer kept returning the same plausible pressure (never seen from the MS5607 driver, which
invalidates failed reads; seen in ISS SG1.2's ground telemetry), the altitude holds still and
hasLanded() fires under the drogue - no main. Detecting "identical pressure for N ticks in DESCENT"
would catch it; not done.

### Mach lockout can delay apogee up to 5 s when velocity reads high near apogee

`MACH_LOCKOUT_TIMER_MS` needs 5 s below 150 m/s. On a baro-only log (Nomad EasyMini) supersonic
pressure errors kept the velocity estimate high until ~2 s before apogee: apogee 5 s late.

### A mid-flight reboot with corrupt FRAM loses the ground elevation

Desktop test (MBTA, reboot under the drogue with 64 bytes of FRAM config overwritten): config CRC
fails, everything resets to defaults, GROUND_ELEVATION becomes 0. The flight still resumes (UNKNOWN
-> ASCENT -> DESCENT, drogue) but altitude is now above sea level: main fired at 94 m true AGL at a
106 m site, and at a site above ~200 m MSL it would never fire. Options: keep GROUND_ELEVATION (and
FLIGHT_STATE) in a second, separately-CRC'd FRAM slot; or on a CRC failure while UNKNOWN fall back to
a time-based main (drogue + N s). Rare (needs corruption and a reboot together), so not changed.

### CLI: a SimpleFlag silently takes a stray argument

`--fire -d -x` fires the drogue: `-x` isn't a known flag, so the parser hands it to `-d` as an argument
and SimpleFlag ignores it. Rejecting arguments on SimpleFlag (or unknown `-` tokens) would make a typo
fail instead. (The related bug - a *failed* parse leaving `-d` set for the next command - is fixed.)

### Proposals from the untouched-code review not applied

See `REVIEW_GUIDE_2026-10-17.md` section 7: Parser leader-loop rewrite, Timer wrap-safety, dead code
(~2700 lines of drivers in no build, CLIEnums.h, StateEstimatorBasic6D, unused Timer/BaseFlag members),
IndicatorManager duplication, OrientationEstimator's shared motion debounce.

## TODOs moved out of the code (comment pass, 2026-10-17; line numbers in be204e56)

- builds/SillyGoose.cpp:34-47 (the old list at the top of the file): barometer re-init / I2C bus lock
  (done tonight: bus recovery); disable write in flash driver; low pass with dt; drogue deployment
  failure detection; update firmware from website; intelligent log memory usage; configuration
  alignment on 64-bit; tune the Kalman Q; Mach lockout in the configuration; GUI waits for DIP switch;
  bootprot checking; burnout detection (done: first-stage burnout); watchdog in setup; USB PIDs so the
  web GUI can tell V1 from V2 and allow sim firmware.
- core/filters/KalmanFilter1D: figure out / tune Q (processJerkStd) - "tuning this correctly matters a LOT".
- core/generic_hardware/Barometer.h:85: a constant for the K/C conversion in calculateAbsoluteHumidity.
- core/state_estimation: gyro and accel full-scale switching (OrientationEstimator.cpp:31,
  StateEstimator1D.cpp:95), bad-reading detection; StateEstimatorBasic6D fixed-gain observer and
  complementary filter (Julia / Xiaofu).
- core/IndicatorManager.h:150: a simpler way to switch every indicator (expose the vector).
- core/cli: return the new argvPos from parse; rework the argc/argv/argvPos parse interface; disallow
  getValue on unset flags; formalize error reporting; a help that prints set values; more validation
  (duplicate leaders/flags, co-dependencies, conditional requirements); template vs runtime `n` in
  addFlagGroup (code size).
- core/configuration/Configuration.cpp:335: the defaults snapshot is a variable-length stack array.
- drivers/arduino/FlashMemoryCommon.cpp:22-40: where completion is waited for; when write-disable is
  called; standardize the API; check a register in setup() that the chip is there.
- drivers (unused builds): MS5607Sensor start at highest OSR; RFM9xRadio naming and busy-wait in
  transmit(); samd21 AprsTransmitter wait-for-completion; AprsTransmitterOld missing max-length check.

### A long barometer outage right after apogee on a low flight fires the main late

Stress `outage`, 3000 flights: 8 LOW_MAIN, all ~300 m apogees where a 1-15 s outage starts near
apogee. The filter coasts at its velocity (~0 as the drogue opens; the accelerometer isn't used in
DESCENT), so the altitude estimate stays near apogee and the main fires when the barometer returns
(0-73 m). Options: use the accelerometer while coasting in DESCENT, or fire the main on a time
fallback (drogue + time to fall from apogee at a typical drogue rate) when the barometer has been out.

### A reboot between liftoff and launch detection loses the flight (no pyros)

Update: the accelerometer half is fixed (board orientation is no longer overwritten from an empty
filter at boot, so a burn still in progress is detected - NO_LAUNCH 2 -> 0 of 3000). The barometric
half below remains: a reboot after burnout but before launch detection still re-learns the ground
from the climbing rocket.

Stress `reboot`, 3000 flights: 2 NO_LAUNCH, both rebooting ~0.4 s after liftoff - before ASCENT was
detected and saved, so the board boots PRE_FLIGHT. Its first ground reference (`m_needNewGroundReference`)
is the already-climbing rocket, the 0.01 low-pass keeps following it up, so AGL never reaches
LAUNCH_ALTITUDE_THRESHOLD_M, and a short motor has burnt out before the accel check sees 150 ms of
>= 20 m/s^2. An ignition brownout lands in exactly this window, so the real-world rate is likely
higher than 2/3000. Options (owner's call, each has a pad-safety side):
- At boot keep the FRAM ground elevation until the barometer has been steady for a few seconds
  (a board carried to a higher site would then read as high above ground: needs the motion check
  that UNKNOWN already has before launch can trigger).
- Treat "booted PRE_FLIGHT but moving fast" like UNKNOWN (re-use its motion test).
- Measured: SeriousGoose boots to its first loop in 374 ms (release build, `boot_capture.py`). So the
  window is liftoff to ~0.3 s, plus 0.37 s of boot; a motor still burning ~0.5 s after the reboot is
  caught by the accel check - short motors / late-burn brownouts are the exposure.

## From the adversarial review (`ADVERSARIAL_REVIEW_2026-10-17.md`; fixed: C1, C2, H1, two CI gaps)

### KalmanFilter1D masks the innovation but not K or P (H2, predates the session)
`genericUpdate` computes K from the full `P + R`, so every update also acts as a "velocity = current
estimate" measurement (R = 5) and shrinks P on axes not measured: the "coast" during an outage is
overconfident and P = 1000 at boot lasts one tick for velocity. Masking properly
(`M = mask.asDiagonal(); S = M P M + R; K = P M S^-1; P = (I - K M) P`) measured, 1000 flights each:
outage 39 -> 7 failures, reboot 228 -> 69 (NO_LANDING 220 -> 61), clean/mach/spikes/rebootmach 0 -> 0,
highalt same, **faults 0 -> 2 FALSE_LAUNCH**: a corrupt config boots UNKNOWN with ground 0 at a 720 m
site, and the now-undamped velocity (noisy barometer) trips UNKNOWN's 5 m / 3 m/s motion test on the
pad -> ASCENT -> drogue and main on the pad. So it's a retune, not a one-line fix: it needs UNKNOWN's
motion test (and probably processJerkStd / R) re-tuned with it. Not applied.

### A dead I2C bus in flight fires nothing (C3)
Barometer and IMU share the bus; the accelerometer has no validity flag, so its last boost sample stays
frozen and the coasting filter integrates it (fw apogee 166-967 km, 20/20 no pyros). With a working
accelerometer still 20/20 no main. Options: accelerometer validity (driver read failure -> invalid ->
filter predicts only), and a backup timer (drogue at burnout + expected time to apogee, main at
drogue + time to fall to MAIN_ELEVATION at a typical drogue rate) when the barometer has been out.

### UNKNOWN boot on the pad at a high site with motion (C4)
A board booting UNKNOWN (fresh/corrupt config, or an unfinished flight) more than 100 m above its
stored ground keeps watching for motion for 11 s (was 1 s). Carried down at >= 3 m/s in that window
(a car, a person walking downhill fast) it calls ASCENT -> apogee -> drogue. Still: never; motion
after 12 s: never. Options: require the motion to be a climb or fall faster than a person/vehicle
(e.g. 10 m/s), or require the stored state to have been ASCENT/DESCENT (not a fresh config) for the
10 s wait.

### Smaller review items, not changed
- M2: above the tropopause the temperature is the ground temperature's ISA offset; at a 313 K site
  27 km reads +2.1 km vs ISA (the 124 m figure in the guide holds at 288 K).
- M3 (suspected): an MS5607 temperature ADC read of 0 (conversion not finished) gives ~44.7 kPa and
  passes as valid; the 600 us wait equals the datasheet maximum.
- M4: SleepyDog rounds the 100 ms watchdog down to 62.5 ms - the timing budget in the notes is 62.5 ms.
- M1 CI gaps left: main_altitude checks the firmware's own estimate; reboot cases skip the state-order
  invariants and the drogue-delay check; the desktop clock is sample index x 10 ms (a log not on a 10 ms
  grid plays fast - convert_rocket_flight_data.py resamples); board logs can't mark invalid reads.
- L2: one non-finite barometer temperature (pressure valid) poisons the ground-temperature low-pass
  until reboot (MS5607 temperatures are integers, so only reachable from the desktop/log path).
- L3: the orientation guard leaves accelerometer launch detection off on a fresh config until a 1 g sample.
- L4/L5: SAMD Wire has no timeouts (a slave holding SCL low hangs until the watchdog); `recoverBus()`
  runs every tick forever if calibration or the magnetometer is dead.
- L6/L7/L9: binary offload of a garbage tail walks all of flash; float printing UB for |x| >= 2^64;
  the desktop build relies on transitive includes.
