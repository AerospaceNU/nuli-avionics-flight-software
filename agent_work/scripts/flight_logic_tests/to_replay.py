# Converts an external flight log to real_replay.exe's input: "t_s,pressure_pa,axial_sf_mss[,ref_h]"
# with t = 0 at launch (first sample with axial specific force > 3 g). Run with python -I - the logs
# are untrusted data and are only read.
# usage: to_replay.py FORMAT IN OUT
#   altos  Altus Metrum CSV export (time, pressure Pa, acceleration = axial, gravity removed; height)
#   tars   ISS TARS flight_computer.csv (timestamp_ms, pressure hPa, az low-g / highg_az, in g)
#   osu    OSU BSLI logv3 (time_boot_ms, ms5607_pressure_mbar, bmi323_accel_z <= 4 g / adxl375_accel_z)
import csv, math, sys

G = 9.80665
fmt, src, dst = sys.argv[1:4]


def num(x):
    try:
        v = float(str(x).strip().strip('"'))
        return v if math.isfinite(v) else None
    except ValueError:
        return None


rows = []  # (t, p, a, ref)
with open(src, newline="", encoding="utf-8", errors="replace") as f:
    lines = [l for l in f if l.strip() and not l.startswith("# ")]
hdr = [h.strip().lstrip("#") for h in next(csv.reader([lines[0]]))]
col = {}
for i, h in enumerate(hdr):
    col.setdefault(h, i)  # first column of a name (AltOS has two "altitude")
for r in csv.reader(lines[1:]):
    if len(r) < len(hdr) - 5:
        continue
    g = lambda name: num(r[col[name]]) if name in col and col[name] < len(r) else None
    if fmt == "altos":
        t, p, a, ref = g("time"), g("pressure"), g("acceleration"), g("height")
        if a is not None:
            a += G
    elif fmt == "tars":
        t, p = g("timestamp_ms"), g("pressure")
        lo, hi = g("az"), g("highg_az")
        if None in (t, p, lo, hi):
            continue
        t, p, a, ref = t / 1000, p * 100, (lo if abs(lo) < 1.9 else hi) * G, None
    elif fmt == "osu":
        t, p = g("time_boot_ms"), g("ms5607_pressure_mbar")
        lo, hi = g("bmi323_accel_z"), g("adxl375_accel_z")
        if None in (t, p, lo, hi):
            continue
        t, p, a, ref = t / 1000, p * 100, (lo if abs(lo) < 3.9 else hi) * G, None
    else:
        raise SystemExit("unknown format " + fmt)
    if None in (t, p, a) or p <= 0:
        continue
    rows.append((t, p, a, ref))

rows.sort()
launch = next((rows[i][0] for i in range(len(rows) - 5) if all(rows[k][2] > 3 * G for k in range(i, i + 5))), None)
if launch is None:
    launch = rows[0][0]
    print("no launch found (no 5 samples > 3 g); t = 0 at the first row")
if fmt == "altos":
    launch = 0.0  # AltOS time is already relative to boost detect
with open(dst, "w") as f:
    for t, p, a, ref in rows:
        f.write(f"{t - launch:.4f},{p:.2f},{a:.4f}" + (f",{ref:.2f}" if ref is not None else "") + "\n")
print(f"{len(rows)} rows, {rows[0][0] - launch:.2f}..{rows[-1][0] - launch:.2f} s -> {dst}")
