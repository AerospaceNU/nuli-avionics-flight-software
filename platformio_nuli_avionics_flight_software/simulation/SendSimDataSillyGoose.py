import os
import re
import sys
import time
import threading
import serial
from serial.tools import list_ports
import numpy as np

# Sibling checkout: <Software>/nuli-avionics-flight-software/.../simulation/ -> <Software>/rocket-flight-data/data/
DATA_DIR = os.path.abspath(
    os.path.join(os.path.dirname(__file__), "..", "..", "..", "rocket-flight-data", "data")
)

STREAM_HZ = 100.0
STREAM_DT_MS = 1000.0 / STREAM_HZ
# In-flight samples are at the firmware loop rate (~10 ms). Pre/post-flight samples are 5 s apart
# because logger.setLogDelay(5000) is used on the ground. Anything bigger than this threshold is
# treated as a ground-state gap and filled with 100 Hz interpolation.
GAP_THRESHOLD_MS = 30.0

# Trim the long pre-launch idle period — keep only this many seconds before the
# transition from sparse ground logging to dense in-flight logging.
PREFLIGHT_KEEP_S = 35.0

OUTPUT_FILE = "output.txt"

# Whether to enable log streaming on the firmware (prints every logged row to
# serial each tick). Useful for debugging, but adds ~2 ms per tick and can push
# the firmware over its 10 ms budget.
STREAM_LOG = False

# High-water mark on the firmware queue. Once the report says we're at this
# level we stop sending until the firmware reports it dropped. Out of 100 slots
# in the firmware's circular buffer.
HIGH_WATER = 90
# Max rows sent per --simSize report. Drain budget in SimulationParser must be
# >= BURST so the next report reflects every line we sent. With BURST=2 and the
# firmware consuming 1/tick, the buffer grows by ~1/tick until HIGH_WATER, then
# settles into 1-per-tick top-ups.
BURST = 2

# SillyGoose log files follow: "<YYYY-MM-DD> V<n> <rocket> <descriptors>.txt"
SILLYGOOSE_RE = re.compile(r"^\d{4}-\d{2}-\d{2} V\d.*\.txt$")

# Column indices in the SillyGoose tab-separated log format
COL_TIMESTAMP_MS = 0
COL_PRESSURE_PA = 1
COL_TEMP_K = 2
COL_ACCEL_X = 3
COL_ACCEL_Y = 4
COL_ACCEL_Z = 5
COL_GYRO_X = 6
COL_GYRO_Y = 7
COL_GYRO_Z = 8

# Floats per sample sent to firmware (matches SimulationParser<8> in SillyGoose.cpp)
SAMPLE_FLOATS = 8

SIMSIZE_RE = re.compile(r"^--simSize\s+(\d+)\s*$")


class FlowControl:
    """Shared state between the serial echo thread and the main send loop.

    The firmware emits --simSize <N> every tick. We track that here so the
    sender can keep the firmware's queue topped up to TARGET_QUEUE.
    """
    def __init__(self):
        self.lock = threading.Lock()
        self.fw_size = 0
        self.event = threading.Event()
        self.ready = False  # Set once we've seen at least one size report

    def update(self, size):
        with self.lock:
            self.fw_size = size
            self.ready = True
        self.event.set()

    def snapshot(self):
        with self.lock:
            return self.fw_size, self.ready


def list_sillygoose_files():
    if not os.path.isdir(DATA_DIR):
        print(f"Could not find rocket-flight-data at {DATA_DIR}")
        print("Expected the repo to be checked out next to nuli-avionics-flight-software/")
        sys.exit(1)

    files = []
    for flight_dir in sorted(os.listdir(DATA_DIR)):
        d = os.path.join(DATA_DIR, flight_dir)
        if not os.path.isdir(d):
            continue
        for f in sorted(os.listdir(d)):
            if SILLYGOOSE_RE.match(f):
                files.append(os.path.join(d, f))

    if not files:
        print(f"No SillyGoose files found under {DATA_DIR}")
        sys.exit(1)

    print(f"\nSillyGoose files in {DATA_DIR}:")
    for i, p in enumerate(files):
        print(f"[{i}] {os.path.relpath(p, DATA_DIR)}")
    choice = int(input("Select file: "))
    return files[choice]


def list_serial_ports():
    ports = list(list_ports.comports())
    if not ports:
        print("No serial ports found")
        sys.exit(1)

    print("\nAvailable Serial Ports:")
    for i, p in enumerate(ports):
        print(f"[{i}] {p.device} ({p.description})")
    choice = int(input("Select serial port: "))
    return ports[choice].device


def serial_echo(ser, fc):
    """Continuously read from serial, echo to stdout, log to file, parse --simSize."""
    buffer = b""
    with open(OUTPUT_FILE, "w", buffering=1) as logfile:
        while True:
            try:
                data = ser.read(ser.in_waiting or 1)
                if not data:
                    continue

                buffer += data
                while b'\n' in buffer or b'\r' in buffer:
                    buffer = buffer.replace(b'\r', b'\n')
                    line, _, remainder = buffer.partition(b'\n')
                    buffer = remainder
                    line_str = line.decode('utf-8', errors='ignore').strip()

                    logfile.write(line_str + "\n")

                    m = SIMSIZE_RE.match(line_str)
                    if m:
                        fc.update(int(m.group(1)))
                    else:
                        sys.stdout.buffer.write(line + b'\n')
                        sys.stdout.flush()
            except serial.SerialException:
                break


def load_sillygoose(path):
    """Parse a SillyGoose log file into an (N, 9) array:
    [timestamp_ms, pressure_pa, temp_k, ax, ay, az, gx, gy, gz].

    Skips any non-data preamble (CONFIG row, column header, "Logger setup", etc.) by keeping only
    lines whose first character is a digit. Tab-separated.
    """
    rows = []
    with open(path, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            line = line.strip()
            if not line or not line[0].isdigit():
                continue
            parts = line.split('\t')
            if len(parts) < 9:
                continue
            try:
                rows.append((
                    float(parts[COL_TIMESTAMP_MS]),
                    float(parts[COL_PRESSURE_PA]),
                    float(parts[COL_TEMP_K]),
                    float(parts[COL_ACCEL_X]),
                    float(parts[COL_ACCEL_Y]),
                    float(parts[COL_ACCEL_Z]),
                    float(parts[COL_GYRO_X]),
                    float(parts[COL_GYRO_Y]),
                    float(parts[COL_GYRO_Z]),
                ))
            except ValueError:
                continue

    if not rows:
        return np.empty((0, 9), dtype=float)
    arr = np.array(rows, dtype=float)
    arr = arr[arr[:, 0].argsort()]
    return arr


def find_launch_index(arr):
    """First sample whose dt from its predecessor drops below GAP_THRESHOLD_MS.
    Ground logging is sparse (~5s); flight logging is dense (~10ms). Returns
    None if no such transition exists.
    """
    if len(arr) < 2:
        return None
    dts = np.diff(arr[:, 0])
    dense_mask = dts <= GAP_THRESHOLD_MS
    if not dense_mask.any():
        return None
    return int(np.argmax(dense_mask)) + 1


def trim_preflight(arr, keep_seconds):
    """Drop everything more than `keep_seconds` before launch."""
    launch_idx = find_launch_index(arr)
    if launch_idx is None:
        return arr
    cutoff_ms = arr[launch_idx, 0] - keep_seconds * 1000.0
    return arr[arr[:, 0] >= cutoff_ms]


def trim_from_offset(arr, offset_seconds):
    """Keep samples at or after `offset_seconds` from the start of the file."""
    if len(arr) == 0:
        return arr
    cutoff_ms = arr[0, 0] + offset_seconds * 1000.0
    return arr[arr[:, 0] >= cutoff_ms]


def prompt_start_offset(arr):
    """Ask the user where to start streaming. Returns seconds from file start,
    or None for auto-trim (last PREFLIGHT_KEEP_S before launch).
    """
    if len(arr) == 0:
        return None
    t0 = arr[0, 0]
    duration_s = (arr[-1, 0] - t0) / 1000.0
    launch_idx = find_launch_index(arr)
    if launch_idx is not None:
        launch_s = (arr[launch_idx, 0] - t0) / 1000.0
        print(f"\nFile: {duration_s:.0f}s total, launch ~{launch_s:.0f}s in")
    else:
        print(f"\nFile: {duration_s:.0f}s total, no launch transition detected")
    raw = input(
        f"Start offset in seconds (blank = auto {PREFLIGHT_KEEP_S:.0f}s before launch): "
    ).strip()
    if not raw:
        return None
    try:
        return float(raw)
    except ValueError:
        print(f"  Couldn't parse '{raw}'; using auto trim.")
        return None


def build_sample_stream(arr):
    """Expand the (timestamp, ...) log rows into a flat list of 8-float samples,
    interpolating ground-state gaps at STREAM_HZ to match the firmware loop rate.
    """
    samples = []
    if len(arr) == 0:
        return samples

    samples.append(arr[0, 1:])
    prev = arr[0]
    for i in range(1, len(arr)):
        cur = arr[i]
        dt_ms = cur[0] - prev[0]
        if dt_ms > GAP_THRESHOLD_MS:
            t = prev[0] + STREAM_DT_MS
            while t < cur[0]:
                ratio = (t - prev[0]) / dt_ms
                interp = prev + (cur - prev) * ratio
                samples.append(interp[1:])
                t += STREAM_DT_MS
        samples.append(cur[1:])
        prev = cur
    return samples


def send_sample(ser, sample):
    assert len(sample) == SAMPLE_FLOATS, f"expected {SAMPLE_FLOATS} floats, got {len(sample)}"
    line = "--sim " + ",".join(f"{v:.6f}" for v in sample) + "\n"
    ser.write(line.encode("utf-8"))


def main():
    path = list_sillygoose_files()
    port_name = list_serial_ports()

    arr = load_sillygoose(path)
    if len(arr) == 0:
        print(f"No data rows found in {path}")
        sys.exit(1)

    offset = prompt_start_offset(arr)
    if offset is None:
        arr = trim_preflight(arr, PREFLIGHT_KEEP_S)
    else:
        arr = trim_from_offset(arr, offset)
    samples = build_sample_stream(arr)

    # Open the serial port only now — once the user is done with prompts —
    # so firmware output doesn't dump into the console mid-selection.
    ser = serial.Serial(port_name, 115200, timeout=0.1)
    fc = FlowControl()
    echo_thread = threading.Thread(target=serial_echo, args=(ser, fc), daemon=True)
    echo_thread.start()

    t_start = arr[0, 0]
    t_end = arr[-1, 0]
    print(
        f"\nStreaming {os.path.basename(path)} to {port_name} "
        f"({len(arr)} log rows -> {len(samples)} samples, "
        f"{(t_end - t_start) / 1000.0:.1f}s of flight). "
        f"Burst {BURST}/report, high-water {HIGH_WATER}.\n"
    )

    if STREAM_LOG:
        ser.write(b"--streamLog -b\n")

    # Blind initial fill so the firmware has something to drain on its first
    # tick (waitForEntry blocks until at least one --sim arrives). One BURST
    # is enough — the report it triggers will drive everything after.
    idx = 0
    initial = min(BURST, len(samples))
    for _ in range(initial):
        send_sample(ser, samples[idx])
        idx += 1

    # Steady state: each --simSize N report, send min(BURST, HIGH_WATER - N).
    # While the buffer is below the high-water mark, that's a full burst every
    # tick → buffer grows by (BURST - 1)/tick. Once it hits HIGH_WATER we only
    # send when the firmware reports a drop. Because the firmware can't grow
    # its buffer without our sends and BURST is bounded, overflow is impossible.
    while idx < len(samples):
        if not fc.event.wait(timeout=1.0):
            continue
        fc.event.clear()
        fw_size, ready = fc.snapshot()
        if not ready:
            continue
        to_send = min(BURST, max(0, HIGH_WATER - fw_size), len(samples) - idx)
        for _ in range(to_send):
            send_sample(ser, samples[idx])
            idx += 1

    print("\nStreaming complete.")
    ser.close()


if __name__ == "__main__":
    main()