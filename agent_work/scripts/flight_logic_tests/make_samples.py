# Turns a flight log into the 100 Hz sample stream a Sim build gets (same trim and gap
# interpolation as scripts/hardware/sim_send.py and the tool's Simulation tab), written as
# "pressurePa,tempK,ax,ay,az,gx,gy,gz" lines for flight_gate_test.
# usage: make_samples.py LOGFILE OUT [END_MS]
import sys

logfile, out = sys.argv[1], sys.argv[2]
end_ms = float(sys.argv[3]) if len(sys.argv) > 3 else None
GAP, KEEP_S, DT = 30.0, 35.0, 10.0

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
with open(out, "w") as f:
    for s in samples:
        f.write(",".join(f"{v:.6f}" for v in s) + "\n")
print(f"{len(samples)} samples -> {out}")
