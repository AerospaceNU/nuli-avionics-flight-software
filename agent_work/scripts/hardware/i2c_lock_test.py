# Exercises Ms5607Mmc5603SensorPackage's I2C bus recovery on a board running a *Dev* build (which
# has --i2cLockTest). Each round: stream, lock the bus mid-stream, and check that the board reports
# the lock, recovers within a tick or two, and never streams a 0xFF/zero-calibration reading. Then
# locks it once more and reboots, so the boot-time bus clear has to free it. Writes every byte to OUT.
# usage: i2c_lock_test.py PORT OUT [ROUNDS]
#   Dev builds wait for the USB connection before booting, so this also sees the whole boot output.
import re, sys, time
import serial

port, out = sys.argv[1], sys.argv[2]
rounds = int(sys.argv[3]) if len(sys.argv) > 3 else 5
log = bytearray()


def open_port(timeout_s=20):
    t = time.time()
    while time.time() - t < timeout_s:
        try:
            s = serial.Serial(port, 115200, timeout=0.05, write_timeout=2)
            s.dtr = True
            return s
        except serial.SerialException:
            time.sleep(0.05)
    sys.exit(f"{port} didn't come back")


def pump(s, seconds):
    t = time.time()
    while time.time() - t < seconds:
        log.extend(s.read(s.in_waiting or 1))


def lines_since(mark):
    return bytes(log[mark:]).decode("latin1").replace("\r", "").split("\n")


s = open_port()
pump(s, 6)  # boot output
failures = 0
for r in range(rounds):
    mark = len(log)
    s.write(b"--streamLog -b\n"); pump(s, 1.0)
    s.write(b"--i2cLockTest\n"); pump(s, 1.0)
    s.write(b"--streamLog -e\n"); pump(s, 0.5)
    L = lines_since(mark)
    locked = any("I2C bus locked by the" in l for l in L)
    warned = [l for l in L if "I2C transfer failed" in l]
    back = [l for l in L if "I2C reads working again" in l]
    rows = [l.split("\t") for l in L if re.match(r"^\d", l)]
    bad = [x for x in rows if len(x) > 2 and (x[1] == "0" or abs(float(x[2]) - 293.15) < 1e-3)]
    ok = locked and warned and back and not bad and rows
    failures += not ok
    reply = next((l.split(chr(9))[-1] for l in L if "I2C bus" in l and "lock" in l), None)
    print(f"round {r + 1}: {reply!r} warn={warned[0].split(chr(9))[-1] if warned else None!r} "
          f"recovered={back[0].split(chr(9))[-1] if back else None!r} rows={len(rows)} bad_rows={len(bad)} -> {'ok' if ok else 'FAIL'}")

# Boot path: lock and reboot in the same tick (--i2cLockTest -r), before the runtime recovery can
# free it - the board must clear the bus during setup
mark = len(log)
s.write(b"--i2cLockTest -r\n"); s.flush()
s.close()
time.sleep(0.5)
s = open_port()
pump(s, 6)
s.write(b"--streamLog -b\n"); pump(s, 1.0)
s.write(b"--streamLog -e\n"); pump(s, 0.5)
L = lines_since(mark)
boot_clear = [l for l in L if "I2C bus was held at boot" in l]
inits = [l for l in L if re.search(r"(MS5607|ICM42605|ICM20602|MMC5603NJ) initialized", l)]
rows = [l.split("\t") for l in L if re.match(r"^\d", l)]
bad = [x for x in rows if len(x) > 2 and (x[1] == "0" or abs(float(x[2]) - 293.15) < 1e-3)]
ok = boot_clear and inits and rows and not bad
failures += not ok
print(f"reboot while locked: {boot_clear[0].split(chr(9))[-1] if boot_clear else 'no boot clear seen'!r}; "
      f"inits={[l.split(chr(9))[-1] for l in inits]} rows={len(rows)} bad_rows={len(bad)} -> {'ok' if ok else 'FAIL'}")
s.close()
open(out, "wb").write(bytes(log))
print(f"{len(log)} bytes -> {out}; {'ALL PASSED' if not failures else f'{failures} FAILED'}")
sys.exit(1 if failures else 0)
