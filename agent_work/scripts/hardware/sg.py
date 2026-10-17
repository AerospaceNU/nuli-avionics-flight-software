# Talks to a board's CLI over USB serial and saves the exact bytes it sends back.
# usage: sg.py PORT OUTFILE [--cmd "--logFormat"]... [--until TEXT] [--idle SECONDS] [--max SECONDS]
# Stops when TEXT has been seen, or after IDLE seconds with no data, or MAX seconds total.
import argparse, sys, time
import serial

ap = argparse.ArgumentParser()
ap.add_argument("port")
ap.add_argument("out")
ap.add_argument("--cmd", action="append", default=[])
ap.add_argument("--until", default=None)
ap.add_argument("--idle", type=float, default=2.0)
ap.add_argument("--max", type=float, default=30.0)
ap.add_argument("--gap", type=float, default=0.5, help="seconds between commands")
a = ap.parse_args()

s = serial.Serial(a.port, 115200, timeout=0.05)
s.dtr = True
time.sleep(0.3)
s.reset_input_buffer()
buf = bytearray()
for c in a.cmd:
    s.write((c + "\n").encode())
    s.flush()
    end = time.time() + a.gap
    while time.time() < end:
        buf += s.read(65536)
start = last = time.time()
until = a.until.encode() if a.until else None
while True:
    chunk = s.read(65536)
    now = time.time()
    if chunk:
        buf += chunk
        last = now
    if until and until in buf:
        # grab whatever trails the marker on the same burst
        time.sleep(0.2)
        buf += s.read(65536)
        break
    if not until and now - last > a.idle:
        break
    if now - start > a.max:
        print("TIMEOUT", file=sys.stderr)
        break
s.close()
open(a.out, "wb").write(bytes(buf))
print(f"{len(buf)} bytes -> {a.out}")
