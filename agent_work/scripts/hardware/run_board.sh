#!/bin/bash
# Captures everything a board says about its log format, for validate.js:
#   <prefix>_logformat.bin  --logFormat reply
#   <prefix>_text.bin       text offload
#   <prefix>_bin.bin        binary offload
#   <prefix>_stream.bin     6 s of --streamLog
# usage: run_board.sh PORT PREFIX      (PY defaults to python)
HERE="$(cd "$(dirname "$0")" && pwd)"
PY=${PY:-python}
P=$1; X=$2
$PY -I "$HERE/sg.py" $P ${X}_logformat.bin "--cmd=--logFormat" --idle 1.5 --max 10
$PY -I "$HERE/sg.py" $P ${X}_text.bin "--cmd=--offload" --until "Ending Offload" --max 1800
$PY -I "$HERE/sg.py" $P ${X}_bin.bin "--cmd=--offload -b" --until "Ending Offload" --max 1800
$PY -I "$HERE/sg.py" $P ${X}_stream.bin "--cmd=--streamLog -b" --gap 6 "--cmd=--streamLog -e" --idle 1.5 --max 20
