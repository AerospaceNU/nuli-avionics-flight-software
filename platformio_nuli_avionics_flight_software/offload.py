import serial
import struct
import time
import serial.tools.list_ports
import csv

# Define the LogData struct format
LOG_DATA_FORMAT = "<Iddd"  # Little-endian: uint32, double, double, double
LOG_DATA_SIZE = struct.calcsize(LOG_DATA_FORMAT)
CSV_FILENAME = "log_data.csv"

def list_serial_ports():
    """List available serial ports."""
    ports = serial.tools.list_ports.comports()
    return [port.device for port in ports]

def select_serial_port():
    """Prompt user to select a serial port."""
    ports = list_serial_ports()
    if not ports:
        print("No serial ports found.")
        return None

    print("Available serial ports:")
    for i, port in enumerate(ports):
        print(f"{i}: {port}")

    choice = int(input("Select a port (enter number): "))
    return ports[choice] if 0 <= choice < len(ports) else None

def parse_log_data(data):
    """Unpack binary data into LogData struct fields."""
    timestamp, baroPressurePa, baroTemperatureK, baroAltitudeM = struct.unpack(LOG_DATA_FORMAT, data)
    return {
        "timestamp": timestamp,
        "baroPressurePa": baroPressurePa,
        "baroTemperatureK": baroTemperatureK,
        "baroAltitudeM": baroAltitudeM
    }

def main():
    port = select_serial_port()
    if not port:
        return

    # Open serial connection
    with serial.Serial(port, baudrate=115200, timeout=1) as ser, open(CSV_FILENAME, "w", newline="") as csvfile:
        print(f"Connected to {port}")

        # Setup CSV writer
        csv_writer = csv.DictWriter(csvfile, fieldnames=["timestamp", "baroPressurePa", "baroTemperatureK", "baroAltitudeM"])
        csv_writer.writeheader()  # Write CSV header

        # Send "offload" command
        ser.write(b"offload\n")
        ser.reset_input_buffer()
        print(f"Reading data... Logging to {CSV_FILENAME}")

        while True:
            raw_data = ser.read(LOG_DATA_SIZE)

            if len(raw_data) < LOG_DATA_SIZE:
                print("No more data received.")
                break

            log_entry = parse_log_data(raw_data)
            print(log_entry)

            # Write to CSV
            csv_writer.writerow(log_entry)

        print(f"Data saved to {CSV_FILENAME}")

if __name__ == "__main__":
    main()