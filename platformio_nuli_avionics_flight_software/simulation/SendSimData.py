import os
import sys
import time
import csv
import serial
import threading
from serial.tools import list_ports
import numpy as np
import pandas as pd

DATA_DIR = "data/"
STREAM_HZ = 100.0
STREAM_DT = 1.0 / STREAM_HZ

# Event flag set by serial_echo when "--continue" is received
continue_event = threading.Event()

# Output file for received serial data
OUTPUT_FILE = "output.txt"

def list_csv_files():
    csv_files = [f for f in os.listdir(DATA_DIR) if f.lower().endswith(".csv")]
    if not csv_files:
        print("No CSV files found in /data")
        sys.exit(1)

    print("\nAvailable CSV files:")
    for i, f in enumerate(csv_files):
        print(f"[{i}] {f}")
    choice = int(input("Select CSV file: "))
    return os.path.join(DATA_DIR, csv_files[choice])

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
    """ Continuously read from serial, echo to stdout, log everything to file, and set event on '--con' """
    buffer = b""
    with open(OUTPUT_FILE, "w", buffering=1) as logfile:  # line-buffered write
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

                    # Log every received line to file
                    logfile.write(line_str + "\n")

                    if line_str == "--con":
                        continue_event.set()
                    else:
                        sys.stdout.buffer.write(line + b'\n')
                        sys.stdout.flush()

            except serial.SerialException:
                break

def load_csv(file_path):
    df = pd.read_csv(file_path)
    df = df.applymap(lambda x: str(x).replace(',', '') if isinstance(x, str) else x)
    df = df.apply(pd.to_numeric, errors='coerce')
    df = df.dropna()
    df = df.sort_values('timestamp_ms').reset_index(drop=True)
    return df

def interpolate_row(df, t_ms):
    if t_ms <= df['timestamp_ms'].iloc[0]:
        return df.iloc[0]
    if t_ms >= df['timestamp_ms'].iloc[-1]:
        return df.iloc[-1]

    idx = np.searchsorted(df['timestamp_ms'].values, t_ms)
    t0 = df['timestamp_ms'].iloc[idx - 1]
    t1 = df['timestamp_ms'].iloc[idx]
    row0 = df.iloc[idx - 1]
    row1 = df.iloc[idx]
    ratio = (t_ms - t0) / (t1 - t0)
    return row0 + (row1 - row0) * ratio

def main():
    csv_path = list_csv_files()
    port_name = list_serial_ports()
    ser = serial.Serial(port_name, 115200, timeout=0.1)
    ser.write(b"--streamLog -start\n")
    time.sleep(0.1)
    ser.write(b"--sim\n")
    time.sleep(0.1)


    # Start serial echo thread
    echo_thread = threading.Thread(target=serial_echo, args=(ser,), daemon=True)
    echo_thread.start()

    df = load_csv(csv_path)
    stream_start_ms = df['timestamp_ms'].iloc[0]
    stream_end_ms = df['timestamp_ms'].iloc[-1]

    current_t_ms = stream_start_ms
    print(f"\nStreaming {csv_path} to {port_name} (waiting for '--con' between lines)...\n")

    while current_t_ms <= stream_end_ms:
        row = interpolate_row(df, current_t_ms)
        line = ','.join(f"{val:.6f}" for val in row.values[1:]) + "\n"
        ser.write(line.encode('utf-8'))

        # Wait for continue_event from serial_echo
        continue_event.wait()
        continue_event.clear()

        current_t_ms += STREAM_DT * 1000.0

    print("\nStreaming complete.")
    ser.close()

if __name__ == "__main__":
    main()
