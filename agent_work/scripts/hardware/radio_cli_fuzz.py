# Fuzzes CLI-over-radio: sends malformed payloads through the ground station's --send, and checks
# every 10 sends that the flight board hasn't rebooted (uptime from a streamed row keeps growing)
# and the ground station still hears its telemetry. Same never-change-anything filter as cli_fuzz.py.
# usage: radio_cli_fuzz.py GROUND_PORT FLIGHT_PORT OUT [SENDS] [SEED]
import random, sys, time
import serial

ground_port, flight_port, out = sys.argv[1:4]
sends = int(sys.argv[4]) if len(sys.argv) > 4 else 60
rng = random.Random(int(sys.argv[5]) if len(sys.argv) > 5 else 3)
READ_ONLY = ["--FIRMWARE_VERSION", "--BOARD_NAME", "--MAIN_ELEVATION", "--DROGUE_DELAY", "--help", "--GROUND_ELEVATION"]
BANNED = ("set", "fire", "erase", "reset", "sim", "log", "offload", "stream", "-b", "-e", "-d", "-m", "-a")


def payload():
    kind = rng.randrange(5)
    if kind == 0: return " ".join(rng.choice(READ_ONLY) for _ in range(rng.randrange(1, 12)))
    if kind == 1: return rng.choice(READ_ONLY) + " " + " ".join(rng.choice(["nan", "1e40", "-", "--", "x" * 40, "99999999999"]) for _ in range(3))
    if kind == 2: return "y" * rng.choice([1, 50, 120, 127, 128, 129, 200, 300])
    if kind == 3: return "".join(chr(rng.randrange(32, 127)) for _ in range(rng.randrange(1, 140))).replace('"', "'")
    return "".join(chr(rng.choice(list(range(1, 9)) + list(range(14, 32)) + list(range(128, 256)))) for _ in range(rng.randrange(1, 60)))


def safe(p):
    return not any(b in p.lower() for b in BANNED)


def flight_uptime_ms():
    s = serial.Serial(flight_port, 115200, timeout=0.1)
    s.dtr = True
    time.sleep(0.3)
    s.reset_input_buffer()
    s.write(b"--streamLog -b\n")
    t, buf = time.time(), b""
    while time.time() - t < 2:
        buf += s.read(s.in_waiting or 1)
    s.write(b"--streamLog -e\n")
    time.sleep(0.5)
    s.read(s.in_waiting or 1)
    s.close()
    rows = [l for l in buf.split(b"\n") if l[:1].isdigit() and l.count(b"\t") > 10]
    return int(rows[-1].split(b"\t")[0]) if rows else None


ground = serial.Serial(ground_port, 115200, timeout=0.05, write_timeout=5)
ground.dtr = True
time.sleep(0.3)
cap = bytearray()
last_uptime = flight_uptime_ms()
reboots = silent = 0
print(f"flight uptime {last_uptime} ms", flush=True)
for i in range(sends):
    p = payload()
    while not safe(p):
        p = payload()
    ground.write(b'--send "' + p.encode("latin1", "replace") + b'"\n')
    t = time.time()
    while time.time() - t < 1.3:
        cap += ground.read(ground.in_waiting or 1)
    if (i + 1) % 10 == 0:
        recent = cap[-20000:]
        heard = recent.count(b"RADIO_RX\t")
        ground.close()
        uptime = flight_uptime_ms()
        ground = serial.Serial(ground_port, 115200, timeout=0.05, write_timeout=5)
        ground.dtr = True
        rebooted = uptime is None or (last_uptime is not None and uptime < last_uptime)
        reboots += rebooted
        silent += heard == 0
        print(f"sends {i + 1}: flight uptime {uptime} ms{' REBOOTED' if rebooted else ''}, telemetry packets in window {heard}", flush=True)
        last_uptime = uptime
ground.close()
open(out, "wb").write(bytes(cap))
print(f"done: {reboots} flight-board reboots, {silent} windows without telemetry")
