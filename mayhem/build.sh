#!/usr/bin/env bash
#
# mayhem/build.sh — build the Linux fuzz targets and the known-answer self-test for the
# portable core of Windows Terminal (VT parser, til, CodepointWidthDetector).
#
# Upstream builds only with MSBuild/MSVC on Windows. The parser, til and the width
# detector are plain C++20, so this compiles upstream's own .cpp files unmodified with
# clang, putting mayhem/shim/include first on the include path: it stands in for
# <windows.h>, SAL, WIL's error macros and TraceLogging (see mayhem/shim/README.md).
# GSL and {fmt} (upstream's vcpkg deps) come pinned from /opt/toolchains/deps.
#
# Outputs: /mayhem/<target> (libFuzzer), /mayhem/<target>-standalone (reproducer),
# mayhem/build/test/selftest (normal flags; mayhem/test.sh runs it).
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
: "${GSL_DIR:=/opt/toolchains/deps/gsl-4.2.1}"
: "${FMT_DIR:=/opt/toolchains/deps/fmt-12.1.0}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"
B=mayhem/build
mkdir -p "$B/gen" "$B/fuzz" "$B/test"

# src/features.xml -> TilFeatureStaging.h (MSBuild force-includes the generated header).
python3 mayhem/gen_features.py src/features.xml "$B/gen/TilFeatureStaging.h"

COMMON=(
  -std=c++20 -fno-strict-aliasing -Wno-everything
  -include "$B/gen/TilFeatureStaging.h"
  -I mayhem/shim/include
  -I mayhem/shim/case/d1/d2 -I mayhem/shim/case/d1 -I mayhem/shim/case/types
  -I src/inc -I src -I src/types -I oss/chromium -I oss/interval_tree
  -I "$GSL_DIR/include" -I "$FMT_DIR/include"
)

# Upstream sources (compiled verbatim) + the shim's Win32 definitions.
SOURCES=(
  src/terminal/parser/stateMachine.cpp
  src/terminal/parser/OutputStateMachineEngine.cpp
  src/terminal/parser/InputStateMachineEngine.cpp
  src/terminal/parser/base64.cpp
  src/terminal/parser/tracing.cpp
  src/terminal/adapter/FontBuffer.cpp
  src/terminal/adapter/MacroBuffer.cpp
  src/types/utils.cpp
  src/types/colorTable.cpp
  src/types/CodepointWidthDetector.cpp
  mayhem/shim/win32_shim.cpp
)

# compile <outdir> <flags...>: every SOURCE -> <outdir>/<path with / -> _>.o, MAYHEM_JOBS at a time.
compile() {
  local outdir="$1"; shift
  local pids=() s fail=0
  for s in "${SOURCES[@]}"; do
    "$CXX" "${COMMON[@]}" "$@" -c "$s" -o "$outdir/$(echo "$s" | tr / _).o" &
    pids+=($!)
    if [ "${#pids[@]}" -ge "$MAYHEM_JOBS" ]; then
      wait "${pids[0]}" || fail=1
      pids=("${pids[@]:1}")
    fi
  done
  for s in "${pids[@]}"; do wait "$s" || fail=1; done
  [ "$fail" -eq 0 ]
}

HARNESSES=(fuzz_vt_output fuzz_vt_input fuzz_til_parse fuzz_u8u16 fuzz_grapheme)

# 1) Fuzz build: upstream code + harnesses with ASan/UBSan and DWARF 3.
#    -fsanitize=fuzzer-no-link puts SanCov on upstream's objects, not just the harness TU.
FUZZ_FLAGS=($SANITIZER_FLAGS $DEBUG_FLAGS -O1)
compile "$B/fuzz" "${FUZZ_FLAGS[@]}" -fsanitize=fuzzer-no-link
"$CXX" "${FUZZ_FLAGS[@]}" -c mayhem/lsan_off.cc -o "$B/fuzz/lsan_off.o"
"$CC" $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$B/fuzz/standalone_main.o"
"$CC" $SANITIZER_FLAGS $DEBUG_FLAGS -c mayhem/sancov_stubs.c -o "$B/fuzz/sancov_stubs.o"
FUZZ_OBJS=("$B"/fuzz/src_*.o "$B"/fuzz/mayhem_*.o "$B/fuzz/lsan_off.o")
for h in "${HARNESSES[@]}"; do
  "$CXX" "${COMMON[@]}" "${FUZZ_FLAGS[@]}" -fsanitize=fuzzer-no-link -c "mayhem/harness/$h.cpp" -o "$B/fuzz/h_$h.o"
  "$CXX" "${FUZZ_FLAGS[@]}" $LIB_FUZZING_ENGINE "$B/fuzz/h_$h.o" "${FUZZ_OBJS[@]}" \
    "$FMT_DIR/lib/libfmt.a" -o "/mayhem/$h"
  "$CXX" "${FUZZ_FLAGS[@]}" "$B/fuzz/standalone_main.o" "$B/fuzz/h_$h.o" "${FUZZ_OBJS[@]}" "$B/fuzz/sancov_stubs.o" \
    "$FMT_DIR/lib/libfmt.a" -o "/mayhem/$h-standalone"
done

# 2) Self-test (known-answer tests): same upstream sources, normal flags, no sanitizers.
TEST_FLAGS=(-O1 $COVERAGE_FLAGS)
compile "$B/test" "${TEST_FLAGS[@]}"
"$CXX" "${COMMON[@]}" "${TEST_FLAGS[@]}" mayhem/tests/selftest.cpp "$B"/test/src_*.o "$B"/test/mayhem_*.o \
  "$FMT_DIR/lib/libfmt.a" -o "$B/test/selftest"

echo "built: ${HARNESSES[*]} (+ -standalone) and $B/test/selftest"
