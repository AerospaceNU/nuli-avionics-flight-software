import os
import sys
import time
from serial.tools import list_ports
import serial
from datetime import datetime

DATA_DIR = "data/"

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

def generate_flight_filename(flight_number):
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    return os.path.join(DATA_DIR, f"offload_{timestamp}_flight{flight_number}.txt")

def offload_data(ser):
    """Offload data from device, splitting into files for each 'New flight'."""
    os.makedirs(DATA_DIR, exist_ok=True)
    flight_number = 0
    current_file = None
    recording = False

    while True:
        try:
            line_bytes = ser.readline()
            if not line_bytes:
                continue

            line = line_bytes.decode('utf-8', errors='ignore').strip()
            print(line)

            # Start recording only after 'MSG:\tStarting Offload'
            if not recording:
                if line == "MSG:\tStarting Offload":
                    recording = True
                continue

            # Stop condition
            if line == "MSG:\tEnding Offload":
                if current_file:
                    current_file.close()
                break

            # Split into new file on 'Logger setup'
            if line == "Logger setup":
                if current_file:
                    current_file.close()
                flight_number += 1
                current_file = open(generate_flight_filename(flight_number), "w", buffering=1)

            # Write to current file if recording
            if current_file:
                current_file.write(line + "\n")

        except serial.SerialException:
            print("Serial connection lost.")
            break

def main():
    port_name = list_serial_ports()
    ser = serial.Serial(port_name, 115200, timeout=0.1)

    # Send offload command
    ser.write(b"--offload\n")
    time.sleep(0.1)

    offload_data(ser)
    print("\nOffload complete.")
    ser.close()

if __name__ == "__main__":
    main()
