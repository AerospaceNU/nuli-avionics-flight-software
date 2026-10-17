# agent_work

Everything produced by agent sessions: notes, and the test and hardware tooling written to
check the firmware. None of it is needed to build or flash the firmware - it is the record of
how changes were verified, and the tooling to verify the next ones.

**Start with [`notes/PENDING.md`](notes/PENDING.md)** - the single list of known-but-not-done
work. Anything that gets fixed should be deleted from that file, not annotated.

## Layout

| path | what | tracked |
|---|---|---|
| `notes/` | every agent-written note, flat, prefixed by topic | yes |
| `scripts/hardware/` | talking to real boards over USB serial: capture, bootloader entry, flight-sim replay, and validators for what the boards send | yes |
| `scripts/log_helper_tests/` | host test suite for `src/core/BasicLoggerHelper.h` - randomized struct definitions, compile-fail cases, multi-TU link, ARM compile | yes |
| `scripts/flight_logic_tests/` | host test of the launch gate: the real `StateEstimator1D` + `FlightStateDeterminer` fed flight logs with broken-barometer scenarios | yes |
| `scripts/log_format_vectors/` | firmware-vs-SillyGooseTool check: compiles each board's real log struct, decodes its records with the tool's `02-protocol.js` | yes |
| `scripts/ci_data/` | `make_ci_cases.py DESKTOP_EXE`: rebuilds `tests/sim/data` + `tests/sim/cases.json` (the CI sim flights) from in-house logs, MIT RocketPy logs and synthetic flights | yes |
| `patches/` | firmware test aids kept out of the tree (e.g. the I2C lock generator for `i2c_lock_test.py`) | yes |
| `data/` | small data a note cites | yes |
| `logs/` | raw captures and run output | no |

Generated files the scripts write next to themselves (`*.exe`, generated `.cpp`, `vectors.json`,
`cf/`) are gitignored.

## Scripts

All of them take paths relative to this repo, and expect SillyGooseTool checked out next to it
(`../SillyGooseTool`) - set `SGT_PROTOCOL` to its `src/js/02-protocol.js` otherwise. The host C++
builds need a `g++` on `PATH`, or `HOST_GXX_DIR` set to its `bin` folder (CLion bundles MinGW at
`%LOCALAPPDATA%/Programs/CLion/bin/mingw/bin`).

### `scripts/hardware/`

- `sg.py PORT OUT [--cmd=...]... [--until TEXT] [--idle S] [--max S]` - sends CLI commands and saves
  the exact bytes the board sends back. Write commands as `--cmd=--offload`: a bare `--cmd --offload`
  is read as an option by argparse.
- `touch.py PORT` - puts an Adafruit SAMD board in its bootloader and prints the bootloader's port,
  for `pio run -t upload --upload-port <that>`. PlatformIO's own 1200-baud touch is unreliable on
  the SillyGooses (see PENDING). Knows the Feather M0 and M4 bootloader IDs.
- `run_board.sh PORT PREFIX` - `--logFormat`, text offload, binary offload and a 6 s stream.
- `validate.js PREFIX [expected_definition.txt]` - checks a `run_board.sh` capture with the tool's
  own parser: one `LOG_FORMAT` line each time, the names line, the binary preamble's size and CRC,
  every binary value equal to the board's text offload, stream ordering and rate.
- `sim_send.py PORT LOG OUT [END_MS] [--stream]` - replays a flight log into a `*Sim` build exactly
  like the tool's Simulation tab. Pass `END_MS` for logs that keep ground-logging for hours after
  landing - the 100 Hz gap interpolation turns that into millions of samples. Leave `--stream` off
  for a real sim run: streaming during a sim makes nearly every tick overrun.
- `i2c_lock_test.py PORT OUT [ROUNDS]` - needs a Dev build (`--i2cLockTest`): locks the I2C bus
  mid-stream and checks the board recovers within a tick with no bad readings, then locks and
  reboots at once so the boot-time bus clear has to free it.
- `boot_capture.py PORT OUT [SECONDS]` - `--reset` and record whatever the board prints as it comes
  back (release builds don't wait for USB, so the earliest lines are usually missed - use a Dev
  build to see the whole boot).
- `sim_check.py ORIGINAL_LOG OFFLOAD [SIM_RUN]` - checks a sim the way it should be checked, by its
  offload: state changes, apogee and pyro events next to the original log's, relative to launch,
  plus the run's loop-overrun count (expect ~1, the start-up tick).
- `relay_check.js GROUND_CAPTURE SOURCE_STREAM` - every ground-station `RADIO_RX` packet must decode
  to exactly the row the flying board streamed over USB with the same timestamp.
- `sim_relay_check.js GROUND_CAPTURE SIM_CAPTURE` - the same for a relayed simulated flight, plus the
  flight-state changes and pyro flags seen over radio.

Before running a sim on a board, check its pyro channels read open (continuity 0) in a stream -
`*Sim` builds run the real flight logic and fire the real pyro outputs.

- `cli_fuzz.py PORT OUT [BATCHES] [SEED]` - malformed USB CLI input (never anything that sets, fires,
  erases or resets); reports stalls and USB disconnects (reboots) with the lines that caused them.
- `radio_cli_fuzz.py GROUND FLIGHT OUT [SENDS]` - the same through CLI-over-radio (`--send`), checking
  the flight board's uptime and that telemetry keeps coming.
- `state_changes.py OFFLOAD [N]` - per-boot state changes and pyro fires; for reboot-mid-sim runs
  (`sim_send.py --to=S`, `--reset`, `sim_send.py --from=S`). `sim_send.py --zero-from/--zero-to`
  zeroes pressure in a window (a barometer outage).

### `scripts/comment_only_check.py REV [PREFIX]`
Proves a change touched only comments: compares every changed C++ file with comments stripped.

### `scripts/flight_logic_tests/`
- `stress_test.cpp` (`build_stress.sh`): Monte-Carlo flights through the real flight code - see
  `notes/flight_STRESS_TEST_FIXES_2026-10-16.md`. `stress_test.exe emit SEED SCENARIO LOG` writes one
  flight as a board log for the desktop build.
- `real_replay.cpp` + `to_replay.py` + `replay_all.sh`: external flight logs through the same code.


`bash run.sh` builds `flight_gate_test` from the working tree's core sources and runs five scenarios
(normal, dead barometer, barometer dying on the pad, dead barometer after a reboot mid-flight, one
bad barometer tick per second in flight) on three flight logs from `../rocket-flight-data`. Ends with
`TOTAL FAILURES: 0`. `BASELINE=<rev> bash run.sh` also builds the flight logic from that revision
for comparison. `sim_send.py --zero-pressure` is the same dead-barometer check on real hardware.

### `scripts/log_helper_tests/`

`bash run_all.sh [iterations]` runs everything and ends with `TOTAL FAILING SECTIONS: 0`.

### `scripts/log_format_vectors/`

`python gen_board_vectors.py && node test_protocol.js` - should end `failures=0`.
