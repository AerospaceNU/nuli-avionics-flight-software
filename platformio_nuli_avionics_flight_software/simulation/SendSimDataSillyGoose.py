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

OUTPUT_FILE = "output.txt"

# SillyGoose log files follow: "<YYYY-MM-DD> V<n> <rocket> <descriptors>.txt"
SILLYGOOSE_RE = re.compile(r"^\d{4}-\d{2}-\d{2} V\d.*\.txt$")

# Column indices in the SillyGoose tab-separated log format
COL_TIMESTAMP_MS = 0
COL_PRESSURE_PA = 1
COL_TEMP_K = 2
COL_ACCEL_X = 3
COL_ACCEL_Y = 4
COL_ACCEL_Z = 5

# Event set by serial_echo when the firmware sends "--send" between rows
continue_event = threading.Event()


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


def serial_echo(ser):
    """Continuously read from serial, echo to stdout, log to file, set event on '--send'."""
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

                    if line_str == "--send":
                        continue_event.set()
                    else:
                        sys.stdout.buffer.write(line + b'\n')
                        sys.stdout.flush()
            except serial.SerialException:
                break


def load_sillygoose(path):
    """Parse a SillyGoose log file into an (N, 6) array: [timestamp_ms, pressure_pa, temp_k, ax, ay, az].

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
            if len(parts) < 6:
                continue
            try:
                rows.append((
                    float(parts[COL_TIMESTAMP_MS]),
                    float(parts[COL_PRESSURE_PA]),
                    float(parts[COL_TEMP_K]),
                    float(parts[COL_ACCEL_X]),
                    float(parts[COL_ACCEL_Y]),
                    float(parts[COL_ACCEL_Z]),
                ))
            except ValueError:
                continue

    if not rows:
        return np.empty((0, 6), dtype=float)
    arr = np.array(rows, dtype=float)
    arr = arr[arr[:, 0].argsort()]
    return arr


def main():
    path = list_sillygoose_files()
    port_name = list_serial_ports()
    ser = serial.Serial(port_name, 115200, timeout=0.1)

    echo_thread = threading.Thread(target=serial_echo, args=(ser,), daemon=True)
    echo_thread.start()

    arr = load_sillygoose(path)
    if len(arr) == 0:
        print(f"No data rows found in {path}")
        ser.close()
        sys.exit(1)

    t_start = arr[0, 0]
    t_end = arr[-1, 0]
    print(
        f"\nStreaming {os.path.basename(path)} to {port_name} "
        f"({len(arr)} log rows, {(t_end - t_start) / 1000.0:.1f}s of flight). "
        f"Filling gaps > {GAP_THRESHOLD_MS:.0f}ms at {STREAM_HZ:.0f}Hz; "
        f"in-flight samples sent verbatim. Waiting for '--send' between rows...\n"
    )

    def send_floats(row5):
        line = "--sim " + ','.join(f"{v:.6f}" for v in row5) + "\n"
        ser.write(line.encode('utf-8'))

    # The firmware emits --send at the end of setup() (and after each consumed row), so we just wait
    # for the first one before sending row 0. The --streamLog -b kicks off the firmware log stream.
    ser.write(b"--streamLog -b\n")
    continue_event.wait()
    continue_event.clear()
    send_floats(arr[0, 1:])

    prev = arr[0]
    for i in range(1, len(arr)):
        cur = arr[i]
        dt_ms = cur[0] - prev[0]
        if dt_ms > GAP_THRESHOLD_MS:
            # Ground-state gap: fill at 100 Hz between prev and cur, exclusive of endpoints
            t = prev[0] + STREAM_DT_MS
            while t < cur[0]:
                ratio = (t - prev[0]) / dt_ms
                interp = prev + (cur - prev) * ratio
                continue_event.wait()
                continue_event.clear()
                send_floats(interp[1:])
                t += STREAM_DT_MS
        continue_event.wait()
        continue_event.clear()
        send_floats(cur[1:])
        prev = cur

    print("\nStreaming complete.")
    ser.close()


if __name__ == "__main__":
    main()