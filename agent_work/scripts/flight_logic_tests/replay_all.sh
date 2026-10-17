#!/bin/bash
# Converts every Altus Metrum CSV under logs/external_data (plus the TARS / OSU logs named below) and
# replays each through real_replay.exe (working tree) and, if built, real_replay_head.exe (HEAD's
# flight code), printing one summary line per flight per build. Outputs in logs/replay/.
T="$(cd "$(dirname "$0")" && pwd)"; A="$T/../.."; X="$A/logs/external_data"; R="$A/logs/replay"; mkdir -p "$R"
PY=${PY:-python}
summ() { # file.out -> one line
  awk '/log apogee/{la=$0; sub(/.*log apogee /,"",la); sub(/ \(.*/,"",la)}
       /DESCENT /&&!d{d=$1" "$2} /DROGUE/{dr=$2" @"$NF" "$(NF-5); sub(/\(/,"",dr)} /MAIN /{m=$2" @"$5"m"} /POST_FLIGHT/{l=$2}
       /firmware max/{fm=$4} END{printf "apogee(log) %-20s descent %-9s drogue %-26s main %-18s landed %-9s fwmax %s\n", la, d, dr, m, l, fm}' "$1" | sed 's/t= *//g'
}
{
  grep -l "^#version" -r "$X" --include=*.csv | sed 's/^/altos|/'
  echo "tars|$X/issuiuc_flight_data/20221029/flight_computer.csv"
  echo "osu|$X/osu_bsli_irec/nomad_irec_2026_6_18/analysis/2026-06-17 Nomad IREC SRAD Cropped.logv3.csv"
} | while IFS='|' read fmt f; do
  name=$(echo "${f#$X/}" | tr ' /()' '____' | sed 's/\.csv$//; s/\.logv3$//')
  $PY -I "$T/to_replay.py" $fmt "$f" "$R/$name.csv" > /dev/null || continue
  "$T/real_replay.exe" "$R/$name.csv" > "$R/$name.new.out"
  echo "== $name"
  echo "  new  $(summ "$R/$name.new.out")"
  if [ -x "$T/real_replay_head.exe" ]; then
    "$T/real_replay_head.exe" "$R/$name.csv" > "$R/$name.head.out"
    echo "  HEAD $(summ "$R/$name.head.out")"
  fi
done
