# End-to-end Monte Carlo through the desktop build (estimator, flight logic, DualDeploy, logger, offload):
# stress_test.exe emits each flight as a board log, the desktop build flies it, and check_sim's checker
# judges the offload against the flight's truth with the same windows the CI synthetic cases use.
# usage: python -I desktop_monte_carlo.py DESKTOP_EXE SCENARIO FLIGHTS [FIRST_SEED]
import json, os, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "../../.."))
STRESS = os.path.join(REPO, "agent_work/scripts/flight_logic_tests/stress_test.exe")
sys.path.insert(0, os.path.join(REPO, "tests/sim"))
import check_sim  # noqa: E402

desktop, scenario, flights = sys.argv[1], sys.argv[2], int(sys.argv[3])
first = int(sys.argv[4]) if len(sys.argv) > 4 else 1
failed = 0
with tempfile.TemporaryDirectory() as work:
    for seed in range(first, first + flights):
        log = os.path.join(work, "in.txt")
        truth = json.loads(subprocess.run([STRESS, "emit", str(seed), scenario, log], capture_output=True, text=True, check=True).stdout)
        if truth["touchdown_s"] < 0 or truth["apogee_m"] > 32000 or truth["apogee_m"] < 150:
            continue
        L, A, T = truth["liftoff_s"], truth["apogee_s"], truth["touchdown_s"]
        slack = 6.0 if scenario in ("highalt", "reboot") else 3.0
        expect = {"launch": [L, L + 1.0], "apogee": [A - 1.0, A + slack], "main": [A, T], "main_altitude": [140, 210]}
        case = {"name": f"s{seed}", "input": "in.txt", "expect": expect}
        if scenario == "reboot" and truth["reboot_at_s"] > 0:
            case["reboot_at_ms"] = [int(round(truth["reboot_at_s"] * 1000 - check_sim.sim_start_ms(check_sim.read_text(log)), -1))]
            if truth["reboot_at_s"] > A:
                del expect["apogee"]
        failures, summary = check_sim.check_case(desktop, case, work, work)
        if failures:
            failed += 1
            print(f"FAIL seed={seed} {'; '.join(failures)} | {summary} | truth L {L:.2f} A {A:.2f} T {T:.2f} reboot {truth['reboot_at_s']:.2f}", flush=True)
print(f"SUMMARY {scenario}: {flights} seeds, {failed} failed")
