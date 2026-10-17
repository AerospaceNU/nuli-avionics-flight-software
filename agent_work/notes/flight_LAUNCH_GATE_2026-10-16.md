# Launch gate: no barometer, no launch (2026-10-16)

Follow-up to `i2c_BUS_LOCKUP_RECOVERY_2026-10-16.md`: the bus recovery fixes a held I2C bus, but a
barometer that is really dead still never produced a reading, the estimator kept seeing 0 Pa
(~45 km), and the flight logic called it a launch.

## Change

- `Barometer` (`core/generic_hardware/Barometer.h`): `validReading()` now means something - false until
  a plausible pressure (finite, 100 Pa - 200 kPa) is injected, and after `invalidate()`. The MS5607
  driver invalidates it on any failed or uncalibrated read; the old `MS5607Sensor`/`MS8607Sensor`
  drivers (unused today) set it too, since they write `m_pressurePa` directly.
- `StateEstimator1D` already skipped invalid barometers and fell back to the last good pressure
  (sea level before the first) - that path was dead because `validReading()` always returned true.
  It now reports `State1D_s::barometerValid`, stops learning the ground reference from a bad
  barometer, and warns once an outage lasts 10 ticks ("launch detection held" on the pad, "holding
  the last good pressure" in flight), plus "valid again" when it recovers.
- `FlightStateDeterminer`: `hasLaunched()` returns false without a valid barometer - acceleration
  included, deliberately, since apogee/pyro logic needs barometric altitude - and the boot-time
  UNKNOWN check settles in PRE_FLIGHT instead of accumulating bad altitudes.

Normal flights are unchanged: same state changes on the same ticks.

## Tests

Host (`scripts/flight_logic_tests/run.sh`, real core sources, 100 Hz replay of three flight logs):

| scenario | before (HEAD `bf5d1342`) | after |
|---|---|---|
| normal | flies | flies - identical state changes and altitudes |
| dead barometer (0 Pa) | "launch" at 0.15 s at 42 km, DESCENT, POST_FLIGHT | stays PRE_FLIGHT, one warning |
| barometer dies on the pad | launches | stays PRE_FLIGHT |
| dead barometer, rebooted mid-flight (UNKNOWN) | PRE_FLIGHT -> "launch" at 1.2 s | PRE_FLIGHT |
| one 0 Pa tick per second in flight | **never detects apogee** (no drogue) | flies, identical to normal |

The last row was a second bug: a single bad barometer read per second during ascent was enough to
keep the old code from ever seeing apogee.

Hardware (`sim_send.py --zero-pressure`, then offload):

- SeriousGooseV1Sim, MBTA log with pressure zeroed: warning printed, every logged row PRE_FLIGHT,
  nothing fired. Same log unmodified: identical to the pre-gate run (DESCENT 20.67 s, drogue 21.68 s,
  main 83.92 s, landed 114.18 s, 1 overrun).
- SillyGooseV2Sim, Crocket log with pressure zeroed: same result.

All 12 firmware envs build (+~400 B flash). Boards left on their release builds.
