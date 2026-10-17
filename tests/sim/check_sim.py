"""Runs the desktop build on every case in cases.json and checks the flight events in its offload.

Event times are on the input log's own clock (seconds), so a case's expected windows come straight
from the original flight or the synthetic flight's truth.
usage: check_sim.py DESKTOP_EXE CASES_JSON [CASE_NAME]
"""
import gzip
import json
import os
import subprocess
import sys
import tempfile

GAP_THRESHOLD_MS = 30.0
PREFLIGHT_KEEP_MS = 35000.0
PRE_FLIGHT, ASCENT, DESCENT, POST_FLIGHT, UNKNOWN = range(5)
DROGUE_AFTER_APOGEE_S = (0.9, 1.2)


def read_text(path):
    opener = gzip.open if path.endswith(".gz") else open
    with opener(path, "rt", encoding="utf-8", errors="replace") as f:
        return f.read()


def sim_start_ms(log_text):
    """The input timestamp the sim's first tick comes from - SimulationLogFile's trim, redone."""
    times = []
    for line in log_text.split("\n"):
        line = line.strip()
        if not line or not line[0].isdigit():
            continue
        parts = line.split("\t")
        try:
            values = [float(p) for p in parts[:9]]
        except ValueError:
            continue
        if len(values) == 9:
            times.append(values[0])
    times.sort()
    for i in range(1, len(times)):
        if times[i] - times[i - 1] <= GAP_THRESHOLD_MS:
            cutoff = times[i] - PREFLIGHT_KEEP_MS
            return next(t for t in times if t >= cutoff)
    return times[0]


def flight_rows(offload_text, reboots_ms):
    """(sim time ms, row dict) for every logged row, boots stitched together with the reboot times."""
    rows, header, boot = [], None, -1
    for line in offload_text.split("\n"):
        if line.startswith("timestampMs\t"):
            header = line.split("\t")
        elif "Logger setup" in line:
            boot += 1
        elif header and line[:1].isdigit():
            values = line.split("\t")
            if len(values) == len(header):
                row = dict(zip(header, values))
                offset = reboots_ms[boot - 1] if boot > 0 else 0
                rows.append((int(row["timestampMs"]) + offset, row))
    return rows


def events(rows, start_ms):
    found, previous_state = {}, None
    to_s = lambda ms: (ms + start_ms) / 1000.0
    for ms, row in rows:
        state = int(row["flightState"])
        if state != previous_state:
            name = {ASCENT: "launch", DESCENT: "apogee", POST_FLIGHT: "landed"}.get(state)
            if name and name not in found:
                found[name] = to_s(ms)
            previous_state = state
        if row["drogueFired"] == "1" and "drogue" not in found:
            found["drogue"] = to_s(ms)
        if row["mainFired"] == "1" and "main" not in found:
            found["main"] = to_s(ms)
            found["main_altitude"] = float(row["altitudeM"])
    return found


def invariant_failures(rows, rebooted):
    failures, previous_state, boot_marker = [], None, None
    for ms, row in rows:
        state = int(row["flightState"])
        if state == PRE_FLIGHT and (row["drogueFired"] == "1" or row["mainFired"] == "1"):
            failures.append(f"pyro on in PRE_FLIGHT at {ms} ms")
            break
        if previous_state is not None and state != previous_state and not rebooted:
            allowed = {PRE_FLIGHT: {ASCENT}, ASCENT: {DESCENT}, DESCENT: {POST_FLIGHT}, UNKNOWN: {PRE_FLIGHT, ASCENT}}
            if state not in allowed.get(previous_state, set()):
                failures.append(f"state {previous_state} -> {state} at {ms} ms")
        previous_state = state
    return failures


def check_case(desktop, case, data_dir, work_dir):
    input_path = os.path.join(data_dir, case["input"])
    log_text = read_text(input_path)
    plain_input = os.path.join(work_dir, case["name"] + ".in.txt")
    with open(plain_input, "w", encoding="utf-8") as f:
        f.write(log_text)
    output = os.path.join(work_dir, case["name"] + ".out.txt")
    reboots_ms = sorted(case.get("reboot_at_ms", []))
    args = [desktop, plain_input, output]
    for ms in reboots_ms:
        args += ["--reboot-at", str(ms)]
    subprocess.run(args, check=True, stdout=subprocess.DEVNULL)

    rows = flight_rows(read_text(output), reboots_ms)
    found = events(rows, sim_start_ms(log_text))
    failures = invariant_failures(rows, bool(reboots_ms))
    if "apogee" in found and "drogue" in found and not reboots_ms:
        delay = found["drogue"] - found["apogee"]
        if not DROGUE_AFTER_APOGEE_S[0] <= delay <= DROGUE_AFTER_APOGEE_S[1]:
            failures.append(f"drogue {delay:.2f} s after apogee")
    if "main" in found and ("drogue" not in found or found["main"] < found["drogue"]):
        failures.append("main before drogue")
    for name, window in case["expect"].items():
        value = found.get(name)
        if window is None:
            if value is not None:
                failures.append(f"{name} at {value:.2f}, expected none")
        elif value is None:
            failures.append(f"no {name}, expected {window[0]}..{window[1]}")
        elif not window[0] <= value <= window[1]:
            failures.append(f"{name} {value:.2f}, expected {window[0]}..{window[1]}")
    summary = "  ".join(f"{k} {v:.2f}" for k, v in sorted(found.items()))
    return failures, summary


def main():
    desktop, cases_path = sys.argv[1], sys.argv[2]
    only = sys.argv[3] if len(sys.argv) > 3 else None
    with open(cases_path, encoding="utf-8") as f:
        cases = json.load(f)
    data_dir = os.path.join(os.path.dirname(os.path.abspath(cases_path)), "data")
    failed = 0
    with tempfile.TemporaryDirectory() as work_dir:
        for case in cases:
            if only and case["name"] != only:
                continue
            failures, summary = check_case(desktop, case, data_dir, work_dir)
            failed += bool(failures)
            print(f"{'FAIL' if failures else 'ok  '} {case['name']}: {summary}")
            for failure in failures:
                print(f"       {failure}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
