#!/bin/bash
# Reruns every throwaway test against the real BasicLoggerHelper.h. Usage: run_all.sh <iterations>
T="$(cd "$(dirname "$0")" && pwd)"
P="$T/../../../platformio_nuli_avionics_flight_software"
PY=${PY:-python}
ARM=${ARM:-~/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-g++.exe}
# Host g++ (MinGW on Windows - CLion bundles one). Set HOST_GXX_DIR to its bin folder if g++ is not on PATH.
# (cygpath turns a C:/... path into /c/..., since a drive colon would split PATH)
[ -n "$HOST_GXX_DIR" ] && export PATH="$(cygpath -u "$HOST_GXX_DIR" 2>/dev/null || echo "$HOST_GXX_DIR"):$PATH"
ITERS=${1:-500}
INC="-I $P/src -I $P/lib/Quaternion -I $T"
cd "$T"
total_bad=0

echo "=== 1. randomized differential suites"
"$PY" -I gen.py 1 300 gen_test.cpp >/dev/null
"$PY" -I gen.py 2 300 gen2.cpp >/dev/null
"$PY" -I gen.py 3 300 gen3.cpp >/dev/null
( g++ -std=c++14 -O1 $INC gen_test.cpp -o g1.exe 2>g1.err; echo "g1 build=$?" ) &
( g++ -std=c++14 -O2 -funsigned-char $INC gen2.cpp -o g2.exe 2>g2.err; echo "g2 build=$?" ) &
( g++ -std=c++17 -O0 $INC gen3.cpp -o g3.exe 2>g3.err; echo "g3 build=$?" ) &
wait
for e in g1 g2 g3; do
  if [ -f $e.exe ]; then ./$e.exe $ITERS | tail -4; [ ${PIPESTATUS[0]} -ne 0 ] && total_bad=$((total_bad+1)); else echo "$e did not build"; grep error $e.err | head -5; total_bad=$((total_bad+1)); fi
  rm -f $e.exe
done

echo "=== 2. edge cases"
"$PY" -I gen_edge.py && g++ -std=c++14 -O1 $INC edge.cpp -o edge.exe 2>&1 | grep error | head -5
./edge.exe || total_bad=$((total_bad+1))

echo "=== 3. compile-fail / compile-pass cases"
mkdir -p cf
while IFS='|' read -r name body use expect; do
  {
    echo '#define remove_struct_padding __attribute__((packed))'
    echo '#include "core/BasicLoggerHelper.h"'
    echo 'typedef float myFloat; enum Color { RED }; struct Inner { int x; };'
    echo "LOG_DATA_AUTO_DEFINE(DEF, $body)"
    echo 'struct Other { uint64_t q; } remove_struct_padding;'
    case $use in
      P) echo 'void p(const A& d, DebugStream* s) { BasicLoggerHelper::genericBasicLogDataPrinter<DEF>(d, s); }';;
      H) echo 'static constexpr auto& H = BasicLoggerHelper::generateBasicLogHeader<DEF>(); const char* use = H;';;
      W) echo 'void p(const Other& d, DebugStream* s) { BasicLoggerHelper::genericBasicLogDataPrinter<DEF>(d, s); }';;
      X) echo 'void p(const A* d, DebugStream* s) { BasicLoggerHelper::genericBasicLogDataPrinter<DEF>(d, s); }';;
    esac
  } > "cf/$name.cpp"
  out=$(g++ -std=c++14 -fsyntax-only $INC "cf/$name.cpp" 2>&1); rc=$?
  ours=$(echo "$out" | grep -c "static assertion failed")
  if [ "$expect" = PASS ]; then [ $rc -eq 0 ] && r=ok || r=WRONG
  else if [ $rc -ne 0 ] && [ $ours -gt 0 ]; then r=ok; elif [ $rc -ne 0 ]; then r=ok-other-error; else r=WRONG_COMPILED; fi; fi
  case $r in ok*) ;; *) total_bad=$((total_bad+1));; esac
  printf "  %-26s expect=%-4s %s\n" "$name" "$expect" "$r"
done < cases.txt

echo "=== 4. multi-TU link (c++14/17/20)"
for std in c++14 c++17 c++20; do
  g++ -std=$std $INC mtu/a.cpp mtu/b.cpp -o mtu/m.exe 2>/dev/null && ./mtu/m.exe >/dev/null; rc=$?
  echo "  $std exit=$rc"; [ $rc -ne 0 ] && total_bad=$((total_bad+1))
done

echo "=== 5. ARM GCC 9.3 (cortex-m0plus) compile of all suites"
for f in gen_test.cpp gen2.cpp edge.cpp; do for std in gnu++14 gnu++17; do
  "$ARM" -mcpu=cortex-m0plus -mthumb -Os -std=$std -fno-rtti -fno-exceptions $INC -c $f -o /dev/null 2>arm.err; rc=$?
  echo "  $f $std exit=$rc"; [ $rc -ne 0 ] && { grep error arm.err | head -3; total_bad=$((total_bad+1)); }
done; done

echo "=== TOTAL FAILING SECTIONS: $total_bad"
