# parse_rawOffloadDF.py
import re

filename = "data/offload_20251117_184136_flight2.txt"

timestamps = []

with open(filename, "r") as f:
    for line in f:
        line = line.strip()
        # Skip empty or non-numeric lines
        if not line or line.startswith("timestamp") or "Logger" in line:
            continue
        # Extract first number (timestampMs)
        parts = re.split(r'\s+', line)
        try:
            ts = int(parts[0])
            timestamps.append(ts)
        except ValueError:
            pass  # skip malformed lines

# Calculate and check dt
if len(timestamps) < 2:
    print("Not enough data rows.")
else:
    last_dt = timestamps[1] - timestamps[0]
    for i in range(2, len(timestamps)):
        dt = timestamps[i] - timestamps[i - 1]
        if dt != last_dt:
            print(f"Timestamp {timestamps[i]}: dt changed from {last_dt} → {dt}")
            last_dt = dt
