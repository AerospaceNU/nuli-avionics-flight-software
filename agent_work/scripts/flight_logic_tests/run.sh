#!/bin/bash
# Builds flight_gate_test from the working tree's core sources and runs every scenario against the
# flight logs in ../rocket-flight-data. With BASELINE=<git rev> it also builds the flight logic
# (Avionics.h, Barometer.h, StateEstimator1D, FlightStateDeterminer) from that revision, to show the
# dead-barometer scenarios failing there. Ends with "TOTAL FAILURES: 0".
# Host g++ (C++17+): on PATH, or HOST_GXX_DIR pointing at its bin folder.
T="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$T/../../.." && pwd)"
FW="$REPO/platformio_nuli_avionics_flight_software"
DATA="${FLIGHT_DATA:-$REPO/../rocket-flight-data/flight-data}"
PY=${PY:-python}
[ -n "$HOST_GXX_DIR" ] && export PATH="$(cygpath -u "$HOST_GXX_DIR" 2>/dev/null || echo "$HOST_GXX_DIR"):$PATH"
cd "$T"

build() { # build OUT SRC_ROOT [EXTRA_FLAGS]
  local out=$1 src=$2
  g++ -std=gnu++17 -O1 -w -DDESKTOP $3 -I "$src" -I "$FW/include" -I "$FW/lib/Quaternion" -I "$FW/lib/Eigen" \
      -I "$FW/lib/Embedded Template Library/include" \
      flight_gate_test.cpp $(find "$src/core" -name '*.cpp') "$FW/lib/Quaternion/Quaternion.cpp" -o "$out"
}

build flight_gate_test.exe "$FW/src" || { echo "build failed"; exit 1; }
if [ -n "$BASELINE" ]; then
  rm -rf baseline_src && cp -r "$FW/src" baseline_src
  for f in Avionics.h core/generic_hardware/Barometer.h core/state_estimation/StateEstimator1D.cpp \
           core/state_estimation/StateEstimator1D.h core/state_estimation/FlightStateDeterminer.cpp \n           core/state_estimation/FlightStateDeterminer.h core/filters/KalmanFilter1D.cpp; do
    git -C "$REPO" show "$BASELINE:platformio_nuli_avionics_flight_software/src/$f" > "baseline_src/$f"
  done
  build flight_gate_baseline.exe "$T/baseline_src" -DBASELINE_BUILD || { echo "baseline build failed"; exit 1; }
fi

fails=0
for spec in "2025-11-15 MBTA/SillyGoose Ridealong 1/2025-11-15 V1 MBTA ridealong board1.txt|11000116" \
            "2025-11-15 Crocket/SillyGoose Backup Backup/2025-11-15 V1 Crocket backup.txt|1150000" \
            "2025-11-15 Nathan McGuire Cert/SillyGoose Ridealong/2025-11-15 V1 Nathan McGuire Cert.txt|1020000"; do
  log="${spec%%|*}"; end="${spec##*|}"
  name=$(basename "$log" .txt | tr ' ' '_')
  $PY -I make_samples.py "$DATA/$log" "samples_$name.csv" "$end" > /dev/null
  echo "=== $name"
  for sc in normal dead_baro baro_dies_on_pad dead_baro_unknown_boot glitch_in_flight; do
    ./flight_gate_test.exe "samples_$name.csv" $sc > "out_${name}_$sc.txt"; rc=$?
    tail -1 "out_${name}_$sc.txt"; [ $rc -ne 0 ] && fails=$((fails+1))
    if [ -n "$BASELINE" ]; then
      ./flight_gate_baseline.exe "samples_$name.csv" $sc > "base_${name}_$sc.txt"
      echo "    baseline ($BASELINE): $(tail -1 "base_${name}_$sc.txt" | sed 's/ (.*//')"
    fi
  done
done
echo "TOTAL FAILURES: $fails"
