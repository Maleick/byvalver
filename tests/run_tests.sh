#!/usr/bin/env bash
# byvalver test runner
# Usage: bash tests/run_tests.sh [--mode full|baseline] [--arch x86|x64|arm|all] [--verbose]

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$PROJECT_ROOT/bin/byvalver"
FIXTURES="$PROJECT_ROOT/tests/fixtures"
TMPDIR=$(mktemp -d)
PASS=0
FAIL=0
SKIP=0

MODE="full"
ARCH="all"
VERBOSE=0

cleanup() { rm -rf "$TMPDIR"; }
trap cleanup EXIT

usage() {
  cat <<USAGE
Usage: bash tests/run_tests.sh [options]

Options:
  --mode MODE     full (default) or baseline
  --arch ARCH     x86 | x64 | arm | all (default)
  --verbose       print command output during execution
  -h, --help      show this help message
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --mode)
      MODE="${2:-}"
      shift 2
      ;;
    --arch)
      ARCH="${2:-}"
      shift 2
      ;;
    --verbose)
      VERBOSE=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1"
      usage
      exit 1
      ;;
  esac
done

if [[ "$MODE" != "full" && "$MODE" != "baseline" ]]; then
  echo "Invalid --mode value: $MODE"
  exit 1
fi

if [[ "$ARCH" != "all" && "$ARCH" != "x86" && "$ARCH" != "x64" && "$ARCH" != "arm" ]]; then
  echo "Invalid --arch value: $ARCH"
  exit 1
fi

log_pass() { PASS=$((PASS + 1)); printf "  [PASS] %s\n" "$1"; }
log_fail() { FAIL=$((FAIL + 1)); printf "  [FAIL] %s\n" "$1"; }
log_skip() { SKIP=$((SKIP + 1)); printf "  [SKIP] %s\n" "$1"; }

run_cmd() {
  if [[ "$VERBOSE" -eq 1 ]]; then
    "$@"
  else
    "$@" > /dev/null 2>&1
  fi
}

echo "========================================"
echo " byvalver test suite"
echo "========================================"
echo " mode: $MODE"
echo " arch: $ARCH"
echo " verbose: $VERBOSE"
echo ""

# ----------------------------------------------------------
# 1. Build verification
# ----------------------------------------------------------
echo "[1/4] Build verification"
if run_cmd make -C "$PROJECT_ROOT" clean && run_cmd make -C "$PROJECT_ROOT"; then
  log_pass "make clean && make succeeded"
else
  log_fail "build failed"
  echo ""
  echo "Build failed -- cannot continue."
  exit 1
fi

if [[ -x "$BIN" ]]; then
  log_pass "executable exists and is executable"
else
  log_fail "executable not found at $BIN"
  exit 1
fi

# ----------------------------------------------------------
# 2. CLI smoke tests
# ----------------------------------------------------------
echo ""
echo "[2/4] CLI smoke tests"

if run_cmd "$BIN" --help; then
  log_pass "--help exits successfully"
else
  log_fail "--help failed"
fi

if run_cmd "$BIN" --version; then
  log_pass "--version exits successfully"
else
  log_fail "--version failed"
fi

if run_cmd "$BIN" --list-profiles; then
  log_pass "--list-profiles exits successfully"
else
  log_fail "--list-profiles failed"
fi

# ----------------------------------------------------------
# 3. Transformation tests on fixtures
# ----------------------------------------------------------
echo ""
echo "[3/4] Transformation tests"

if [[ "$ARCH" == "all" ]]; then
  TARGET_ARCHES=(x86 x64 arm)
else
  TARGET_ARCHES=("$ARCH")
fi

fixture_count=0
shopt -s nullglob

for arch in "${TARGET_ARCHES[@]}"; do
  arch_dir="$FIXTURES/$arch"

  if [[ ! -d "$arch_dir" ]]; then
    if [[ "$MODE" == "baseline" ]]; then
      log_fail "$arch fixtures missing at $arch_dir (baseline mode requires canonical fixtures)"
    else
      log_skip "$arch fixtures missing at $arch_dir"
    fi
    continue
  fi

  files=("$arch_dir"/*.bin)
  if [[ ${#files[@]} -eq 0 ]]; then
    if [[ "$MODE" == "baseline" ]]; then
      log_fail "$arch fixtures empty in $arch_dir (baseline mode requires curated fixtures)"
    else
      log_skip "$arch has no fixture binaries"
    fi
    continue
  fi

  for input in "${files[@]}"; do
    fixture_count=$((fixture_count + 1))
    name=$(basename "$input")
    output="$TMPDIR/${arch}_${name}"

    if run_cmd "$BIN" --arch "$arch" "$input" "$output"; then
      if run_cmd python3 "$PROJECT_ROOT/verify_denulled.py" "$output"; then
        log_pass "$arch/$name -- transformed and verified"
      else
        log_fail "$arch/$name -- null bytes remain after transformation"
      fi
    else
      log_fail "$arch/$name -- transformation failed"
    fi
  done
done

if [[ "$fixture_count" -eq 0 ]]; then
  if [[ "$MODE" == "baseline" ]]; then
    log_fail "no eligible fixture binaries found for baseline mode"
  else
    log_skip "no fixture binaries found in $FIXTURES"
    echo "  Hint: copy test binaries into tests/fixtures/x86/, tests/fixtures/x64/, or tests/fixtures/arm/"
  fi
fi

# ----------------------------------------------------------
# 4. Batch processing test (full mode only)
# ----------------------------------------------------------
echo ""
echo "[4/4] Batch processing test"

if [[ "$MODE" == "baseline" ]]; then
  log_skip "batch processing skipped in baseline mode"
else
  ASSET_TESTS="$PROJECT_ROOT/assets/tests"
  if [[ -d "$ASSET_TESTS" ]] && ls "$ASSET_TESTS"/*.bin > /dev/null 2>&1; then
    batch_out="$TMPDIR/batch_output"
    mkdir -p "$batch_out"
    if run_cmd "$BIN" -r --pattern "*.bin" "$ASSET_TESTS" "$batch_out"; then
      log_pass "batch processing completed on assets/tests"
    else
      log_fail "batch processing returned non-zero"
    fi
  else
    log_skip "no assets/tests/*.bin found for batch test"
  fi
fi

# ----------------------------------------------------------
# Summary
# ----------------------------------------------------------
echo ""
echo "========================================"
printf " Results: %d passed, %d failed, %d skipped\n" "$PASS" "$FAIL" "$SKIP"
echo "========================================"

[[ "$FAIL" -eq 0 ]] && exit 0 || exit 1
