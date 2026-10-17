# Replays a flight log into a *Sim firmware build exactly like the tool's 10-simulation.js
# (35 s preflight trim, ground-gap interpolation at 100 Hz, --simSize flow control with
# HIGH_WATER 90 / BURST 2), recording every byte the board sends. Check the result by
# offloading afterwards, not by streaming during the sim: --streamLog adds a row per tick on top
# of the sim and makes nearly every tick overrun, which a normal sim run doesn't do.
# --zero-pressure sends 0 Pa for every sample - a dead/uncalibrated barometer - to check the board
# never leaves PRE_FLIGHT without one.
# --from=S / --to=S send only the samples from S to S seconds into the (trimmed) sim, for a reboot
# mid-sim: run --to=S, --reset the board, then run --from=S. "launch at sample N" is printed so S can
# be picked relative to launch.
# --zero-from=S --zero-to=S sends 0 Pa only in that window (seconds into the trimmed sim): a barometer outage.
# usage: sim_send.py PORT LOGFILE OUTFILE [END_MS] [--stream] [--zero-pressure] [--from=S] [--to=S] [--zero-from=S --zero-to=S]
import re, sys, time, threading
import serial

port, logfile, outfile = sys.argv[1:4]
stream = "--stream" in sys.argv
zero_pressure = "--zero-pressure" in sys.argv
opt = {a.split("=")[0]: float(a.split("=")[1]) for a in sys.argv[4:] if a.startswith(("--from=", "--to=", "--zero-from=", "--zero-to="))}
args = [a for a in sys.argv[4:] if a not in ("--stream", "--zero-pressure") and not a.startswith(("--from=", "--to=", "--zero-from=", "--zero-to="))]
end_ms = float(args[0]) if args else None  # optional: drop samples after this log timestamp
HZ, DT = 100.0, 10.0
GAP, KEEP_S, HIGH, BURST = 30.0, 35.0, 90, 2

rows = []
for line in open(logfile, encoding="utf-8", errors="replace"):
    line = line.strip()
    if not line or not line[0].isdigit():
        continue
    p = line.split("\t")
    if len(p) < 9:
        continue
    try:
        rows.append([float(x) for x in p[:9]])
    except ValueError:
        pass
rows.sort(key=lambda r: r[0])
if end_ms is not None:
    rows = [r for r in rows if r[0] <= end_ms]
launch = next((i for i in range(1, len(rows)) if rows[i][0] - rows[i - 1][0] <= GAP), -1)
if launch >= 0:
    cutoff = rows[launch][0] - KEEP_S * 1000
    rows = [r for r in rows if r[0] >= cutoff]
samples = [rows[0][1:]]
prev = rows[0]
for cur in rows[1:]:
    dt = cur[0] - prev[0]
    if dt > GAP:
        t = prev[0] + DT
        while t < cur[0]:
            ratio = (t - prev[0]) / dt
            samples.append([v + (c - v) * ratio for v, c in zip(prev, cur)][1:])
            t += DT
    samples.append(cur[1:])
    prev = cur
if "--zero-from" in opt:
    for smp in samples[int(opt["--zero-from"] * HZ):int(opt["--zero-to"] * HZ)]:
        smp[0] = 0.0
if zero_pressure:
    for smp in samples:
        smp[0] = 0.0  # sample order is pressure, temperature, accel x,y,z, gyro x,y,z
print(f"launch (first dense log row) at sample {int(KEEP_S * HZ) if launch >= 0 else 0}", flush=True)
samples = samples[int(opt.get("--from", 0) * HZ):int(opt["--to"] * HZ) if "--to" in opt else None]
print(f"{len(samples)} samples (~{len(samples) / HZ:.0f} s){' with pressure zeroed' if zero_pressure else ''}", flush=True)

s = serial.Serial(port, 115200, timeout=0.02)
s.dtr = True
time.sleep(0.3)
s.reset_input_buffer()
log = bytearray()
size_lock = threading.Condition()
state = {"size": None, "stop": False}


def reader():
    pending = b""
    while not state["stop"]:
        # Only ask for what's already buffered (or wait for 1 byte): read(65536) sits out the
        # whole timeout, so every --simSize report arrived late and starved the board
        chunk = s.read(s.in_waiting or 1)
        if not chunk:
            continue
        log.extend(chunk)
        pending += chunk
        *lines, pending = pending.split(b"\n")
        for l in lines:
            m = re.match(rb"^--simSize\s+(\d+)", l)
            if m:
                with size_lock:
                    state["size"] = int(m.group(1))
                    size_lock.notify_all()


th = threading.Thread(target=reader, daemon=True)
th.start()


def send(batch):
    s.write("".join("--sim " + ",".join(f"{v:.6f}" for v in smp) + "\n" for smp in batch).encode())


if stream:
    s.write(b"--streamLog -b\n")
    time.sleep(0.5)
idx = min(BURST, len(samples))
send(samples[:idx])
t0 = time.time()
while idx < len(samples):
    with size_lock:
        if state["size"] is None:
            size_lock.wait(1.0)
        size, state["size"] = state["size"], None
    if size is None:
        continue
    n = max(0, min(BURST, HIGH - size, len(samples) - idx))
    if n:
        send(samples[idx:idx + n])
        idx += n
    if idx % 2000 < BURST:
        print(f"  {idx}/{len(samples)} sent, fw buffer {size}, {time.time() - t0:.0f}s", flush=True)
print(f"done in {time.time() - t0:.0f}s", flush=True)
time.sleep(2)
state["stop"] = True
th.join()
s.close()
open(outfile, "wb").write(bytes(log))
print(f"{len(log)} bytes -> {outfile}")
