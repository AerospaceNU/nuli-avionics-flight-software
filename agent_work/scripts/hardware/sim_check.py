# Checks a flight sim by its offload: the simulated flight's state changes, apogee and pyro events,
# next to the original log's, all relative to launch. Also counts the loop overruns the sim run
# reported.
# usage: sim_check.py ORIGINAL_LOG OFFLOAD_TEXT_CAPTURE [SIM_RUN_CAPTURE]
import re, sys

STATES = {"0": "PRE_FLIGHT", "1": "ASCENT", "2": "DESCENT", "3": "POST_FLIGHT", "4": "UNKNOWN"}


def load(path):
    text = open(path, "rb").read().decode("latin1").replace("\r", "")
    return text.split("\n")


def columns(lines):
    # Column names: this repo's firmware names line, or an old tool export's names
    for l in lines:
        names = l.split("\t")
        if len(names) > 1 and all(re.fullmatch(r"[A-Za-z_]\w*", n) for n in names):
            return {n: i for i, n in enumerate(names)}
    raise SystemExit(f"no column-name line found")


def pick(cols, *names):
    for n in names:
        if n in cols:
            return cols[n]
    raise SystemExit(f"none of {names} in columns")


def flights(lines):
    # Splits rows at each "Logger setup" (one boot = one flight in an offload)
    out, cur = [], []
    for l in lines:
        if "Logger setup" in l:
            if cur:
                out.append(cur)
            cur = []
        elif re.match(r"^\d", l):
            cur.append(l.split("\t"))
    if cur:
        out.append(cur)
    return out


def summarize(rows, cols):
    t, st, alt = pick(cols, "timestampMs"), pick(cols, "flightState"), pick(cols, "altitudeM")
    fired = [(n[:-5], i) for n, i in cols.items() if n.endswith("Fired")]
    launch = next((float(r[t]) for i, r in enumerate(rows) if i and r[st] == "1" and rows[i - 1][st] != "1"), None)
    if launch is None:
        return None
    events = []
    for i in range(1, len(rows)):
        if rows[i][st] != rows[i - 1][st]:
            events.append(((float(rows[i][t]) - launch) / 1000, f"{STATES.get(rows[i - 1][st], '?')} -> {STATES.get(rows[i][st], '?')}"))
        for name, c in fired:
            if rows[i][c] == "1" and rows[i - 1][c] == "0":
                events.append(((float(rows[i][t]) - launch) / 1000, f"{name} fired"))
    apogee = max(rows, key=lambda r: float(r[alt]))
    return {"events": sorted(events), "apogee_m": float(apogee[alt]), "apogee_t": (float(apogee[t]) - launch) / 1000}


orig_lines = load(sys.argv[1])
orig_cols = columns(orig_lines)
orig = summarize([l.split("\t") for l in orig_lines if re.match(r"^\d", l)], orig_cols)

off_lines = load(sys.argv[2])
off_cols = columns(off_lines)
sims = [s for s in (summarize(f, off_cols) for f in flights(off_lines)) if s]
if not sims:
    raise SystemExit("no flight (PRE_FLIGHT -> ASCENT) found in the offload")
sim = sims[-1]

print("(each side relative to its own launch detection - a firmware that detects launch earlier shifts every later event; tests/sim/check_sim.py compares on the log clock)")
print(f"{'event':<28}{'original (s)':>14}{'sim offload (s)':>18}")
print(f"{'apogee':<28}{orig['apogee_t']:>14.2f}{sim['apogee_t']:>18.2f}   altitude {orig['apogee_m']:.0f} m vs {sim['apogee_m']:.0f} m")
names = []
for _, n in orig["events"] + sim["events"]:
    if n not in names:
        names.append(n)
for n in names:
    a = next((f"{t:.2f}" for t, m in orig["events"] if m == n), "-")
    b = next((f"{t:.2f}" for t, m in sim["events"] if m == n), "-")
    print(f"{n:<28}{a:>14}{b:>18}")
if len(sys.argv) > 3:
    run = load(sys.argv[3])
    over = [l for l in run if "Loop overrun" in l]
    print(f"loop overruns during the sim run: {len(over)}" + (f" (longest {max(int(re.search(r'(\d+) ms', l).group(1)) for l in over)} ms)" if over else ""))
