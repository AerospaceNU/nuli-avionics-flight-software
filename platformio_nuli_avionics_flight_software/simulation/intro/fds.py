import csv
import os

INPUT_FILE = "2023-04-15-beanboozler-FCB.csv"
OUTPUT_FILE = "test_data.h"

HEADER_NAME = "TEST_DATA_H"

def parse_int(value: str) -> int:
    # Handles "966,166" → 966166
    return int(value.replace(",", "").replace('"', ''))

def parse_float(value: str) -> float:
    # Handles "100,635" → 100635.0
    return float(value.replace(",", "").replace('"', ''))

def main():
    with open(INPUT_FILE, newline='', encoding='utf-8') as csvfile:
        reader = csv.DictReader(csvfile)
        rows = list(reader)

    with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
        f.write(f"#ifndef {HEADER_NAME}\n#define {HEADER_NAME}\n\n")
        f.write("struct Vector3D_s {\n    float x;\n    float y;\n    float z;\n};\n\n")
        f.write("struct Data {\n"
                "    uint32_t timestampMs;\n"
                "    float pressurePa;\n"
                "    float temperatureK;\n"
                "    Vector3D_s accelerationMSS;\n"
                "};\n\n")
        f.write("constexpr Data testData[] = {\n")

        for row in rows:
            timestamp = parse_int(row["timestamp_ms"])
            temp = parse_float(row["baro_temp_k"])
            pressure = parse_float(row["baro_pressure_pa"])
            ax = float(row["imu_acceleration_x_mss"])
            ay = float(row["imu_acceleration_y_mss"])
            az_key = [k for k in row.keys() if "z_mss" in k][0]  # handle the z column name
            az = float(row[az_key])

            f.write(f"    {{ {timestamp}, {pressure}f, {temp}f, {{ {ax}f, {ay}f, {az}f }} }},\n")

        f.write("};\n\n#endif\n")

    print(f"✅ Generated {OUTPUT_FILE} with {len(rows)} entries.")

if __name__ == "__main__":
    main()
