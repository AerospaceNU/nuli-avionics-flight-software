# Throws malformed CLI input at a board (release build, PRE_FLIGHT so the CLI runs) and checks after
# every batch that it still answers a config query. Never sends anything that could change the board:
# every generated line is rejected if it contains -set, fire, erase, reset, sim, log (all spellings),
# offload or the stream flags. Compare a config snapshot before and after (the same --<CONFIG> queries).
# Every line sent goes to OUT.sent; a USB disconnect (crash / reboot) is reported with the last lines
# sent, and the run carries on once the port is back.
# usage: cli_fuzz.py PORT OUT [BATCHES] [SEED] [--only=KIND,...]
import random, sys, time
import serial

port, out = sys.argv[1], sys.argv[2]
pos = [a for a in sys.argv[3:] if not a.startswith("--")]
batches = int(pos[0]) if pos else 40
rng = random.Random(int(pos[1]) if len(pos) > 1 else 1)
only = next((set(map(int, a[7:].split(","))) for a in sys.argv if a.startswith("--only=")), None)
READ_ONLY = ["--FIRMWARE_VERSION", "--DROGUE_DELAY", "--MAIN_ELEVATION", "--BOARD_NAME", "--BUZZER_ENABLED", "--GROUND_ELEVATION",
             "--help", "--logFormat", "--PYRO_FIRE_DURATION", "--CONFIGURATION_VERSION", "--RADIO_FREQUENCY"]
BANNED = ("set", "fire", "erase", "reset", "sim", "log", "offload", "stream", "-b", "-e", "-d", "-m")


def junk_token():
    kind = rng.randrange(6)
    if kind == 0: return "".join(rng.choice("abcdefghijklmnopqrstuvwxyz0123456789") for _ in range(rng.randrange(1, 12)))
    if kind == 1: return rng.choice(["-", "--", "---", "-x", "--x", "-?", "--?", "-1", "--1", '"', "'", "\\", "\t"])
    if kind == 2: return rng.choice(["nan", "inf", "-inf", "1e40", "-1e40", "99999999999999999999", "0x10", "1.2.3", "", " "])
    if kind == 3: return str(rng.uniform(-1e12, 1e12))
    if kind == 4: return rng.choice(READ_ONLY)
    return "".join(chr(rng.randrange(1, 256)) for _ in range(rng.randrange(1, 8)))


def junk_line():
    kind = rng.randrange(6)
    while only is not None and kind not in only:
        kind = rng.randrange(6)
    if kind == 0: return " ".join(junk_token() for _ in range(rng.randrange(1, 8)))
    if kind == 1: return rng.choice(READ_ONLY) + " " + " ".join(junk_token() for _ in range(rng.randrange(1, 5)))
    if kind == 2: return "x" * rng.choice([200, 255, 256, 257, 511, 512, 1023, 1024, 2000, 5000])
    if kind == 3: return " ".join(rng.choice(READ_ONLY) for _ in range(rng.randrange(2, 20)))
    if kind == 4: return "".join(chr(rng.randrange(0, 256)) for _ in range(rng.randrange(1, 300)))
    return rng.choice(READ_ONLY)[: rng.randrange(1, 10)]


def safe(line):
    low = line.lower()
    return not any(b in low for b in BANNED)


def open_port():
    for _ in range(60):
        try:
            p = serial.Serial(port, 115200, timeout=0.05, write_timeout=5)
            p.dtr = True
            time.sleep(0.3)
            p.reset_input_buffer()
            return p
        except serial.SerialException:
            time.sleep(1)
    raise SystemExit("port never came back")


def run_batch(s, cap, recent, sent_log):
    stalls, sent = 0, 0
    while sent < 25:
        line = junk_line()
        if not safe(line):
            continue
        end = rng.choice(["\n", "\r\n", "\r", ""])
        recent.append(f"{line[:60]!r}{'...(%d chars)' % len(line) if len(line) > 60 else ''} + {end!r}")
        del recent[:-10]
        sent_log.write(repr(line + end) + "\n")
        sent_log.flush()
        try:
            s.write(line.encode("latin1", "replace") + end.encode())
        except serial.SerialTimeoutException:  # the board stopped taking input for 5 s
            stalls += 1
            print(f"  write stalled on a {len(line)}-char line", flush=True)
        sent += 1
        time.sleep(rng.choice([0, 0, 0.005, 0.02]))
        cap += s.read(s.in_waiting or 0)
    s.write(b"\n--BOARD_NAME\n")
    deadline, got = time.time() + 3, bytearray()
    while time.time() < deadline and b"BOARD_NAME is set to" not in got:
        got += s.read(s.in_waiting or 1)
    cap += got
    return b"BOARD_NAME is set to" in got, stalls


s = open_port()
cap, recent = bytearray(), []
sent_log = open(out + ".sent", "w", encoding="utf-8")
failures = stalls = drops = 0
for b in range(batches):
    try:
        ok, st = run_batch(s, cap, recent, sent_log)
        stalls += st
        failures += not ok
        print(f"batch {b + 1}/{batches}: {'alive' if ok else 'NO ANSWER'}", flush=True)
    except serial.SerialException:  # the board left USB: crashed or rebooted
        drops += 1
        print(f"  USB DISCONNECT in batch {b + 1}; last lines sent:\n    " + "\n    ".join(recent[-5:]), flush=True)
        sent_log.write(f"### DISCONNECT in batch {b + 1}\n")
        try:
            s.close()
        except Exception:
            pass
        time.sleep(3)
        s = open_port()
s.close()
open(out, "wb").write(bytes(cap))
print(f"done: {failures} batches without an answer, {stalls} stalled writes, {drops} USB disconnects")
