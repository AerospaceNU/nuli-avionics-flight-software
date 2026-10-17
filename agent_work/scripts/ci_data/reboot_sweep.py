# Reboot sweep: flies every CI case once without a reboot (its own baseline), then again with a reboot
# at every step through the flight (fine around launch, apogee, main and through the climb), and checks
# each run: no pyro in PRE_FLIGHT, a drogue no earlier than 1 s before and no later than 7 s after the
# baseline apogee, a main no later than 2 s after the baseline main. Writes every failing reboot time.
# --fine: ~1500 reboot times from 2 s before launch to 5 s after apogee (every 10 ms tick on short climbs),
# every tick around the main, 0.5 s for the rest of the flight, 1 s on the pad.
# usage: python -I reboot_sweep.py [--fine] DESKTOP_EXE CASES_JSON OUT_JSON [WORKERS] [CASE_NAME...]
import json, os, subprocess, sys, tempfile, threading
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "../../.."))
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(REPO, "tests/sim"))
import check_sim  # noqa: E402

FINE = "--fine" in sys.argv
FINE_CLIMB_TIMES = 1500
argv = [a for a in sys.argv if a != "--fine"]
desktop, cases_path, out_path = argv[1:4]
workers = int(argv[4]) if len(argv) > 4 else 6
only = set(argv[5:])
cases = [c for c in json.load(open(cases_path)) if not c.get("reboot_at_ms") and (not only or c["name"] in only)]
data_dir = os.path.join(os.path.dirname(os.path.abspath(cases_path)), "data")
work = tempfile.mkdtemp()
lock = threading.Lock()
RECOVERY_S = 3.0  # a reboot right before the main costs ~2.4 s: 1 s UNKNOWN, apogee again, 1 s drogue delay, main debounce


def fly(case, plain, start_ms, reboot_s, tag):
    out = os.path.join(work, f"{case['name']}_{tag}.out.txt")
    args = [desktop, plain, out]
    reboots = []
    if reboot_s is not None:
        reboots = [int(round(reboot_s * 1000 - start_ms))]
        if reboots[0] <= 0:
            return None
        args += ["--reboot-at", str(reboots[0])]
    subprocess.run(args, check=True, stdout=subprocess.DEVNULL)
    rows = check_sim.flight_rows(check_sim.read_text(out), reboots)
    os.remove(out)
    return rows


def first(rows, start_ms, field, value="1", after_s=None):
    for ms, row in rows:
        t = (ms + start_ms) / 1000.0
        if row[field] == value and (after_s is None or t >= after_s):
            return t, row
    return None, None


def judge(rows, start_ms, base, reboot_s):
    reasons = []
    for ms, row in rows:
        if row["flightState"] == "0" and (row["drogueFired"] == "1" or row["mainFired"] == "1"):
            reasons.append(f"pyro in PRE_FLIGHT at {(ms + start_ms) / 1000:.2f}")
            break
    if base.get("apogee") is not None:
        drogue, _ = first(rows, start_ms, "drogueFired")
        if drogue is None:
            reasons.append("no drogue")
        elif drogue < base["apogee"] - 1.0:
            reasons.append(f"drogue {drogue - base['apogee']:+.2f} s from apogee (early)")
        elif drogue > max(base["apogee"] + 7.0, reboot_s + RECOVERY_S) and reboot_s < base["apogee"] + 7.0:
            reasons.append(f"drogue {drogue - base['apogee']:+.2f} s from apogee (late)")
    if base.get("main") is not None and reboot_s < base["main"]:
        main, _ = first(rows, start_ms, "mainFired")
        if main is None:
            reasons.append("no main")
        elif main > max(base["main"] + 2.0, reboot_s + RECOVERY_S):
            reasons.append(f"main {main - base['main']:+.2f} s late")
    return reasons


def reboot_times(base, start_s, end_s):
    L, A, M = base.get("launch"), base.get("apogee"), base.get("main")
    times = set()
    def span(a, b, step):
        for i in range(int(round((b - a) / step)) + 1):
            times.add(round(a + i * step, 2))
    if FINE:
        span(start_s + 1, end_s, 1.0)
        if L is not None:
            climb_end = (A if A is not None else L + 20) + 5
            span(L - 2, climb_end, max(0.01, round((climb_end - L + 2) / FINE_CLIMB_TIMES, 2)))
            span(L - 2, end_s, 0.5)
        if M is not None:
            span(M - 6, M + 3, 0.01)
        return sorted(times)
    first_s = (L - 3) if L is not None else 5
    span(first_s, end_s, 1.0)
    if L is not None:
        span(L - 1, L + 3, 0.1)
        if A is not None:
            span(L, A, 0.25)
    if A is not None:
        span(A - 3, A + 3, 0.1)
    if M is not None:
        span(M - 5, M + 2, 0.1)
    return sorted(times)


failures = []
summary = []
for case in cases:
    text = check_sim.read_text(os.path.join(data_dir, case["input"]))
    plain = os.path.join(work, case["name"] + ".in.txt")
    open(plain, "w", encoding="utf-8").write(text)
    start_ms = check_sim.sim_start_ms(text)
    base_rows = fly(case, plain, start_ms, None, "base")
    base = check_sim.events(base_rows, start_ms)
    end_s = (base_rows[-1][0] + start_ms) / 1000.0
    times = reboot_times(base, (base_rows[0][0] + start_ms) / 1000.0, end_s)
    case_fail = []

    def one(r):
        rows = fly(case, plain, start_ms, r, f"r{int(round(r * 100))}")
        if rows is None:
            return
        reasons = judge(rows, start_ms, base, r)
        if reasons:
            with lock:
                case_fail.append({"case": case["name"], "reboot_s": r, "reasons": reasons})

    with ThreadPoolExecutor(workers) as pool:
        list(pool.map(one, times))
    case_fail.sort(key=lambda f: f["reboot_s"])
    failures += case_fail
    line = f"{case['name']}: {len(times)} reboot times, {len(case_fail)} failed"
    summary.append(line)
    print(line, flush=True)
    for f in case_fail[:12]:
        print(f"    reboot at {f['reboot_s']:.2f} s: {'; '.join(f['reasons'])}", flush=True)
    json.dump({"summary": summary, "failures": failures}, open(out_path, "w"), indent=1)
json.dump({"summary": summary, "failures": failures}, open(out_path, "w"), indent=1)
print(f"TOTAL {len(failures)} failing reboot times")
