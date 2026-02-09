#!/usr/bin/env bash
# byvalver test runner
# Usage: bash tests/run_tests.sh

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$PROJECT_ROOT/bin/byvalver"
FIXTURES="$PROJECT_ROOT/tests/fixtures"
TMPDIR=$(mktemp -d)
PASS=0
FAIL=0
SKIP=0

cleanup() { rm -rf "$TMPDIR"; }
trap cleanup EXIT

log_pass() { PASS=$((PASS + 1)); printf "  [PASS] %s\n" "$1"; }
log_fail() { FAIL=$((FAIL + 1)); printf "  [FAIL] %s\n" "$1"; }
log_skip() { SKIP=$((SKIP + 1)); printf "  [SKIP] %s\n" "$1"; }

echo "========================================"
echo " byvalver test suite"
echo "========================================"
echo ""

# ----------------------------------------------------------
# 1. Build verification
# ----------------------------------------------------------
echo "[1/4] Build verification"
if make -C "$PROJECT_ROOT" clean > /dev/null 2>&1 && make -C "$PROJECT_ROOT" > /dev/null 2>&1; then
    log_pass "make clean && make succeeded"
else
    log_fail "build failed"
    echo ""
    echo "Build failed -- cannot continue."
    exit 1
fi

if [ -x "$BIN" ]; then
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

if "$BIN" --help > /dev/null 2>&1; then
    log_pass "--help exits successfully"
else
    log_fail "--help failed"
fi

if "$BIN" --version > /dev/null 2>&1; then
    log_pass "--version exits successfully"
else
    log_fail "--version failed"
fi

if "$BIN" --list-profiles > /dev/null 2>&1; then
    log_pass "--list-profiles exits successfully"
else
    log_fail "--list-profiles failed"
fi

# ----------------------------------------------------------
# 3. Transformation tests on fixtures
# ----------------------------------------------------------
echo ""
echo "[3/4] Transformation tests"

fixture_count=0
for arch_dir in "$FIXTURES"/*/; do
    [ -d "$arch_dir" ] || continue
    arch=$(basename "$arch_dir")
    for input in "$arch_dir"/*.bin; do
        [ -f "$input" ] || continue
        fixture_count=$((fixture_count + 1))
        name=$(basename "$input")
        output="$TMPDIR/${arch}_${name}"

        if "$BIN" --arch "$arch" "$input" "$output" > /dev/null 2>&1; then
            # Verify no null bytes remain
            if python3 "$PROJECT_ROOT/verify_denulled.py" "$output" > /dev/null 2>&1; then
                log_pass "$arch/$name -- transformed and verified"
            else
                log_fail "$arch/$name -- null bytes remain after transformation"
            fi
        else
            log_fail "$arch/$name -- transformation failed"
        fi
    done
done

if [ "$fixture_count" -eq 0 ]; then
    log_skip "no fixture binaries found in $FIXTURES"
    echo "  Hint: copy test binaries into tests/fixtures/x86/ or tests/fixtures/x64/"
fi

# ----------------------------------------------------------
# 4. Batch processing test (using assets/tests if available)
# ----------------------------------------------------------
echo ""
echo "[4/4] Batch processing test"

ASSET_TESTS="$PROJECT_ROOT/assets/tests"
if [ -d "$ASSET_TESTS" ] && ls "$ASSET_TESTS"/*.bin > /dev/null 2>&1; then
    batch_out="$TMPDIR/batch_output"
    mkdir -p "$batch_out"
    if "$BIN" -r --pattern "*.bin" "$ASSET_TESTS" "$batch_out" > /dev/null 2>&1; then
        log_pass "batch processing completed on assets/tests"
    else
        log_fail "batch processing returned non-zero"
    fi
else
    log_skip "no assets/tests/*.bin found for batch test"
fi

# ----------------------------------------------------------
# Summary
# ----------------------------------------------------------
echo ""
echo "========================================"
printf " Results: %d passed, %d failed, %d skipped\n" "$PASS" "$FAIL" "$SKIP"
echo "========================================"

[ "$FAIL" -eq 0 ] && exit 0 || exit 1
