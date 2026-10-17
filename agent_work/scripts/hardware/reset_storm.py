# Reset storm with torn config writes: sets a sentinel config value, then repeatedly changes another
# config value and resets the board a random 0-60 ms later (mid FRAM write). A torn write that broke
# the config CRC would reset everything to defaults, which the sentinel would show. Restores both at
# the end. Uses only the board's own CLI.
# usage: reset_storm.py PORT OUT [CYCLES] [SEED] [MAX_DELAY_S]
import random, sys, time
import serial

port, out = sys.argv[1], sys.argv[2]
cycles = int(sys.argv[3]) if len(sys.argv) > 3 else 40
rng = random.Random(int(sys.argv[4]) if len(sys.argv) > 4 else 5)
MAX_DELAY_S = float(sys.argv[5]) if len(sys.argv) > 5 else 0.06
SENTINEL, SENTINEL_VALUE, SENTINEL_DEFAULT = "--MAIN_ELEVATION", "201", "200"
cap = bytearray()


def open_port():
    for _ in range(60):
        try:
            s = serial.Serial(port, 115200, timeout=0.05, write_timeout=3)
            s.dtr = True
            time.sleep(0.3)
            s.reset_input_buffer()
            return s
        except serial.SerialException:
            time.sleep(0.5)
    raise SystemExit("port never came back")


def query(s, cmd, key):
    s.write((cmd + "\n").encode())
    t, buf = time.time(), b""
    while time.time() - t < 2 and key.encode() not in buf:
        buf += s.read(s.in_waiting or 1)
    t = time.time()
    while time.time() - t < 0.2:
        buf += s.read(s.in_waiting or 1)
    cap.extend(buf)
    line = next((l for l in buf.decode("latin1").split("\n") if key in l and "set to" in l), "")
    return line.split(":")[-1].strip()


s = open_port()
query(s, SENTINEL + " -set " + SENTINEL_VALUE, "MAIN_ELEVATION")
lost = 0
for i in range(cycles):
    value = str(rng.choice([0, 1]))
    try:
        s.write(("--BUZZER_ENABLED -set " + value + "\n").encode())
        time.sleep(rng.uniform(0, MAX_DELAY_S))
        s.write(b"--reset\n")
        time.sleep(0.2)
        s.close()
    except serial.SerialException:
        pass
    time.sleep(2.5)
    s = open_port()
    sentinel = query(s, SENTINEL, "MAIN_ELEVATION")
    buzzer = query(s, "--BUZZER_ENABLED", "BUZZER_ENABLED")
    reset_to_defaults = not sentinel.startswith(SENTINEL_VALUE)
    lost += reset_to_defaults
    print(f"cycle {i + 1}: MAIN_ELEVATION={sentinel} BUZZER_ENABLED={buzzer} (wrote {value}){'  CONFIG RESET' if reset_to_defaults else ''}", flush=True)
query(s, SENTINEL + " -set " + SENTINEL_DEFAULT, "MAIN_ELEVATION")
query(s, "--BUZZER_ENABLED -set 1", "BUZZER_ENABLED")
s.close()
open(out, "wb").write(bytes(cap))
print(f"done: {lost} of {cycles} cycles lost the config")
