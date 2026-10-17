# One line per real_replay.exe output: log apogee, firmware launch/apogee/drogue/main/landing.
# usage: replay_summary.py OUT...
import re, sys
for path in sys.argv[1:]:
    txt = open(path).read()
    m = re.search(r"log apogee (-?\d+) m at (-?[\d.]+) s", txt)
    ev = {}
    for name, key in (("ASCENT", "launch"), ("DESCENT", "apogee"), ("POST_FLIGHT", "landed")):
        e = re.search(rf"t=\s*(-?[\d.]+) {name}\s", txt)
        if e: ev[key] = float(e.group(1))
    d = re.search(r"t=\s*(-?[\d.]+) DROGUE\s+alt=\s*(-?[\d.]+) vel=\s*(-?[\d.]+)", txt)
    mn = re.search(r"t=\s*(-?[\d.]+) MAIN\s+alt=\s*(-?[\d.]+)", txt)
    rb = re.search(r"t=\s*(-?[\d.]+) REBOOT", txt)
    f = lambda k: f"{ev[k]:7.2f}" if k in ev else "      -"
    print(f"{path.split('/')[-1][:58]:58} logApo {m.group(2):>6}s {m.group(1):>5}m | launch {f('launch')} apogee {f('apogee')} "
          f"drogue {d.group(1) + '@' + d.group(3) + 'm/s' if d else '-':>16} main {mn.group(1) + '@' + mn.group(2) + 'm' if mn else '-':>16} "
          f"landed {f('landed')}" + (f" reboot@{rb.group(1)}" if rb else ""))
