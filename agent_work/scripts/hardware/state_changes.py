# Lists each flight in a text offload (one per "Logger setup"): its CONFIG line's FLIGHT_STATE /
# GROUND_ELEVATION, then every flight-state change and pyro fire with altitude, velocity and the
# unfiltered altitude. For checking reboot-mid-sim runs, where one sim spans two logged flights.
# usage: state_changes.py OFFLOAD_TEXT_CAPTURE [LAST_N_FLIGHTS]
import re, sys

lines = open(sys.argv[1], "rb").read().decode("latin1").replace("\r", "").split("\n")
last = int(sys.argv[2]) if len(sys.argv) > 2 else 3
flights, names = [], None
for l in lines:
    p = l.split("\t")
    if p[0] == "timestampMs":
        names = p
    elif l.startswith("CONFIG"):
        m = dict(kv.split("=", 1) for kv in p[1:] if "=" in kv)
        flights.append([f"CONFIG FLIGHT_STATE={m.get('FLIGHT_STATE')} GROUND_ELEVATION={m.get('GROUND_ELEVATION')}"])
        prev = None
        fired = set()
    elif names and flights and l[:1].isdigit() and len(p) == len(names):
        d = dict(zip(names, p))
        if d["flightState"] != prev:
            flights[-1].append(f"  t={d['timestampMs']:>7} state={d['flightState']} alt={float(d['altitudeM']):8.1f} vel={float(d['velocityMS']):7.1f} raw={float(d['unfilteredAltitudeM']):8.1f}")
            prev = d["flightState"]
        for k in names:
            if k.endswith("Fired") and d[k] not in ("0", "") and k not in fired:
                fired.add(k)
                flights[-1].append(f"  t={d['timestampMs']:>7} {k} alt={float(d['altitudeM']):8.1f} vel={float(d['velocityMS']):7.1f}")
for f in flights[-last:]:
    print("\n".join(f))
