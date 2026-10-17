# Reboots a board with --reset and records what it prints while starting up (sensor init
# messages, config warnings), reopening the port as soon as it re-enumerates.
# usage: boot_capture.py PORT OUTFILE [SECONDS]
import sys, time
import serial

port, out = sys.argv[1], sys.argv[2]
seconds = float(sys.argv[3]) if len(sys.argv) > 3 else 15
s = serial.Serial(port, 115200, timeout=0.05)
s.write(b"--reset\n")
s.flush()
s.close()
buf = bytearray()
start = time.time()
while time.time() - start < seconds:
    try:
        s = serial.Serial(port, 115200, timeout=0.05)
        s.dtr = True
        break
    except serial.SerialException:
        time.sleep(0.02)
else:
    sys.exit("port never came back")
opened = time.time() - start
while time.time() - start < seconds:
    buf += s.read(s.in_waiting or 1)
s.close()
open(out, "wb").write(bytes(buf))
print(f"port back after {opened:.2f} s; {len(buf)} bytes -> {out}")
