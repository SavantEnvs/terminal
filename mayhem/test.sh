#!/usr/bin/env bash
#
# mayhem/test.sh — run the known-answer self-test that mayhem/build.sh built
# (mayhem/tests/selftest.cpp, normal flags). Upstream's own suites are TAEF tests that
# only build and run on Windows; the self-test checks the same code the fuzz targets
# reach (VT output/input parsing, OSC colour/clipboard/hyperlinks, DECDLD, DECDMAC,
# til::parse_*, til::u8u16, CodepointWidthDetector) against expected values.
# It prints one PASS/FAIL line per check and "SUMMARY passed=<n> failed=<n>"; the counts
# come from that line, so a neutered binary (no output) fails.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

runner=mayhem/build/test/selftest
if [ ! -x "$runner" ]; then
  echo "test.sh: $runner is missing — run mayhem/build.sh first" >&2
  emit_ctrf "terminal-selftest" 0 1
  exit 1
fi

out="$("$runner" 2>&1)"
rc=$?
printf '%s\n' "$out"

summary="$(printf '%s\n' "$out" | grep -E '^SUMMARY passed=[0-9]+ failed=[0-9]+$' | tail -n1)"
if [ -z "$summary" ]; then
  echo "test.sh: no SUMMARY line from $runner (exit $rc)" >&2
  emit_ctrf "terminal-selftest" 0 1
  exit 1
fi
passed="$(sed -E 's/.*passed=([0-9]+).*/\1/' <<<"$summary")"
failed="$(sed -E 's/.*failed=([0-9]+).*/\1/' <<<"$summary")"
# The PASS lines must agree with the summary, and a clean run must exit 0.
lines="$(printf '%s\n' "$out" | grep -c '^PASS ')"
if [ "$lines" -ne "$passed" ] || { [ "$failed" -eq 0 ] && [ "$rc" -ne 0 ]; }; then
  failed=$(( failed + 1 ))
fi
emit_ctrf "terminal-selftest" "$passed" "$failed"
