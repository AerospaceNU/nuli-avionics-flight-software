# LOG_FORMAT / BasicLoggerHelper - hardware validation, 2026-10-16

What was checked when the log struct moved to `LOG_DATA_AUTO_DEFINE` and boards started sending
their own definition (`LOG_FORMAT\t<definition>`, see `BasicLogger::printFormat()`), with
SillyGooseTool decoding from that instead of a per-board registry. Raw captures are in
`logs/hardware/2026-10-16_log_format/` (not tracked).

## Boards

| board | USB serial | firmware before | config after update |
|---|---|---|---|
| SeriousGooseV1, ground station | `8FB1E805` | local "unversioned" build | reset (see PENDING), restored by hand: `GROUND_STATION_MODE=1`, `DROGUE_DELAY=1003`, `BUZZER_ENABLED=0` |
| SeriousGooseV1, flight | `8E8098CA` | `542c992` | reset to defaults (it was at defaults anyway) |
| SillyGooseV1 | `C5CB437E` | `c4d9423` | kept |
| SillyGooseV2 | `AC331F6B` | `c4d9423` | kept |

Every board's log was backed up with a text offload on its old firmware before flashing (copies in
the user's Downloads folder).

## Results

Per board and build (SeriousGooseV1, SeriousGooseAirbreaksV1, SillyGooseV1, SillyGooseV2), via
`run_board.sh` + `validate.js`:

- `--logFormat` reply byte-identical to the compiled `HEADER_OUTPUT`; the same line before every
  text offload, before every binary preamble, and immediately before "Streaming enabled".
- Binary preamble size and CRC match the definition; every binary record decodes to exactly the
  board's own text-offload value - 228-770 k values per SeriousGoose run, 0 mismatches.
- The same flash records offloaded by the old and the new firmware agree in every value (233,905
  compared) once the old firmware's truncation is allowed for: its `%f` cuts digits
  (`300.029999` -> `300.02`), the new printer rounds.
- Ground-station relay: with both SeriousGooses capturing at once, every `RADIO_RX` packet decodes
  to the identical row the flying board streamed over USB at that timestamp (20/20, then 256/256
  through a whole simulated flight). `--logFormat` sent over the radio CLI (`--send`) is ignored,
  as intended (high-bandwidth only); a normal command still answers.
- Flight sims, run and then checked by offloading (`sim_send.py` + `sim_check.py`), times relative
  to launch:

  | board / log | apogee (orig / sim) | descent | drogue | main | landed | overruns |
  |---|---|---|---|---|---|---|
  | SillyGooseV1, 2025-11-15 MBTA ridealong | 1694 / 1688 m | 19.4 / 20.7 s | 20.4 / 21.7 s | 82.9 / 83.9 s | 112.9 / 114.2 s | 1 |
  | SillyGooseV2, 2025-11-15 Crocket backup | 1916 / 1882 m | 18.2 / 22.2 s | 19.2 / 23.2 s | 124.3 / 126.8 s | 154.9 / 158.6 s | 1 |
  | SillyGooseV1, 2025-11-15 Nathan McGuire cert | 285 / 280 m | 8.9 / 9.5 s | 10.0 / 10.5 s | 14.4 / 15.4 s | 43.2 / 43.8 s | 1 |

  Three back-to-back binary offloads of the V2 board's 31,600 records came out byte-identical.

  Both offloads pass `validate.js` (binary == text in every value). The one overrun per run is the
  start-up tick (~170 ms). A SeriousGooseV1Sim run (MBTA log) was also relayed live over radio to
  the ground station: 256/256 packets identical to the board's own rows, every state change and
  pyro flag seen. Pyro channels read open on every board first.
- Replaying the real captures through SillyGooseTool's own read loop (built `dist/index.html`,
  64-512 byte chunks) gives the same flights, rows, configs and live-view values.

## Found and fixed during the campaign

- **SillyGoose binary offload returned 0 records** while text found them all: the V1's flash
  starts with entries from `c4d9423` (different entry layout, no CRC), and the binary loop counted
  CRC failures toward its four-strikes stop. It now skips them like the text loop does.
- **SillyGoose streaming overran every tick** (12 ms on the first new build vs 10 ms on the old
  firmware): the printer re-parsed the definition string per row and used 64-bit division on an
  M0+ with no divider. The definition is now parsed once at compile time (`FieldsOf<Def>`) and
  formatting uses 32-bit math where it fits: 1 overrun per ~600 rows at 100 rows/s, better than the
  old firmware's every-row overrun. SeriousGooses never overran while streaming.
- The first sim runs overran nearly every tick - that was `sim_send.py`, not the firmware: its
  reader asked for 64 KB per read and so sat out the whole 20 ms timeout on every `--simSize`
  report, starving the board (~62 samples/s instead of 100); those runs also had `--streamLog` on,
  which a sim shouldn't. Fixed both; the runs in the table above are with the fix.

## Not caused by this work

Config reset on update (`e25e4171`), PlatformIO's touch on SillyGoose, Airbreaks' `auxState`
meaning - see `PENDING.md`.
