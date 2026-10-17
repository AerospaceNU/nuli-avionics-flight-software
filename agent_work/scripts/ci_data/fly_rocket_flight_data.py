# Flies every log convert_rocket_flight_data.py wrote through the desktop build and checks it against the
# manifest's own estimates: no launch before the real one (all pad time kept), apogee near the estimate, a
# drogue, and a main near 200 m when the flight went above 250 m. One line per flight, failures listed.
# usage: python -I fly_rocket_flight_data.py DESKTOP_EXE [SIM_DIR] [OUT_JSON]
import json, os, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "../../.."))
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(REPO, "tests/sim"))
import check_sim  # noqa: E402

desktop = sys.argv[1]
sim_dir = sys.argv[2] if len(sys.argv) > 2 and sys.argv[2] else os.path.join(REPO, "agent_work/logs/rocket_flight_data_sim")
out_json = sys.argv[3] if len(sys.argv) > 3 else None
manifest = json.load(open(os.path.join(sim_dir, "manifest.json")))
work = tempfile.mkdtemp()
results, total_fail = [], 0
for entry in manifest["outputs"]:
    path = os.path.join(sim_dir, entry["file"])
    out = os.path.join(work, "out.txt")
    subprocess.run([desktop, path, out], check=True, stdout=subprocess.DEVNULL)
    text = check_sim.read_text(path)
    rows = check_sim.flight_rows(check_sim.read_text(out), [])
    found = check_sim.events(rows, check_sim.sim_start_ms(text))
    fails = check_sim.invariant_failures(rows, False)
    L = entry.get("launch_ms") and entry["launch_ms"] / 1000
    A = entry.get("apogee_ms") and entry["apogee_ms"] / 1000
    landed_in_log = entry.get("landing_ms") is not None
    apogee_m = entry.get("apogee_m_agl_est") or 0
    if "launch" in found and L is not None and found["launch"] < L - 0.5:
        fails.append(f"EARLY LAUNCH {found['launch'] - L:+.2f} s")
    if L is None and "launch" in found:
        fails.append(f"launch at {found['launch']:.2f} with no flight in the log")
    if L is not None and apogee_m > 100 and "launch" not in found:
        fails.append("no launch")
    if A is not None and apogee_m > 100 and entry.get("rate_hz", {}).get("flight"):
        if "apogee" not in found:
            fails.append("no apogee")
        elif not A - 1.0 <= found["apogee"] <= A + 4.0:
            fails.append(f"apogee {found['apogee'] - A:+.2f} s from estimate")
        if "drogue" not in found and "apogee" in found:
            fails.append("no drogue")
    if apogee_m > 250 and landed_in_log and "apogee" in found:
        if "main" not in found:
            fails.append("no main")
        elif not 140 <= found["main_altitude"] <= 230:
            fails.append(f"main at {found['main_altitude']:.0f} m")
    rel = {k: round(v - L, 2) if L is not None and k != "main_altitude" else round(v, 2) for k, v in found.items()}
    results.append({"name": entry["name"], "events_from_launch_s": rel, "failures": fails, "flags": entry.get("flags", [])})
    total_fail += bool(fails)
    print(f"{'FAIL' if fails else 'ok  '} {entry['name']}: {'  '.join(f'{k} {v}' for k, v in sorted(rel.items()))}", flush=True)
    for f in fails:
        print(f"       {f}")
if out_json:
    json.dump(results, open(out_json, "w"), indent=1)
print(f"{len(results)} flights, {total_fail} with failures")
