# Converts every flight log in an AerospaceNU/rocket-flight-data checkout (GPL-3, same organization) into
# the desktop sim's input layout, one file per flight, plus manifest.json describing each one.
#
#   timestampMs  pressurePa  barometerTemperatureK  accelerationMSS_x/y/z  velocityRadS_x/y/z   (tab-separated)
#
# Accelerations are what a strapped-down accelerometer reads (specific force: |a| = g at rest). The
# firmware picks the vertical axis and its sign from the pad reading, so source axes are kept as they are.
# Single-axis sources (AltOS) put the axial reading on z. Sources without an accelerometer get a constant
# (0, 0, g) and are flagged. All ground time in a log is kept; logs that start at launch (AltOS,
# StratoLogger, some FCB) get 60 s of constant synthetic pad in front, flagged, so the sim has a ground
# reference.
#
# usage: python -I convert_rocket_flight_data.py [--src RFD_CHECKOUT] [--out DIR] [--sim DESKTOP_EXE]
#   --src  default ../rocket-flight-data next to this repo; only its data/ and flight-data/ are read
#   --out  default agent_work/logs/rocket_flight_data_sim
#   --sim  also run each output through the desktop build and record the states it reports
import argparse
import ast
import csv
import hashlib
import json
import math
import os
import re
import statistics
import subprocess
import sys
from bisect import bisect_left
from collections import Counter, deque

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "../../.."))
G = 9.80665
ATM = 101325.0
HEADER = "timestampMs\tpressurePa\tbarometerTemperatureK\taccelerationMSS_x\taccelerationMSS_y\taccelerationMSS_z\tvelocityRadS_x\tvelocityRadS_y\tvelocityRadS_z"
SYNTH_PAD_MS = 60000
SYNTH_PAD_STEP_MS = 100
MIN_REAL_PAD_S = 10.0
SG_V1_COLS = "timestampMs pressurePa tempK accelX accelY accelZ gyroX gyroY gyroZ imuTemp battV altitudeM velocityMS accelerationMSS unfiltAlt flightState drogueCont drogueFired mainCont mainFired".split()
SG_V2_COLS = SG_V1_COLS + "tiltMagnitudeDeg angularVelRadS_x angularVelRadS_y angularVelRadS_z quaternion_a quaternion_b quaternion_c quaternion_d".split()


# ---------------------------------------------------------------- helpers
def std_alt(p):
    return (1.0 - (max(p, 1.0) / ATM) ** 0.190263) / 2.25577e-5


def std_p(h):
    return ATM * (1.0 - 2.25577e-5 * h) ** 5.25588


def med(xs, default=None):
    xs = [x for x in xs if x is not None and math.isfinite(x)]
    return statistics.median(xs) if xs else default


def num(s):
    try:
        v = float(s)
        return v if math.isfinite(v) else None
    except (TypeError, ValueError):
        return None


def slug(s):
    s = s.replace("'", "")
    return re.sub(r"[^a-z0-9]+", "_", s.lower()).strip("_")


def read_attributes(folder):
    path = os.path.join(folder, "attributes.csv")
    out = {}
    if os.path.isfile(path):
        with open(path, encoding="utf-8", errors="replace", newline="") as f:
            for row in csv.reader(f):
                if len(row) >= 2 and row[0] != "key":
                    out[row[0]] = row[1]
    return out


def segment(rows, events=None, **kw):
    seg = {"rows": rows, "events": events or [], "flags": [], "notes": [], "has_accel": True, "has_gyro": True,
           "has_temp": True, "accel_kind": "imu", "pad_pressure_hint": None, "pad_accel_hint": None, "launch_hint_ms": None}
    seg.update(kw)
    return seg


def state_events(times, values, label, persist=1):
    """[(t, text)] for every change of values, ignoring values that do not last `persist` samples."""
    out, prev = [], None
    for i, v in enumerate(values):
        if v is None or v == prev:
            continue
        if persist > 1 and any(i + k >= len(values) or values[i + k] != v for k in range(persist)):
            continue
        out.append((times[i], f"{label} {prev if prev is not None else '-'} -> {v}"))
        prev = v
    return out


def monotonic_split(rows, overlap_ms=2000.0):
    """FCB/SillyGoose clocks: a small step back is a pre-launch buffer dump (keep the dump, drop what it
    overlaps); a big step back is a reboot (new segment)."""
    segs, cur, dropped = [], [], 0
    for r in rows:
        if cur and r[0] <= cur[-1][0]:
            if cur[-1][0] - r[0] < overlap_ms:
                while cur and cur[-1][0] >= r[0]:
                    cur.pop()
                    dropped += 1
            else:
                segs.append(cur)
                cur = []
        cur.append(r)
    if cur:
        segs.append(cur)
    return segs, dropped


# ---------------------------------------------------------------- format: SillyGoose text logs
def parse_sillygoose(path, attrs):
    config, names, raw = {}, None, []
    for line in open(path, encoding="utf-8", errors="replace"):
        s = line.strip()
        if not s:
            continue
        if s.startswith("CONFIG"):
            for kv in s.split("\t")[1:]:
                if "=" in kv:
                    k, v = kv.split("=", 1)
                    config[k] = v
            continue
        parts = re.split(r"[\t ,]+", s)
        if not re.match(r"^-?\d", parts[0]):
            names = parts
            continue
        vals = [num(x) for x in parts]
        if vals[0] is None:
            continue
        raw.append(vals)
    if not raw:
        return [], "SillyGoose (empty)", {}
    ncol = Counter(len(r) for r in raw).most_common(1)[0][0]
    if names is None:
        names = SG_V2_COLS if ncol == 28 else SG_V1_COLS
    ix = {n: i for i, n in enumerate(names)}
    need = ["timestampMs", "pressurePa", "tempK", "accelX", "accelY", "accelZ", "gyroX", "gyroY", "gyroZ"]
    raw = [r for r in raw if len(r) >= len(need)]
    get = lambda r, n: r[ix[n]] if ix[n] < len(r) else None
    st_i = ix.get("flightState")
    first_state = raw[0][st_i] if st_i is not None and st_i < len(raw[0]) else None
    pad = [r for r in raw[:200] if st_i is None or get(r, "flightState") == first_state][:20]
    flags, notes = [], []
    p_pad = med([get(r, "pressurePa") for r in pad if (get(r, "pressurePa") or 0) > 0])
    p_scale = 1.0
    if p_pad is not None and p_pad < 2:
        p_scale = ATM; notes.append("pressure column in atm -> Pa")
    elif p_pad is not None and p_pad < 1500:
        p_scale = 100.0; notes.append("pressure column in hPa -> Pa")
    t_pad = med([get(r, "tempK") for r in pad])
    t_off = 0.0
    if t_pad is not None and t_pad < 150:
        t_off = 273.15
        flags.append(f"temperature column labelled K reads {t_pad:.1f} on the pad - treated as degC")
    a_pad = med([math.sqrt(get(r, "accelX") ** 2 + get(r, "accelY") ** 2 + get(r, "accelZ") ** 2) for r in pad])
    a_scale = 1.0
    if a_pad is not None and 0.5 < a_pad < 1.5:
        a_scale = G; notes.append("accel in g -> m/s^2")
    elif a_pad is None or not 7 < a_pad < 12.5:
        flags.append(f"pad |accel| {a_pad} - units unclear, passed through")
    g_max = max(abs(get(r, n)) for r in raw for n in ("gyroX", "gyroY", "gyroZ"))
    g_scale = 1.0
    if g_max > 60:
        g_scale = math.pi / 180; notes.append(f"gyro in deg/s (max {g_max:.0f}) -> rad/s")
    rows = [[get(r, "timestampMs"), get(r, "pressurePa") * p_scale, get(r, "tempK") + t_off,
             get(r, "accelX") * a_scale, get(r, "accelY") * a_scale, get(r, "accelZ") * a_scale,
             get(r, "gyroX") * g_scale, get(r, "gyroY") * g_scale, get(r, "gyroZ") * g_scale, r] for r in raw]
    segs, dropped = monotonic_split(rows)
    if dropped:
        notes.append(f"{dropped} rows overlapped by a clock step back dropped")
    out = []
    for s in segs:
        t = [r[0] for r in s]
        ev = []
        if st_i is not None:
            ev += state_events(t, [get(r[9], "flightState") for r in s], "flightState")
        for col in ("drogueFired", "mainFired"):
            if col in ix:
                ev += [(tt, txt) for tt, txt in state_events(t, [get(r[9], col) for r in s], col) if txt.endswith("-> 1.0")][:1]
        out.append(segment([r[:9] for r in s], ev, flags=list(flags), notes=list(notes)))
    board = config.get("BOARD_NAME") or attrs.get("BOARD_NAME") or ""
    fmt = f"SillyGoose text log ({board or 'board unknown'}, {ncol} columns)"
    if "converted_from" in config or "converted_from" in attrs:
        fmt = "SillyGoose-layout CSV converted from an FCB ground-station capture (EggThing flight)"
    return out, fmt, config


# ---------------------------------------------------------------- format: FCB post-processed CSV
def lsq(x, y):
    n = len(x)
    mx, my = sum(x) / n, sum(y) / n
    sxx = sum((a - mx) ** 2 for a in x)
    syy = sum((b - my) ** 2 for b in y)
    sxy = sum((a - mx) * (b - my) for a, b in zip(x, y))
    if sxx <= 0 or syy <= 0:
        return 0.0, 0.0, 0.0
    s = sxy / sxx
    return s, my - s * mx, sxy / math.sqrt(sxx * syy)


def parse_fcb_post(path, attrs):
    with open(path, encoding="utf-8", errors="replace", newline="") as f:
        rd = csv.reader(f)
        hdr = next(rd)
        data = [r for r in rd if len(r) == len(hdr)]
    ix = {n: i for i, n in enumerate(hdr)}
    col = lambda name: [num(r[ix[name]]) if name in ix else None for r in data]
    flags, notes = [], []
    tk = "timestamp_ms" if "timestamp_ms" in ix and any(col("timestamp_ms")[:50]) else "timestamp_s"
    t = col(tk)
    dts = [b - a for a, b in zip(t, t[1:]) if a is not None and b is not None and b > a]
    t_scale = 1000.0 if med(dts, 1) < 1 else 1.0
    if t_scale != 1.0:
        notes.append(f"time column {tk} in seconds -> ms")
    elif tk == "timestamp_s":
        notes.append("time column 'timestamp_s' holds ms")

    def pick(cands, ok):
        best = None
        for c in cands:
            if c not in ix:
                continue
            v = col(c)
            good = sum(1 for x in v if x is not None and ok(x))
            if good > 0.9 * len(v):
                return c
            if best is None or good > best[1]:
                best = (c, good)
        return best[0] if best else None

    pcol = pick(["baro1_pres", "baro2_pres"], lambda x: 0.3 < x < 1.2 or 30000 < x < 120000)
    tcol = pcol.replace("pres", "temp")
    if tcol not in ix or not any(col(tcol)):
        tcol = next((c for c in ("baro1_temp", "baro2_temp", "baro_temp_avg") if c in ix and any(col(c))), None)
    p = col(pcol)
    p_pad = med(p[:50])
    p_scale = ATM if p_pad < 2 else (100.0 if p_pad < 1500 else 1.0)
    notes.append(f"pressure from {pcol} ({'atm' if p_scale == ATM else 'hPa' if p_scale == 100 else 'Pa'} -> Pa), temperature from {tcol} (degC -> K)")
    temp = col(tcol) if tcol else [None] * len(data)

    def imu_ok(k):
        a = [col(f"imu{k}_accel_{ax}_real") for ax in "xyz"]
        if any(v[0] is None for v in a):
            return None
        mags = [math.sqrt(x * x + y * y + z * z) for x, y, z in zip(*[v[:30] for v in a]) if None not in (x, y, z)]
        return abs(med(mags, 0) - G)

    use = 1
    e1, e2 = imu_ok(1), imu_ok(2)
    if e1 is None or (e1 > 3 and e2 is not None and e2 < 1.5):
        use = 2
    notes.append(f"accel/gyro from imu{use}_*_real (m/s^2, rad/s); imu_*_avg not used (it halves when imu2 reads 0)")
    acc = [col(f"imu{use}_accel_{ax}_real") for ax in "xyz"]
    gyr = [col(f"imu{use}_gyro_{ax}_real") for ax in "xyz"]
    pad_mag = med([math.sqrt(x * x + y * y + z * z) for x, y, z in zip(*[v[:60] for v in acc]) if None not in (x, y, z)], G)
    if 1.4 < pad_mag / G < 1.6:
        # FCBV2: raw counts are 2048 LSB/g (+-16 g) but the post-processing scaled them as +-24 g
        acc = [[x * 2 / 3 if x is not None else None for x in v] for v in acc]
        flags.append(f"imu accel *_real columns read {pad_mag:.2f} m/s^2 at rest (raw ~2048 counts/g = +-16 g, scaled as +-24 g): multiplied by 2/3")
    elif abs(pad_mag - G) > 1.5:
        flags.append(f"pad |accel| {pad_mag:.2f} m/s^2 - not 1 g, passed through")
    gmax = max(abs(x) for v in gyr for x in v if x is not None)
    if gmax > 60:
        gyr = [[x * math.pi / 180 if x is not None else None for x in v] for v in gyr]
        notes.append("gyro in deg/s -> rad/s")
    # IMU saturation: refill clipped samples from the high-g accelerometer, axes matched by fit
    sat = max(abs(x) for v in acc for x in v if x is not None)
    nsat = sum(1 for v in acc for x in v if x is not None and abs(x) >= 0.995 * sat)
    if sat > 150 and nsat >= 3:
        hg = [col(f"high_g_accel_{ax}_real") for ax in "xyz"]
        msgs = []
        multi = [i for i in range(len(data)) if sum(1 for v in acc if v[i] is not None and abs(v[i]) >= 0.995 * sat) >= 2]
        if multi and t[multi[0]] is not None:
            flags.append(f"shock: 2-3 IMU axes clipped at once on {len(multi)} samples, first at source time {t[multi[0]] * t_scale:.0f} ms "
                         "(ejection / CATO / impact; pressure is disturbed around it)")
        for j in range(3):
            clipped = [i for i, x in enumerate(acc[j]) if x is not None and abs(x) >= 0.995 * sat]
            if not clipped:
                continue
            good = [i for i in range(len(data)) if all(acc[a][i] is not None and abs(acc[a][i]) < 0.9 * sat for a in range(3))
                    and abs(acc[j][i]) > 15 and all(hg[k][i] is not None for k in range(3))]
            best = None
            if len(good) >= 30:
                for k in range(3):
                    s, b, r = lsq([hg[k][i] for i in good], [acc[j][i] for i in good])
                    if best is None or abs(r) > abs(best[2]):
                        best = (k, s, b, r)
            if best and abs(best[3]) > 0.8 and (0.5 < abs(best[1]) < 2 or 5 < abs(best[1]) < 20):
                k, s, b, r = best
                for i in clipped:
                    if hg[k][i] is not None:
                        acc[j][i] = s * hg[k][i] + b
                msgs.append(f"{'xyz'[j]}<-{s:+.3f}*hg_{'xyz'[k]} (r={r:.2f}, {len(clipped)} samples)")
            else:
                msgs.append(f"{'xyz'[j]} {len(clipped)} samples left clipped (no high-g fit, r={best[3] if best else float('nan'):.2f})")
        flags.append(f"imu accel clipped at {sat:.0f} m/s^2; " + "; ".join(msgs))
    st = col("state")
    rows = []
    for i in range(len(data)):
        if t[i] is None or p[i] is None:
            continue
        rows.append([t[i] * t_scale, p[i] * p_scale, (temp[i] + 273.15) if temp[i] is not None else None,
                     acc[0][i], acc[1][i], acc[2][i], gyr[0][i], gyr[1][i], gyr[2][i], st[i]])
    segs, dropped = monotonic_split(rows)
    if dropped:
        notes.append(f"{dropped} rows overlapped by the pre-launch buffer dump (clock steps back ~90 ms) dropped")
    meta = {}
    mpath = os.path.splitext(path)[0].replace("-output-FCB-post", "") + "-metadata.json"
    if os.path.isfile(mpath):
        txt = open(mpath, encoding="utf-8").read().replace("NaN", "null")
        meta = json.loads(txt)
    out = []
    for s in segs:
        tt = [r[0] for r in s]
        ev = state_events(tt, [int(r[9]) if r[9] is not None else None for r in s], "FCB state")
        for key, val in list(meta.items()) + [("apogee_timestamp", attrs.get("apogee_timestamp"))]:
            if key == "apogee_timestamp" and num(val) is not None and tt[0] <= num(val) * (1 if t_scale == 1 else 1) <= tt[-1] + 1e9:
                ev.append((num(val), f"metadata apogee_timestamp {val}"))
        out.append(segment([r[:9] for r in s], ev, flags=list(flags), notes=list(notes)))
    return out, f"FCB post-processed CSV ({len(hdr)} columns)", meta


# ---------------------------------------------------------------- format: FCB ground-station capture
GS_LINE = re.compile(r"^(\d\d):(\d\d):(\d\d\.\d+): ([A-Za-z &]+?)\s*(\{.*\})\s*$")


def parse_fcb_gs(path, attrs):
    recs, unparsable, badcrc = [], 0, 0
    for line in open(path, encoding="utf-8", errors="replace"):
        m = GS_LINE.match(line.strip())
        if not m:
            continue
        try:
            d = ast.literal_eval(m.group(5))
        except Exception:
            unparsable += 1
            continue
        if "time_stamp_ms" not in d:
            continue
        if d.get("crc") != "Good":
            badcrc += 1
            continue
        wall = (int(m.group(1)) * 3600 + int(m.group(2)) * 60 + float(m.group(3))) * 1000
        recs.append((wall, m.group(4), d))
    # boots: the board clock minus the ground-station clock is constant within a boot; records whose
    # offset matches neither the current boot nor the next few records are corrupt and dropped
    segs, cur, junk = [], [], 0
    offs = [r[2]["time_stamp_ms"] - r[0] for r in recs]
    for i, r in enumerate(recs):
        off = offs[i]
        if cur:
            ref = med([o for o, _ in cur[-20:]])
            if abs(off - ref) > 3000:
                nxt = offs[i + 1:i + 6]
                if len(nxt) >= 3 and all(abs(o - off) < 3000 for o in nxt):
                    segs.append(cur)
                    cur = []
                else:
                    junk += 1
                    continue
        cur.append((off, r))
    if cur:
        segs.append(cur)
    out = []
    for sg in segs:
        rs = [r for _, r in sg]
        ori = sorted((d["time_stamp_ms"], d) for _, k, d in rs if k == "Orientation")
        pos = sorted((d["time_stamp_ms"], d) for _, k, d in rs if k == "Position Data")
        alt = sorted((d["time_stamp_ms"], d) for _, k, d in rs if k.startswith("Alt Info"))
        if not pos or not alt:
            out.append(segment([], skip_reason=f"boot with {len(rs)} records but no Position Data / Alt Info packets (no pressure) - {Counter(k for _, k, _ in rs).most_common()}"))
            continue
        flags, notes = [], [f"{badcrc} bad-CRC and {unparsable} unparsable records in the file dropped" + (f", {junk} records with a corrupt board timestamp dropped" if junk else "")]
        refs_t = [t for t, _ in alt]
        refs = [d["press_ref"] for _, d in alt]
        ref_scale = ATM if med(refs) < 2 else 1.0

        def press_ref_at(t):
            i = max(0, bisect_left(refs_t, t) - 1)
            return refs[i] * ref_scale

        pch = [(t, std_p(d["fcb_altitude"] + std_alt(press_ref_at(t)))) for t, d in pos if num(d.get("fcb_altitude")) is not None]
        tch = [(t, d["temperature"] + 273.15) for t, d in pos if num(d.get("temperature")) is not None]
        has_accel = any(abs(d.get("accel_x", 0)) + abs(d.get("accel_y", 0)) + abs(d.get("accel_z", 0)) > 0.5 for _, d in ori)
        has_gyro = has_accel
        ach = [(t, (d["accel_x"], d["accel_y"], d["accel_z"], d["rot_vel_x"], d["rot_vel_y"], d["rot_vel_z"])) for t, d in ori] if has_accel else []
        flags.append("pressure rebuilt from telemetry fcb_altitude + press_ref (standard atmosphere); raw barometer_pressure only comes ~1 Hz")
        if not has_accel:
            flags.append("no accel: Orientation packets carry all-zero accel/gyro - constant (0,0,g), gyro 0")

        def interp(series, t, dim=None):
            ts = [s[0] for s in series]
            i = bisect_left(ts, t)
            if i <= 0:
                return series[0][1]
            if i >= len(series):
                return series[-1][1]
            (t0, v0), (t1, v1) = series[i - 1], series[i]
            if t1 == t0:
                return v1
            w = (t - t0) / (t1 - t0)
            if isinstance(v0, tuple):
                return tuple(a + (b - a) * w for a, b in zip(v0, v1))
            return v0 + (v1 - v0) * w

        def dedupe(series):
            outl = []
            for s in series:
                if outl and s[0] == outl[-1][0]:
                    outl[-1] = s
                else:
                    outl.append(s)
            return outl

        pch, tch, ach = dedupe(pch), dedupe(tch), dedupe(ach)
        times = sorted(set([t for t, _ in pch] + [t for t, _ in ach]))
        rows = []
        for t in times:
            a = interp(ach, t) if ach else (0.0, 0.0, G, 0.0, 0.0, 0.0)
            rows.append([float(t), interp(pch, t), interp(tch, t) if tch else 288.15, *a])
        states = sorted((d["time_stamp_ms"], d.get("fcb_state_number")) for _, k, d in rs if "fcb_state_number" in d)
        ev = state_events([s[0] for s in states], [s[1] for s in states], "fcb_state_number", persist=3)
        out.append(segment(rows, ev, flags=flags, notes=notes, has_accel=has_accel, has_gyro=has_gyro,
                           accel_kind="imu (telemetry)" if has_accel else "none"))
    return out, "FCB ground-station telemetry capture (GroundStationDataInterface_parsed)", {}


# ---------------------------------------------------------------- format: Altus Metrum AltOS CSV
def parse_altos(path, attrs):
    with open(path, encoding="utf-8", errors="replace", newline="") as f:
        lines = [l for l in f if l.strip()]
    hdr = [h.strip().lstrip("#") for h in next(csv.reader([lines[0]]))]
    ix = {h: i for i, h in enumerate(hdr)}
    flights = {}
    for r in csv.reader(lines[1:]):
        r = [x.strip() for x in r]
        if len(r) < len(hdr):
            continue
        flights.setdefault(r[ix["flight"]], []).append(r)
    product = attrs.get("altimeter_name", "")
    easymini = "easymini" in (product + os.path.basename(path)).lower().replace(" ", "") or "ezmini" in os.path.basename(path).lower().replace(" ", "")
    out = []
    for fl, rs in flights.items():
        rows, states = [], []
        for r in rs:
            t, p, a, temp = (num(r[ix[k]]) for k in ("time", "pressure", "acceleration", "temperature"))
            if None in (t, p, a):
                continue
            rows.append([t * 1000.0, p, temp + 273.15 if temp is not None else None, 0.0, 0.0, a + G, 0.0, 0.0, 0.0])
            states.append(r[ix["state_name"]])
        alt0, h0 = num(rs[0][ix["altitude"]]), num(rs[0][ix["height"]])
        flags = ["no gyro"]
        if easymini:
            flags.append("baro-derived accel: EasyMini has no accelerometer; its 'acceleration' column is differentiated from pressure (+g on z)")
        ev = state_events([r[0] for r in rows], states, "AltOS state")
        out.append(segment(rows, ev, flags=flags, notes=[f"AltOS flight {fl}, serial {rs[0][ix['serial']]}; acceleration is axial with gravity removed -> z = a + g"],
                           has_gyro=False, accel_kind="baro-derived" if easymini else "axial",
                           pad_pressure_hint=std_p(alt0 - h0) if None not in (alt0, h0) else None,
                           pad_accel_hint=(0.0, 0.0, G), launch_hint_ms=0.0))
    return out, "Altus Metrum AltOS CSV" + (" (EasyMini)" if easymini else ""), {}


# ---------------------------------------------------------------- format: PerfectFlite StratoLogger
def parse_stratologger(path, attrs):
    head, rows_raw = {}, []
    for line in open(path, encoding="utf-8", errors="replace"):
        s = line.strip()
        m = re.match(r"^([A-Za-z ]+):\s*(.*)$", s)
        if m and not s[0].isdigit():
            head[m.group(1).strip()] = m.group(2).strip()
            continue
        parts = [num(x) for x in s.split(",")]
        if len(parts) >= 4 and None not in parts[:4]:
            rows_raw.append(parts)
    ground_ft = num(re.sub(r"[^-\d.]", "", head.get("Ground Elevation", "0"))) or 0.0
    rows = [[t * 1000.0, std_p((ground_ft + alt) * 0.3048), (tf - 32) * 5 / 9 + 273.15, 0.0, 0.0, G, 0.0, 0.0, 0.0] for t, alt, vel, tf, *_ in rows_raw]
    ev = []
    for k in ("Drogue At", "Main At"):
        v = num(re.sub(r"[^\d.]", "", head.get(k, "")))
        if v is not None:
            ev.append((v * 1000.0, f"StratoLogger header '{k}: {head[k]}'"))
    if "Apogee" in head:
        ev.append((None, f"StratoLogger header 'Apogee: {head['Apogee']}'"))
    flags = ["no accel: constant (0,0,g)", "no gyro",
             f"altitude-converted: pressure from baro altitude (ft AGL) + header ground elevation {ground_ft:.0f} ft MSL via standard atmosphere",
             "altitude quantized to 1 ft; temperature degF -> K"]
    seg = segment(rows, ev, flags=flags, has_accel=False, has_gyro=False, accel_kind="none",
                  pad_pressure_hint=std_p(ground_ft * 0.3048), pad_accel_hint=(0.0, 0.0, G), launch_hint_ms=0.0,
                  notes=[f"serial {head.get('Serial Number')}, flight {head.get('Flight Number')}, t=0 is the StratoLogger's own launch detection"])
    return [seg], "PerfectFlite StratoLoggerCF text export", head


# ---------------------------------------------------------------- common processing
def clean(rows):
    stats = Counter()
    good = []
    for r in rows:
        if any(v is None or not math.isfinite(v) for v in r[:2]) or any(v is not None and not math.isfinite(v) for v in r[2:9]):
            stats["non-finite"] += 1
            continue
        if not 5000 < r[1] < 115000:
            stats["pressure out of range"] += 1
            continue
        good.append(r)
    # fill missing temperature / accel / gyro with the previous value
    last = [None] * 9
    for r in good:
        for k in range(2, 9):
            if r[k] is None:
                r[k] = last[k]
            last[k] = r[k]
    first = next((r for r in good if None not in r[2:9]), None)
    if first:
        for r in good:
            for k in range(2, 9):
                if r[k] is None:
                    r[k] = first[k]
    good.sort(key=lambda r: r[0])
    dedup = []
    for r in good:
        if dedup and abs(r[0] - dedup[-1][0]) < 0.5:
            dedup[-1] = r
            stats["duplicate timestamp"] += 1
        else:
            dedup.append(r)
    out = []
    for i, r in enumerate(dedup):
        if 0 < i < len(dedup) - 1:
            a, b = dedup[i - 1][1], dedup[i + 1][1]
            if abs(r[1] - a) > 4000 and abs(r[1] - b) > 4000 and abs(a - b) < 2000:
                stats["single-sample pressure spike"] += 1
                continue
        out.append(r)
    return out, dict(stats)


def detect_flights(rows, has_accel):
    """Ascents: altitude more than 30 m above its 20 s running minimum, grouped (gap < 120 s)."""
    t = [r[0] for r in rows]
    alt = [std_alt(r[1]) for r in rows]
    als = [statistics.median(alt[max(0, i - 2):i + 3]) for i in range(len(alt))]  # robust to impact/ejection spikes
    win, cand = deque(), []
    for i in range(len(rows)):
        while win and als[win[-1]] >= als[i]:
            win.pop()
        win.append(i)
        while t[win[0]] < t[i] - 20000:
            win.popleft()
        if als[i] - als[win[0]] > 30:
            cand.append(i)
    groups = []
    for i in cand:
        if groups and t[i] - t[groups[-1][-1]] < 120000:
            groups[-1].append(i)
        else:
            groups.append([i])
    flights = []
    for g in groups:
        if len(g) < 3:
            continue
        i0 = g[0]
        tc = t[i0]
        padw = [rows[k][1] for k in range(len(rows)) if tc - 30000 <= t[k] <= tc - 8000]
        if not padw:
            padw = [max(rows[k][1] for k in range(max(0, i0 - 200), i0 + 1))]
        p_pad = med(padw)
        a_pad = std_alt(p_pad)
        if has_accel:
            near = [k for k in range(len(rows)) if tc - 10000 <= t[k] <= tc + 2000]
            if not any(math.sqrt(sum(rows[k][j] ** 2 for j in (3, 4, 5))) > 2 * G for k in near):
                continue  # a climb without boost acceleration: car ride, weather, carried up a hill
        launch = None
        if has_accel:
            for k in range(len(rows)):
                if tc - 8000 <= t[k] <= tc and math.sqrt(sum(rows[k][j] ** 2 for j in (3, 4, 5))) > 2 * G:
                    launch = t[k]
                    break
        if launch is None:
            k = i0
            while k > 0 and alt[k] - a_pad > 3 and t[k] > tc - 15000:
                k -= 1
            launch = t[k]
        flights.append({"launch": launch, "p_pad": p_pad, "a_pad": a_pad})
    # apogee and landing per flight
    for n, f in enumerate(flights):
        end_t = flights[n + 1]["launch"] if n + 1 < len(flights) else float("inf")
        idx = [k for k in range(len(rows)) if f["launch"] <= t[k] < end_t]
        sm = {}
        for j, k in enumerate(idx):
            sm[k] = med([alt[idx[q]] for q in range(max(0, j - 2), min(len(idx), j + 3))])
        ka = max(idx, key=lambda k: sm[k])
        f["apogee"], f["apogee_alt"] = t[ka], sm[ka] - f["a_pad"]
        f["landing"] = None
        for j, k in enumerate(idx):
            if t[k] <= f["apogee"] or als[k] - f["a_pad"] > 40:
                continue
            nxt = [als[q] for q in idx[j:] if t[q] <= t[k] + 10000]
            if t[idx[-1]] >= t[k] + min(10000, max(3000, t[idx[-1]] - t[k])) and t[idx[-1]] - t[k] >= 3000 and max(nxt) - min(nxt) < 6:
                f["landing"] = t[k]
                break
    return flights


def to_sim_grid(rows):
    """The sim plays every row closer than 30 ms to the previous one as exactly one 10 ms tick, and
    interpolates longer gaps at 10 ms. So: stretches of >= 5 rows that are <= 30 ms apart are resampled
    onto an exact 10 ms grid (linear interpolation); shorter clusters collapse to one row; rows outside
    them are kept on the same grid phase and more than 30 ms apart, so they are interpolated, not compressed."""
    n = len(rows)
    dense_first = next((i for i in range(1, n) if rows[i][0] - rows[i - 1][0] <= 30), None)
    phase = (rows[dense_first][0] if dense_first is not None else rows[0][0]) % 10
    snap = lambda t: int(round((t - phase) / 10.0)) * 10 + phase
    out, i = [], 0
    while i < n:
        j = i
        while j + 1 < n and rows[j + 1][0] - rows[j][0] <= 30:
            j += 1
        if j - i + 1 >= 5:
            g = snap(rows[i][0])
            if g < rows[i][0]:
                g += 10
            if out and g - out[-1][0] <= 30:  # continue the previous row's grid instead of a short step
                g = out[-1][0] + 10
            k = i
            while g <= rows[j][0]:
                while k + 1 <= j and rows[k + 1][0] < g:
                    k += 1
                a, b = rows[k], rows[min(k + 1, j)]
                w = 0.0 if b[0] == a[0] else max(0.0, min(1.0, (g - a[0]) / (b[0] - a[0])))
                out.append([g] + [x + (y - x) * w for x, y in zip(a[1:], b[1:])])
                g += 10
        else:
            cl = rows[i:j + 1]
            r = [snap(sum(x[0] for x in cl) / len(cl))] + [sum(x[c] for x in cl) / len(cl) for c in range(1, 9)]
            if not out or r[0] - out[-1][0] > 30:
                out.append(r)
        i = j + 1
    return out


def rate_hz(times):
    d = [b - a for a, b in zip(times, times[1:]) if b > a]
    m = med(d)
    return round(1000.0 / m, 2) if m else None


def finish(seg, rows, fl, name, fmt, src_rel, dup, attrs, extra_notes):
    flags, notes = list(seg["flags"]), list(seg["notes"]) + extra_notes
    t = [r[0] for r in rows]
    launch = fl["launch"] if fl else None
    pad_real_s = (launch - t[0]) / 1000.0 if launch is not None else None
    synth_s = 0.0
    if launch is not None and pad_real_s < MIN_REAL_PAD_S:
        pre = [r for r in rows if r[0] < launch - 200]
        p_pad = seg["pad_pressure_hint"] or (med([r[1] for r in pre]) if len(pre) >= 3 else rows[0][1])
        if seg["pad_accel_hint"]:
            acc = seg["pad_accel_hint"]
        elif len(pre) >= 3:
            acc = tuple(med([r[j] for r in pre]) for j in (3, 4, 5))
        else:
            acc = (0.0, 0.0, G)
        t0 = t[0]
        pad = [[t0 - SYNTH_PAD_MS + k * SYNTH_PAD_STEP_MS, p_pad, rows[0][2], *acc, 0.0, 0.0, 0.0] for k in range(SYNTH_PAD_MS // SYNTH_PAD_STEP_MS)]
        pad[-1][0] = t0 - 10
        rows = pad + rows
        synth_s = SYNTH_PAD_MS / 1000.0
        fl = dict(fl, p_pad=p_pad)
        flags.append(f"synthetic pad: {synth_s:.0f} s of constant pad (p {p_pad:.0f} Pa, accel {tuple(round(a, 2) for a in acc)}) prepended - real log has {pad_real_s:.1f} s before launch")
    offset = 0.0
    if rows[0][0] < 0 or rows[0][0] != int(rows[0][0]) or synth_s:
        offset = -rows[0][0]
    out_rows, last = [], None
    for r in rows:
        tt = int(round(r[0] + offset))
        if last is not None and tt <= last:
            continue
        out_rows.append([tt] + r[1:])
        last = tt
    if out_rows[-1][0] >= 2 ** 32:
        raise SystemExit(f"{name}: timestamps overflow uint32")
    o = lambda x: None if x is None else int(round(x + offset))
    lo, la, ap = o(launch), o(fl["landing"]) if fl else None, o(fl["apogee"]) if fl else None
    if lo is not None:
        # Boards without a pre-launch buffer jump from slow pad logging straight into boost: the sim would
        # interpolate a seconds-long ramp from the last pad row into the boost and launch early. Hold the
        # last pad row until 40 ms before dense logging starts.
        for d in range(1, len(out_rows) - 1):
            t_d = out_rows[d][0]
            if t_d > lo + 1000:
                break
            if t_d >= lo - 5000 and t_d - out_rows[d - 1][0] > 1000 and out_rows[d + 1][0] - t_d <= 300:
                hold = [t_d - 40] + list(out_rows[d - 1][1:])
                out_rows.insert(d, hold)
                notes.append(f"pad hold row added 40 ms before fast logging starts at {(t_d - lo) / 1000:+.2f} s from launch "
                             f"(the source jumps from slow pad logging, {(t_d - out_rows[d - 1][0]) / 1000:.1f} s gap, straight into the flight)")
                break
    if lo is not None:
        early = [r[1] for r in out_rows if lo <= r[0] <= lo + 1000]
        if len(early) >= 3 and early[-1] - early[0] > 100:
            flags.append(f"pressure rises {early[-1] - early[0]:.0f} Pa during the first second of boost (baro disturbed or log reconstructed)")
    wr = to_sim_grid(out_rows)
    if len(wr) != len(out_rows) or any(a[0] != b[0] for a, b in zip(wr, out_rows)):
        notes.append(f"resampled for the sim's tick model: dense stretches put on an exact 10 ms grid ({len(out_rows)} source rows -> {len(wr)} rows), isolated rows kept >30 ms apart")
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, name + ".txt"), "w", encoding="utf-8", newline="\n") as f:
        f.write(HEADER + "\n")
        for r in wr:
            f.write("%d\t%.2f\t%.2f\t%.4f\t%.4f\t%.4f\t%.5f\t%.5f\t%.5f\n" % tuple(r))
    T = [r[0] for r in out_rows]
    if fl and fl["landing"] is None:
        flags.append("log ends before landing")
    first_real = T[0] + int(synth_s * 1000)
    pad_t = [x for x in T if first_real <= x < (lo - 500 if lo is not None else T[-1] + 1)]
    fl_t = [x for x in T if lo is not None and lo <= x <= (la if la is not None else T[-1])]
    post_t = [x for x in T if la is not None and x > la]
    # gaps: longer than 2 s, not counting the source's own slow ground-logging interval (SillyGoose: 5 s)
    slow_dt = Counter(round((b - a) / 100) * 100 for a, b in zip(T, T[1:]) if b - a > 1000)
    ground_dt = slow_dt.most_common(1)[0][0] if slow_dt and slow_dt.most_common(1)[0][1] >= 10 else None
    if ground_dt:
        notes.append(f"source logs the ground every {ground_dt / 1000:.1f} s")
    for label, ts in (("pad", pad_t), ("flight", fl_t), ("post-landing", post_t)):
        gaps = [(a, b) for a, b in zip(ts, ts[1:]) if b - a > 2000 and not (ground_dt and abs(b - a - ground_dt) <= 0.1 * ground_dt)]
        if gaps:
            where = ", ".join(f"{(a - lo) / 1000 if lo is not None else a / 1000:+.1f} s ({(b - a) / 1000:.1f} s)" for a, b in gaps[:4])
            flags.append(f"gaps in {label}: {len(gaps)} > 2 s, largest {max(b - a for a, b in gaps) / 1000:.1f} s; first at {where}{' (times from launch)' if lo is not None else ''}")
    slow = [b - a for a, b in zip(fl_t, fl_t[1:])]
    if lo is not None and len(slow) > 20 and med(slow) <= 30 and any(x >= 1000 for x in slow):
        k = next(i for i, x in enumerate(slow) if x >= 1000)
        flags.append(f"flight logging drops to the ground rate {(fl_t[k] - lo) / 1000:.1f} s after launch (before landing)")
    events = []
    for et, txt in seg["events"]:
        if et is None:
            events.append({"event": txt})
            continue
        ot = int(round(et + offset))
        if T[0] - 1 <= ot <= T[-1] + 1:
            events.append({"t_ms": ot, "t_from_launch_s": round((ot - lo) / 1000, 3) if lo is not None else None, "event": txt})
    entry = {
        "file": name + ".txt",
        "source": src_rel,
        "source_duplicates": dup,
        "format": fmt,
        "rows": len(wr),
        "source_rows": len(out_rows),
        "time_offset_ms": offset,
        "total_duration_s": round((T[-1] - T[0]) / 1000, 2),
        "pad_duration_s": round((lo - first_real) / 1000, 2) if lo is not None else None,
        "synthetic_pad_s": synth_s,
        "launch_ms": lo,
        "apogee_ms": ap,
        "apogee_m_agl_est": round(fl["apogee_alt"], 1) if fl else None,
        "landing_ms": la,
        "flight_duration_s": round(((la if la is not None else T[-1]) - lo) / 1000, 2) if lo is not None else None,
        "post_landing_s": round((T[-1] - la) / 1000, 2) if la is not None else None,
        "rate_hz": {"pad": rate_hz(pad_t), "flight": rate_hz(fl_t), "post_landing": rate_hz(post_t)},
        "has_accel": seg["has_accel"],
        "accel_kind": seg["accel_kind"],
        "has_gyro": seg["has_gyro"],
        "has_temperature": seg["has_temp"],
        "pad_pressure_pa": round(fl["p_pad"], 1) if fl else round(out_rows[0][1], 1),
        "flags": flags,
        "notes": notes,
        "source_events": events,
        "source_attributes": {k: v for k, v in attrs.items() if k in ("altimeter_name", "altimeter_note", "flight_notes", "peak_altitude_m", "BOARD_NAME") and v},
    }
    if lo is None:
        entry["flags"].append("no launch in this log")
        drop = std_alt(out_rows[0][1]) - std_alt(out_rows[-1][1])
        if drop > 30:
            entry["flags"].append(f"starts in the air: first row {drop:.0f} m above the last (log begins mid-descent, e.g. after an in-flight reboot)")
    return entry


def process(path, src_rel, dup, name_base, fmt_fn, attrs, manifest, skipped):
    segs, fmt, _ = fmt_fn(path, attrs)
    if not segs:
        skipped.append({"source": src_rel, "reason": "no parsable rows"})
        return
    multi_seg = len(segs) > 1
    for si, seg in enumerate(segs):
        rows, dropped = clean(seg["rows"])
        notes = [f"dropped rows: {dropped}"] if dropped else []
        if multi_seg:
            notes.append(f"segment {si + 1} of {len(segs)} in the source (board reboot / clock reset)")
        if len(rows) < 10:
            skipped.append({"source": src_rel, "segment": si + 1, "reason": seg.get("skip_reason") or f"only {len(rows)} usable rows"})
            continue
        flights = detect_flights(rows, seg["has_accel"] and seg["accel_kind"] != "baro-derived")
        base = name_base + (f"_boot{si + 1}" if multi_seg else "")
        if not flights:
            span = max(std_alt(r[1]) for r in rows) - min(std_alt(r[1]) for r in rows)
            dense = sum(1 for a, b in zip(rows, rows[1:]) if b[0] - a[0] <= 30)
            if span < 30:
                skipped.append({"source": src_rel, "segment": si + 1 if multi_seg else None,
                                "reason": f"ground only: no flight ({len(rows)} rows over {(rows[-1][0] - rows[0][0]) / 1000:.0f} s, altitude span {span:.1f} m, {dense} dense rows)"})
                continue
            manifest.append(dict(finish(seg, rows, None, base, fmt, src_rel, dup, attrs, notes), name=base,
                                 altitude_span_m=round(span, 1)))
            continue
        bounds = []
        for n in range(len(flights)):
            lo = -float("inf") if n == 0 else bounds[-1][1]
            if n + 1 < len(flights):
                prev_end = flights[n]["landing"] or flights[n]["apogee"]
                hi = (prev_end + flights[n + 1]["launch"]) / 2
            else:
                hi = float("inf")
            bounds.append((lo, hi))
        for n, (fl, (lo, hi)) in enumerate(zip(flights, bounds)):
            part = [list(r) for r in rows if lo <= r[0] < hi]
            nm = base + (f"_flight{n + 1}" if len(flights) > 1 else "")
            nt = list(notes) + ([f"flight {n + 1} of {len(flights)} in this source, split at the ground time between them"] if len(flights) > 1 else [])
            e = finish(seg, part, fl, nm, fmt, src_rel, dup, attrs, nt)
            if len(flights) > 1:
                e["flags"].append(f"multiple flights in one source: this is flight {n + 1} of {len(flights)}")
            e["name"] = nm
            manifest.append(e)


def detect_format(path):
    base = os.path.basename(path).lower()
    if base.endswith(".zip"):
        return None, "zip archive containing 'Rocket Flight Data-0.1.0-portable.exe' (the importer application), not a log"
    with open(path, "rb") as f:
        head = f.read(4096).decode("utf-8", errors="replace")
    first = head.lstrip().split("\n", 1)[0]
    if head.lstrip().startswith("{") and "altusmetrum" in open(path, encoding="utf-8", errors="replace").read(4000):
        return None, "raw AltOS EEPROM image (JSON config + hex dump); it holds only flight 2, the same flight as gianniL3.csv (decoded AltOS CSV), which is converted"
    if first.startswith("#version"):
        return parse_altos, None
    if first.startswith("PerfectFlite"):
        return parse_stratologger, None
    if "RUN START" in head and "Orientation" in head or "_parsed.txt" in base and "RUN START" in head:
        return parse_fcb_gs, None
    if first.startswith(",") and "imu1_accel_x" in first:
        return parse_fcb_post, None
    if first.startswith("timestamp,latitude,longitude") or re.match(r"^\d{10}\.\d+,-?\d+\.\d+,-?\d+\.\d+,-?[\d.]+\s*$", first):
        return None, "GPS-only track (epoch, lat, lon, alt) - no barometer/IMU, nothing the sim can use"
    if first.startswith("CONFIG") or first.startswith("timestampMs"):
        return parse_sillygoose, None
    return None, f"unrecognised format (first line: {first[:80]!r})"


def norm_hash(path):
    with open(path, "rb") as f:
        return hashlib.sha1(f.read().replace(b"\r\n", b"\n")).hexdigest()


def main():
    global OUT
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", default=os.path.abspath(os.path.join(REPO, "../rocket-flight-data")))
    ap.add_argument("--out", default=os.path.join(REPO, "agent_work/logs/rocket_flight_data_sim"))
    ap.add_argument("--sim", default=None, help="desktop.exe to smoke-test every output with")
    args = ap.parse_args()
    OUT = args.out
    os.makedirs(OUT, exist_ok=True)
    for f in os.listdir(OUT):
        if f.endswith(".txt"):
            os.remove(os.path.join(OUT, f))
    files = []
    for top in ("flight-data", "data"):  # flight-data first: it is the canonical copy of duplicates
        for d, _, fs in os.walk(os.path.join(args.src, top)):
            for fn in sorted(fs):
                if fn != "attributes.csv" and not fn.endswith("-metadata.json"):
                    files.append(os.path.join(d, fn))
    seen, manifest, skipped = {}, [], []
    for path in sorted(files, key=lambda p: (0 if os.sep + "flight-data" + os.sep in p or "/flight-data/" in p.replace("\\", "/") else 1, p)):
        rel = os.path.relpath(path, args.src).replace("\\", "/")
        h = norm_hash(path)
        if h in seen:
            seen[h]["dups"].append(rel)
            continue
        entry = {"dups": []}
        seen[h] = entry
        fn, reason = detect_format(path)
        if fn is None:
            entry["skip"] = {"source": rel, "reason": reason}
            skipped.append(entry["skip"])
            continue
        parts = rel.split("/")
        if parts[0] == "flight-data":
            name = slug(parts[1]) + "__" + slug(" ".join(parts[2:-1]) or os.path.splitext(parts[-1])[0])
        else:
            name = "data__" + slug(parts[1]) + "__" + slug(os.path.splitext(parts[-1])[0])
        attrs = read_attributes(os.path.dirname(path))
        n0 = len(manifest)
        process(path, rel, entry["dups"], name, fn, attrs, manifest, skipped)
        entry["outputs"] = manifest[n0:]
    for e in manifest:
        e["source_duplicates"] = sorted(set(e["source_duplicates"]))
    for e in skipped:
        for v in seen.values():
            if v.get("skip") is e and v["dups"]:
                e["duplicates"] = v["dups"]
    if args.sim:
        sim_check(args.sim, manifest)
    with open(os.path.join(OUT, "manifest.json"), "w", encoding="utf-8") as f:
        json.dump({"generated_by": "agent_work/scripts/ci_data/convert_rocket_flight_data.py", "source_root": args.src.replace("\\", "/"),
                   "columns": HEADER.split("\t"), "outputs": manifest, "skipped": skipped}, f, indent=1)
    for e in manifest:
        print(f"{e['name']:<60} {e['format'][:30]:<30} pad {e['pad_duration_s']} s  flight {e['flight_duration_s']} s  rates {e['rate_hz']}  flags {len(e['flags'])}")
    for s in skipped:
        print(f"SKIP {s['source']}: {s['reason']}")


def sim_check(exe, manifest):
    """Fly every output through the desktop build; record the events it finds on the output file's clock
    (tests/sim/check_sim.py's own trim/event logic). Needs the build's runtime DLLs on PATH."""
    sys.dont_write_bytecode = True
    sys.path.insert(0, os.path.join(REPO, "tests/sim"))
    import check_sim  # noqa: E402
    work = os.path.join(OUT, "_sim_check")
    os.makedirs(work, exist_ok=True)
    for e in manifest:
        inp, out = os.path.join(OUT, e["file"]), os.path.join(work, e["name"] + ".out.txt")
        try:
            subprocess.run([exe, inp, out], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=3600)
        except Exception as ex:  # noqa: BLE001
            e["sim_check"] = {"error": str(ex)}
            print(f"sim {e['name']}: ERROR {ex}")
            continue
        start = check_sim.sim_start_ms(check_sim.read_text(inp))
        rows = check_sim.flight_rows(check_sim.read_text(out), [])
        found = check_sim.events(rows, start)
        lo = e["launch_ms"]
        rel = {k: (round(v - lo / 1000, 2) if lo is not None and k != "main_altitude" else round(v, 2)) for k, v in found.items()}
        e["sim_check"] = {"sim_start_ms": start, "ticks": len(rows), "events_s": {k: round(v, 2) for k, v in found.items()},
                          "events_from_launch_estimate_s": rel, "invariant_failures": check_sim.invariant_failures(rows, False)}
        print(f"sim {e['name']}: {rel}")
        os.remove(out)


if __name__ == "__main__":
    main()
