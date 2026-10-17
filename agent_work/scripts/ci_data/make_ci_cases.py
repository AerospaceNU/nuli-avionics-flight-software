# Builds tests/sim/data/*.txt.gz and tests/sim/cases.json for the desktop-sim CI check.
#   real in-house logs (AerospaceNU/rocket-flight-data, GPL-3 like this repo): first 9 columns, trimmed to
#     the part the sim uses; windows = the validated desktop result +- tolerance (printed next to the
#     original board's events for review)
#   MIT RocketPy logs (AltOS CSV): converted to board columns, 60 s of pad prepended
#   synthetic: stress_test.exe emit, windows from the flight's truth
# usage: python -I make_ci_cases.py DESKTOP_EXE
import gzip, json, os, random, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "../../.."))
OUT = os.path.join(REPO, "tests/sim/data")
FLIGHT_DATA = os.path.abspath(os.path.join(REPO, "../rocket-flight-data/flight-data"))
EXTERNAL = os.path.join(REPO, "agent_work/logs/external_data")
STRESS = os.path.join(REPO, "agent_work/scripts/flight_logic_tests/stress_test.exe")
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(REPO, "tests/sim"))
import check_sim  # noqa: E402

DESKTOP = sys.argv[1]
G = 9.80665
HEADER = "timestampMs\tpressurePa\tbarometerTemperatureK\taccelerationMSS_x\taccelerationMSS_y\taccelerationMSS_z\tvelocityRadS_x\tvelocityRadS_y\tvelocityRadS_z"
cases = []


def rows_of(path):
    rows = []
    for line in open(path, encoding="utf-8", errors="replace"):
        p = line.strip().split("\t")
        if not p or not p[0][:1].isdigit() or len(p) < 9:
            continue
        try:
            rows.append([float(x) for x in p[:9]])
        except ValueError:
            pass
    rows.sort(key=lambda r: r[0])
    return rows


def dense_span(rows):
    dense = [i for i in range(1, len(rows)) if rows[i][0] - rows[i - 1][0] <= 30]
    return rows[dense[0]][0], rows[dense[-1]][0]


def open_gz(path, newline=None):
    import io
    return io.TextIOWrapper(gzip.GzipFile(path, "wb", compresslevel=9, mtime=0), encoding="utf-8", newline=newline)


def write_log(name, rows, fmt="%.0f\t%.6g\t%.6g\t%.6g\t%.6g\t%.6g\t%.6g\t%.6g\t%.6g"):
    path = os.path.join(OUT, name + ".txt.gz")
    with open_gz(path) as f:
        f.write(HEADER + "\n")
        for r in rows:
            f.write(fmt % tuple(r) + "\n")
    return name + ".txt.gz"


def run(case):
    import tempfile
    with tempfile.TemporaryDirectory() as work:
        failures, summary = check_sim.check_case(DESKTOP, case, OUT, work)
        text = check_sim.read_text(os.path.join(OUT, case["input"]))
        start = check_sim.sim_start_ms(text)
        out = os.path.join(work, case["name"] + ".out.txt")
        rows = check_sim.flight_rows(check_sim.read_text(out), sorted(case.get("reboot_at_ms", [])))
        return check_sim.events(rows, start), failures, summary


def windows_from(found, tolerances):
    return {k: ([round(found[k] - tol[0], 2), round(found[k] + tol[1], 2)] if k in found else None) for k, tol in tolerances.items()}


REAL_TOLERANCE = {"launch": (0.3, 0.3), "apogee": (0.5, 0.5), "drogue": (0.5, 0.5), "main": (1.0, 1.0), "main_altitude": (15, 15), "landed": (6, 6)}


def real_case(name, rel_path, note, reboot=None):
    rows = rows_of(os.path.join(FLIGHT_DATA, rel_path)) if reboot is None else reboot
    first, last = dense_span(rows)
    rows = [r for r in rows if first - 40000 <= r[0] <= last + 120000]
    d = next(i for i in range(1, len(rows)) if rows[i][0] - rows[i - 1][0] <= 30) - 1
    if d > 0 and rows[d][0] - rows[d - 1][0] > 30:  # pad row then straight into boost: hold the pad values or the sim interpolates a climb
        rows.insert(d, [rows[d][0] - 40] + rows[d - 1][1:])
    case = {"name": name, "input": write_log(name, rows), "source": note, "expect": {}}
    if isinstance(reboot, list):
        case["reboot_at_ms"] = [HAYBALES_REBOOT_INPUT_MS - int(check_sim.sim_start_ms(check_sim.read_text(os.path.join(OUT, case["input"]))))]
    found, _, summary = run(case)
    case["expect"] = windows_from(found, REAL_TOLERANCE)
    print(f"real {name}: {summary}")
    cases.append(case)


def altos_case(name, rel_path, note):
    import csv
    lines = [l for l in open(os.path.join(EXTERNAL, rel_path), encoding="utf-8", errors="replace") if not l.startswith("# ")]
    hdr = [h.strip().lstrip("#") for h in next(csv.reader([lines[0]]))]
    col = {}
    for i, h in enumerate(hdr):
        col.setdefault(h, i)
    flight = []
    for r in csv.reader(lines[1:]):
        try:
            t, p, a, temp = (float(r[col[k]]) for k in ("time", "pressure", "acceleration", "temperature"))
        except (ValueError, IndexError):
            continue
        flight.append([60000 + t * 1000, p, temp + 273.15, 0.0, 0.0, a + G, 0.0, 0.0, 0.0])
    pad = flight[0][1:]
    rows = [[t] + pad[:4] + [G] + [0, 0, 0] for t in range(0, 59000, 5000)] + flight
    case = {"name": name, "input": write_log(name, rows), "source": note, "expect": {}}
    found, _, summary = run(case)
    case["expect"] = windows_from(found, REAL_TOLERANCE)
    print(f"altos {name}: {summary}")
    cases.append(case)


def emit(seed, scenario, env=None):
    path = os.path.join(OUT, "_emit.txt")
    e = dict(os.environ, **(env or {}))
    truth = json.loads(subprocess.run([STRESS, "emit", str(seed), scenario, path], capture_output=True, text=True, env=e, check=True).stdout)
    return truth, rows_of(path)


def pick(scenario, predicate, start=1, env=None):
    for seed in range(start, start + 3000):
        truth, rows = emit(seed, scenario, env)
        if predicate(truth):
            return truth, rows
    raise SystemExit(f"no seed for {scenario}")


def synthetic_case(name, truth, rows, note, transform=None, reboot_input_s=None, expect=None, slack=3.0, land_slack=30, input_of=None):
    if transform:
        rows = transform(rows, truth)
    if input_of:
        log = next(c["input"] for c in cases if c["name"] == input_of)
    else:
        log = write_log(name, rows, "%.0f\t%.1f\t%.2f\t%.4f\t%.4f\t%.4f\t%.5f\t%.5f\t%.5f")
    case = {"name": name, "input": log, "source": note}
    L, A, T = truth["liftoff_s"], truth["apogee_s"], truth["touchdown_s"]
    case["expect"] = expect if expect is not None else {
        "launch": [round(L, 2), round(L + 1.0, 2)], "apogee": [round(A - 1.0, 2), round(A + slack, 2)],
        "main": [round(A, 2), round(T, 2)], "main_altitude": [140, 210], "landed": [round(T, 2), round(T + land_slack, 2)]}
    if reboot_input_s is not None:
        start = check_sim.sim_start_ms(check_sim.read_text(os.path.join(OUT, case["input"])))
        case["reboot_at_ms"] = [int(round(reboot_input_s * 1000 - start, -1))]
    case["truth"] = {k: truth[k] for k in ("seed", "scenario", "apogee_m", "max_speed_ms", "max_mach", "ground_elevation_m", "ground_temperature_k")}
    found, failures, summary = run(case)
    print(f"syn  {name}: {'FAIL ' + '; '.join(failures) if failures else 'ok'}  {summary}  | truth L {L:.2f} A {A:.2f} T {T:.2f}")
    cases.append(case)


def reboot_cases(base, truth, rows, times_s, note):
    for reboot_s in times_s:
        synthetic_case(f"{base}_reboot_{reboot_s:.2f}".replace(".", "_"), truth, rows, f"{base} rebooted at {reboot_s:.2f} s: {note}",
                       reboot_input_s=reboot_s, slack=6.0, land_slack=40, input_of=base)


def zero_pressure(after_s=None, every_s=None):
    def f(rows, truth):
        out = []
        for i, r in enumerate(rows):
            r = list(r)
            t = r[0] / 1000
            dense = 0 < i < len(rows) - 1 and rows[i + 1][0] - rows[i - 1][0] <= 20  # a 0 Pa row in a sparse span would interpolate into a valid-looking ramp
            if after_s is None and every_s is None:
                r[1] = 0
            elif after_s is not None and t >= truth["liftoff_s"] + after_s:
                r[1] = 0
            elif every_s is not None and truth["liftoff_s"] < t < truth["touchdown_s"] and dense and int(r[0]) % int(every_s * 1000) == 500:
                r[1] = 0
            out.append(r)
        return out
    return f


os.makedirs(OUT, exist_ok=True)
for f in os.listdir(OUT):
    if f.endswith(".txt.gz"):
        os.remove(os.path.join(OUT, f))

# ---- real in-house logs
real_case("mbta_board1", "2025-11-15 MBTA/SillyGoose Ridealong 1/2025-11-15 V1 MBTA ridealong board1.txt", "AerospaceNU 2025-11-15 MBTA, SillyGoose V1 ridealong 1")
real_case("mbta_board2", "2025-11-15 MBTA/SillyGoose Ridealong 2/2025-11-15 V1 MBTA ridealong board2.txt", "AerospaceNU 2025-11-15 MBTA, SillyGoose V1 ridealong 2")
real_case("crocket", "2025-11-15 Crocket/SillyGoose Backup Backup/2025-11-15 V1 Crocket backup.txt", "AerospaceNU 2025-11-15 Crocket, SillyGoose V1")
real_case("mcguire_cert", "2025-11-15 Nathan McGuire Cert/SillyGoose Ridealong/2025-11-15 V1 Nathan McGuire Cert.txt", "AerospaceNU 2025-11-15 cert flight, SillyGoose V1")
real_case("blueup", "2026-02-22 BlueUp/SillyGoose Backup/2026-02-22 V1 BlueUp backup.txt", "AerospaceNU 2026-02-22 BlueUp, SillyGoose V1")
real_case("ethical_missile", "2026-02-22 Ethical Missile/SillyGoose Backup/2026-02-22 V1 Ethical Missile backup.txt", "AerospaceNU 2026-02-22 Ethical Missile, SillyGoose V1")
real_case("rocketworks", "2026-02-22 RocketWorks/SillyGoose Backup/2026-02-22 V1 RocketWorks backup.txt", "AerospaceNU 2026-02-22 RocketWorks, SillyGoose V1")
real_case("gianni_l3", "2026-05-16 Gianni's L3/SillyGoose Ridealong/2026-05-16 V2 Gianni L3 ridealong.txt", "AerospaceNU 2026-05-16 L3, SillyGoose V2")

# Haybales: the board really rebooted mid-descent. Stitch the post-reboot log after the pre-reboot one; the
# gap is the time the descent rate needs to cover the pressure difference.
pre = rows_of(os.path.join(FLIGHT_DATA, "2026-02-22 Haybales/SillyGoose Primary PT1/2026-02-22 V1 Haybales primary pre reboot.txt"))
post = rows_of(os.path.join(FLIGHT_DATA, "2026-02-22 Haybales/SillyGoose Primary PT2/2026-02-22 V1 Haybales primary post reboot.txt"))
tail = [r for r in pre if r[0] >= pre[-1][0] - 2000]
pa_per_ms = (tail[-1][1] - tail[0][1]) / max(1.0, tail[-1][0] - tail[0][0])
gap_ms = max(500.0, (post[0][1] - pre[-1][1]) / pa_per_ms) if pa_per_ms > 0 else 1000.0
HAYBALES_REBOOT_INPUT_MS = int(pre[-1][0] + gap_ms / 2)
stitched = pre + [[r[0] + pre[-1][0] + gap_ms] + r[1:] for r in post]
print(f"haybales: pre ends {pre[-1][0]:.0f} ms, gap {gap_ms:.0f} ms")
real_case("haybales_real_reboot", None, "AerospaceNU 2026-02-22 Haybales, SillyGoose V1 - rebooted mid-descent, pre/post logs stitched", reboot=stitched)


# ---- MIT (RocketPy-Team/RocketPy data/rockets): see LICENSE-RocketPy.txt
altos_case("prometheus_telemega", "rocketpy_data/prometheus/2022-06-24-serial-6583-flight-0003-TeleMega.csv",
           "Western Engineering Rocketry Team Prometheus, SAC 2022, TeleMega (RocketPy data, MIT) - transonic, 1.7 kPa main-ejection spike")
altos_case("prometheus_telemetrum", "rocketpy_data/prometheus/2022-06-24-serial-5115-flight-0001-TeleMetrum.csv",
           "Western Engineering Rocketry Team Prometheus, SAC 2022, TeleMetrum (RocketPy data, MIT)")


def padded(flight):
    pad = flight[0][1:]
    return [[t] + pad[:5] + [0, 0, 0] for t in range(0, 59000, 5000)] + flight


def real_rows_case(name, rows, note, reference):
    case = {"name": name, "input": write_log(name, rows), "source": note, "expect": {}}
    found, _, summary = run(case)
    case["expect"] = windows_from(found, REAL_TOLERANCE)
    print(f"real {name}: {summary}  | {reference}")
    cases.append(case)


import csv as _csv
hedy = os.path.join(EXTERNAL, "rocketpy_data/hedy/cats_tust")
baro = list(_csv.DictReader(open(os.path.join(hedy, "baro.csv"))))
imu = list(_csv.DictReader(open(os.path.join(hedy, "imu.csv"))))
info = list(_csv.DictReader(open(os.path.join(hedy, "flightInfo.csv"))))
deg = 3.141592653589793 / 180
flight = [[60000 + float(b["ts"]) * 1000, float(b["P"]), float(b["T"]) + 273.15, float(i["Ax"]), float(i["Ay"]), float(i["Az"]),
           float(i["Gx"]) * deg, float(i["Gy"]) * deg, float(i["Gz"]) * deg] for b, i in zip(baro, imu)]
cats_apogee = max(info, key=lambda r: float(r["height"]))
real_rows_case("hedy_cats", padded(flight), "TU Wien Space Team Hedy, CATS Vega, 100 Hz (RocketPy data, MIT) - high-subsonic baro jumps",
               f"CATS apogee {60 + float(cats_apogee['ts']):.2f} s at {float(cats_apogee['height']):.0f} m")

juno = list(_csv.DictReader(open(os.path.join(EXTERNAL, "rocketpy_data/juno3/cots_altimeter.csv"))))
flight = []
for r in juno:
    try:
        flight.append([60000 + float(r["time"]) * 1000, float(r["pressure"]) * 100, float(r["temperature"]) + 273.15, 0, 0, G, 0, 0, 0])
    except ValueError:
        pass
real_rows_case("juno3_rrc3_glitch", padded(flight), "Projeto Jupiter Juno III, SAC 2023, RRC3 20 Hz baro only (RocketPy data, MIT) - 19 kPa / 116 kPa glitch at drogue, log ends",
               "RRC3 log ends at drogue fire")

# Older boards and ground-station captures, converted to board columns by convert_rocket_flight_data.py (run it first)
RFD_SIM = os.path.join(REPO, "agent_work/logs/rocket_flight_data_sim")
for name, file, note in (
        ("fcb_beanboozler_2022_06", "2022_06_11_beanboozler__fcb_fcbv0", "FCB v0, 67 Hz resampled to 10 ms, 320 s flight"),
        ("fcb_beanboozler_2022_09", "2022_09_17_beanboozler__fcb_fcbv0", "FCB v0, accelerometer clipping refilled from high-g, ejection shock"),
        ("fcb_oorr_2023_04", "2023_04_15_oorr__fcb_fcbv0", "FCB v0, 135 s flight"),
        ("fcbv2_wyatt_2025_02", "2025_02_15_wyatt__fcb_fcbv2", "FCB v2, accelerometer scale fixed (x2/3)"),
        ("groundstation_starship_2023_05", "2023_05_27_starship_enterprise__fcbgroundstation_fcbv1", "ground-station telemetry, 5 Hz with gaps up to 23 s, ends before landing"),
        ("haybales_post_reboot_only", "2026_02_22_haybales__sillygoose_primary_pt2", "Haybales post-reboot log alone: fresh board starting 154 m up, mid-descent")):
    rows = rows_of(os.path.join(RFD_SIM, file + ".txt"))
    if any(rows[i][0] - rows[i - 1][0] <= 30 for i in range(1, len(rows))):
        first, last = dense_span(rows)
        rows = [r for r in rows if first - 40000 <= r[0] <= last + 120000]
    real_rows_case(name, rows, f"AerospaceNU rocket-flight-data {file}: {note}", "converted")

# ---- synthetic (agent_work/scripts/flight_logic_tests/stress_test.cpp)
t, r = emit(1012, "clean")
synthetic_case("syn_subsonic", t, r, "clean subsonic flight")
for name, label, delta in (("syn_reboot_near_apogee", "reboot 0.3 s after apogee", 0.3), ("syn_reboot_under_drogue", "reboot 20 s into the drogue descent", 20.0)):
    synthetic_case(name, t, r, label, reboot_input_s=t["apogee_s"] + delta, slack=6.0, land_slack=40)
synthetic_case("syn_reboot_during_boost", t, r, "reboot 1.5 s after liftoff, motor still burning", reboot_input_s=t["liftoff_s"] + 1.5, slack=6.0, land_slack=40)
synthetic_case("syn_dead_barometer", t, r, "every barometer read fails", zero_pressure(),
               expect={"launch": None, "apogee": None, "drogue": None, "main": None})
synthetic_case("syn_barometer_dies_on_pad", t, r, "barometer fails 20 s before liftoff and stays dead", zero_pressure(after_s=-20),
               expect={"launch": None, "apogee": None, "drogue": None, "main": None})
synthetic_case("syn_one_bad_read_per_second", t, r, "one failed barometer read per second in flight", zero_pressure(every_s=1))
garble_rng = random.Random(7)
CRLF, TAB = "\r\n", "\t"
with open_gz(os.path.join(OUT, "syn_garbled_log.txt.gz"), newline="") as f:
    f.write("MSG:" + TAB + "Logger setup" + CRLF + "CONFIG" + TAB + "BOARD_NAME=x" + CRLF + HEADER + CRLF)
    swapped = list(r)
    for i in range(0, len(swapped) - 1, 97):
        swapped[i], swapped[i + 1] = swapped[i + 1], swapped[i]
    for row in swapped:
        line = TAB.join(["%.0f" % row[0], "%.1f" % row[1], "%.2f" % row[2]] + ["%.4f" % v for v in row[3:6]] + ["%.5f" % v for v in row[6:9]] + ["extra", "1"])
        f.write(line + CRLF)
        roll = garble_rng.random()
        if roll < 0.01:
            f.write(line + CRLF)
        elif roll < 0.02:
            f.write(TAB.join(["12", "34", "short"]) + CRLF)
        elif roll < 0.03:
            f.write(TAB.join(["%.0f" % row[0], "nan"] + ["1"] * 7) + CRLF)
        elif roll < 0.04:
            f.write("WARN:" + TAB + "Loop overrun, lasting 11 ms" + CRLF)
        elif roll < 0.05:
            f.write(chr(0) + chr(255) + " garbage " + TAB + TAB + CRLF)
subsonic = next(c for c in cases if c["name"] == "syn_subsonic")
case = {"name": "syn_garbled_log", "input": "syn_garbled_log.txt.gz", "expect": dict(subsonic["expect"]),
        "source": "syn_subsonic with CRLF, junk/NaN/short lines, duplicated and swapped rows, extra columns"}
found, failures, summary = run(case)
print(f"syn  syn_garbled_log: {'FAIL ' + '; '.join(failures) if failures else 'ok'}  {summary}")
cases.append(case)
t, r = emit(1003, "clean", {"STRESS_GROUND_TEMP": "310", "STRESS_GROUND_ELEV": "1500"})
synthetic_case("syn_hot_high_site", t, r, "1500 m, 37 C launch site (old ground-temperature false launch)")
t, r = emit(1302, "clean")
synthetic_case("syn_hot_site_fresh_board", t, r, "1065 m, 39 C site, freshly flashed board (old code: drogue fired on the pad 6 s after boot)")
t, r = pick("mach", lambda x: x["max_mach"] > 1.15 and x["apogee_m"] > 3000)
synthetic_case("syn_transonic", t, r, "Mach 1.15+ with static-port dip/spike")
TRANSONIC = "transonic/supersonic climb (Mach lockout cleared by a slow-looking filter fired the drogue 21-33 s early)"
reboot_cases("syn_transonic", t, r, [54.43], TRANSONIC)
t, r = pick("spikes", lambda x: x["max_mach"] > 1.05)
synthetic_case("syn_ejection_spikes", t, r, "transonic plus ejection-charge pressure spikes, 10-25 g boost")
reboot_cases("syn_ejection_spikes", t, r, [45.05], TRANSONIC)
t, r = pick("faults", lambda x: x["apogee_m"] > 1000)
synthetic_case("syn_noisy_dropouts", t, r, "noisy barometer and accelerometer, scattered failed reads", land_slack=45)
t, r = pick("outage", lambda x: x["outage_at_s"] > x["apogee_s"] + 5 and x["outage_s"] > 8 and x["touchdown_s"] - x["outage_at_s"] > 90)
synthetic_case("syn_outage_under_drogue", t, r, "barometer outage of 8+ s under the drogue")
t, r = pick("highalt", lambda x: 25000 < x["apogee_m"] < 29000 and x["max_mach"] > 2.0)
synthetic_case("syn_90kft_mach2", t, r, "~27 km apogee, Mach 2+", slack=6.0, land_slack=40)
reboot_cases("syn_90kft_mach2", t, r, [97.06], TRANSONIC)
t, r = pick("highalt", lambda x: 29000 < x["apogee_m"] < 32000 and x["max_mach"] > 2.8)
synthetic_case("syn_32km_mach3", t, r, "~30 km apogee, Mach 3", slack=6.0, land_slack=40)
reboot_cases("syn_32km_mach3", t, r, [88.41, 93.66], TRANSONIC)
os.remove(os.path.join(OUT, "_emit.txt"))

json.dump(cases, open(os.path.join(REPO, "tests/sim/cases.json"), "w"), indent=2)
size = sum(os.path.getsize(os.path.join(OUT, f)) for f in os.listdir(OUT))
print(f"{len(cases)} cases, data {size / 1e6:.1f} MB")
