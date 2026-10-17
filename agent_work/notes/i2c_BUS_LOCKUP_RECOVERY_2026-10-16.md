# I2C bus lock-up: what happened, and the soft recovery

## The incident (SeriousGooseV1 `8E8098CA`, 2026-10-16)

After a routine flash, the board booted with every I2C sensor dead. Its log shows exactly what
the firmware saw:

| boot | rows | barometer | flight state |
|---|---|---|---|
| previous build, after a USB replug | 6 | ~100,250 Pa | PRE_FLIGHT |
| first boot after the flash | 1,208 | **0 Pa** every row | ASCENT -> DESCENT (9.4 s) -> **drogue fired** (10.4 s) -> POST_FLIGHT |
| after `--reset` | 65,181 | 0 Pa | ASCENT for 652 s |

What the sensors were really returning:

- `pressurePa = 0`, `barometerTemperatureK = 293.15` exactly: the MS5607 calibration read failed,
  every coefficient was 0, and the datasheet compensation math turns any input into
  `P = 0`, `TEMP = 2000` (20.00 C) with zero coefficients.
- Accelerometer frozen at -0.0048 m/s^2 = -1 LSB: every IMU byte read back as `0xFF`, which is
  what `Wire.read()` returns after a failed transfer.

At 0 Pa the altitude is ~45 km, which passes the launch check, so the board "launched", detected
an "apogee", and fired the drogue on the bench (pyro channels were open - nothing attached). On
the next boot it "launched" again but never saw apogee, and sat in ASCENT: the CLI only runs in
PRE_FLIGHT/POST_FLIGHT, so USB went silent - it looked hung, but the loop was running (and the
watchdog was being fed) the whole time.

## Cause

A device on the bus was holding SDA low. When the MCU resets in the middle of a read, the device
that is sending a byte keeps driving its current 0 bit, waiting for clocks that never come - and
only losing power resets it. `readBarometerAdc()` deliberately leaves its read without a STOP, which
makes the barometer the likeliest candidate. With SDA held, the SAMD core's
`SERCOM::startTransmissionWIRE()` sees the bus as BUSY and returns failure at once, so every
transaction fails fast; nothing in the driver checked.

The lock survived a flash (bootloader reset) and a `--reset`, so soft resets alone don't clear it.

## Recovery (`src/drivers/arduino/I2cBusRecovery.h`, `Ms5607Mmc5603SensorPackage`)

`recoverI2cBus()` is the I2C spec's bus-clear sequence (UM10204 3.1.16): take the pins off the
peripheral, clock SCL by hand until the device finishes its byte and releases SDA (at most 9; it
allows 16), then send a STOP. Harmless on a healthy bus (no clocks, just a STOP). ~0.2 ms.

- **At boot**, before `Wire.begin()`. On the incident board itself - never power-cycled - the first
  boot of the recovery build printed `I2C bus was held at boot - SDA released after 1 clocks` and
  every sensor initialized and read normally.
- **In flight**: every transfer's result is now checked. A sensor whose transfer fails is not
  injected (it keeps its last reading rather than getting 0xFF bytes), the barometer is not
  injected until its calibration has been read, and `read()` clears the bus before the next tick -
  cheap enough to retry every tick until it comes back. One warning when it starts, one message
  when reads work again. A missing calibration is re-read after a successful clear (every 100th
  clear at most, since it costs ~3 ms).

## Tests (`scripts/hardware/i2c_lock_test.py`, Dev builds)

Dev builds register `--i2cLockTest [-r]`: it bit-bangs the barometer's ADC read and stops clocking
mid-byte while the chip drives a 0 bit - the same state a reset mid-read leaves - then hands the
pins back to the peripheral. `-r` reboots in the same tick, before the runtime recovery can act, so
the boot-time clear has to free it. (Locking the IMU instead also works for the runtime path, but
the IMU lets go across a reset, unlike the barometer - matching which device held the bus in the
incident.)

| board | runtime: lock mid-stream | reboot while locked |
|---|---|---|
| SeriousGooseV1 (SAMD51, ICM42605 + MMC5603) | 10/10 recovered next tick, "released after 1 clocks", 0 bad rows streamed | cleared at boot, all sensors init |
| SillyGooseV2 (SAMD21, ICM42605) | 5/5, same | cleared at boot |
| SillyGooseV1 (SAMD21, ICM20602) | 5/5, same | cleared at boot |

"Bad rows" = any streamed row with 0 Pa or the 293.15 K zero-calibration temperature.

## Still open (also in PENDING.md)

- A sensor that is really dead (not just a held bus) still reads as 0 Pa = 45 km, and the flight
  logic still treats that as a launch. The flight state machine needs a sensor-valid gate.
- The core's I2C waits (`while(!MB)` in write-mode address, `readDataWIRE()`) are unbounded: a
  device holding SCL low mid-transfer still wedges the loop, and only the watchdog gets out.
- Once a board decides it's in flight it ignores USB, so a false launch can't be stopped from the
  CLI.
