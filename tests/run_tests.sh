#!/usr/bin/env bash
# byvalver test runner
# Usage: bash tests/run_tests.sh [--mode full|baseline|verify-denulled] [--arch x86|x64|arm|all] [--verbose]

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$PROJECT_ROOT/bin/byvalver"
FIXTURES="$PROJECT_ROOT/tests/fixtures"
MANIFEST="$PROJECT_ROOT/tests/fixtures/manifest.yaml"
TMPDIR=$(mktemp -d)
PASS=0
FAIL=0
SKIP=0

MODE="full"
ARCH="all"
VERBOSE="${VERBOSE:-0}"
ARTIFACTS_DIR="$PROJECT_ROOT/ci-artifacts"
PROFILE_SET=""

cleanup() {
  rm -rf "$TMPDIR"
}
trap cleanup EXIT

usage() {
  cat <<USAGE
Usage: bash tests/run_tests.sh [options]

Options:
  --mode MODE             full (default), baseline, or verify-denulled
  --arch ARCH             x86 | x64 | arm | all (default)
  --profiles CSV          profile override (e.g. null-only,http-newline)
  --artifacts-dir PATH    output directory for verification logs (default: ci-artifacts)
  --verbose               print command output during execution
  -h, --help              show this help message
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
    --profiles)
      PROFILE_SET="${2:-}"
      shift 2
      ;;
    --artifacts-dir)
      ARTIFACTS_DIR="${2:-}"
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

if [[ "$MODE" != "full" && "$MODE" != "baseline" && "$MODE" != "verify-denulled" ]]; then
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

run_cmd_logged() {
  local log_file="$1"
  shift

  mkdir -p "$(dirname "$log_file")"

  if "$@" >"$log_file" 2>&1; then
    if [[ "$VERBOSE" -eq 1 ]]; then
      cat "$log_file"
    fi
    return 0
  fi

  if [[ "$VERBOSE" -eq 1 ]]; then
    cat "$log_file"
  fi
  return 1
}

preflight_check() {
  local missing=0

  check_tool() {
    local tool="$1"
    local install_hint="$2"
    if command -v "$tool" > /dev/null 2>&1; then
      log_pass "preflight: found $tool"
    else
      log_fail "preflight: missing $tool"
      echo "  install hint: $install_hint"
      missing=1
    fi
  }

  check_tool gcc "Install build-essential (Ubuntu/Debian) or Xcode Command Line Tools (macOS)."
  check_tool make "Install build-essential (Ubuntu/Debian) or Xcode Command Line Tools (macOS)."
  check_tool nasm "Install nasm (apt install nasm / brew install nasm)."
  check_tool xxd "Install xxd (vim-common on Linux, brew install vim on macOS)."
  check_tool pkg-config "Install pkg-config (apt install pkg-config / brew install pkg-config)."
  check_tool python3 "Install python3 (apt install python3 / brew install python)."

  if pkg-config --exists capstone > /dev/null 2>&1; then
    log_pass "preflight: found Capstone via pkg-config"
  else
    log_fail "preflight: missing Capstone pkg-config entry"
    echo "  install hint: install libcapstone-dev (Ubuntu/Debian) or capstone (Homebrew)."
    missing=1
  fi

  return "$missing"
}

resolve_target_arches() {
  if [[ "$ARCH" == "all" ]]; then
    TARGET_ARCHES=(x86 x64 arm)
  else
    TARGET_ARCHES=("$ARCH")
  fi
}

sanitize_name() {
  echo "$1" | tr '/ ' '__' | tr -cd '[:alnum:]_.-'
}

manifest_representatives_for_arch() {
  local target_arch="$1"

  python3 - "$MANIFEST" "$target_arch" <<'PY'
import re
import sys

manifest_path = sys.argv[1]
target_arch = sys.argv[2]

required_fields = ["fixture_id", "arch", "path", "expected_outcome", "owner", "notes"]
entries = []
current = None
in_fixtures = False

try:
    with open(manifest_path, "r", encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.rstrip("\n")
            stripped = line.strip()

            if not stripped or stripped.startswith("#"):
                continue

            if stripped == "fixtures:":
                in_fixtures = True
                continue

            if not in_fixtures:
                continue

            if line.startswith("  - "):
                if current:
                    entries.append(current)
                current = {}
                remainder = line[4:].strip()
                if ":" in remainder:
                    key, value = remainder.split(":", 1)
                    current[key.strip()] = value.strip().strip('"\'')
                continue

            if current is None:
                continue

            if line.startswith("    ") and ":" in line:
                key, value = line.strip().split(":", 1)
                current[key.strip()] = value.strip().strip('"\'')

    if current:
        entries.append(current)
except FileNotFoundError:
    print(f"manifest missing: {manifest_path}", file=sys.stderr)
    sys.exit(2)

selected = []
for entry in entries:
    missing = [field for field in required_fields if not entry.get(field)]
    if missing:
        print(
            f"manifest entry missing required fields {','.join(missing)} for fixture_id={entry.get('fixture_id', '<unknown>')}",
            file=sys.stderr,
        )
        sys.exit(2)

    representative = entry.get("ci_representative", "false").lower() == "true"
    if not representative or entry["arch"] != target_arch:
        continue

    fixture_profiles = entry.get("profiles", "").strip()
    selected.append(
        {
            "fixture_id": entry["fixture_id"],
            "path": entry["path"],
            "profiles": fixture_profiles,
        }
    )

selected.sort(key=lambda item: item["fixture_id"])

if not selected:
    print(f"no representative fixtures selected for arch={target_arch}", file=sys.stderr)
    sys.exit(3)

for item in selected:
    print(f"{item['fixture_id']}\t{item['path']}\t{item['profiles']}")
PY
}

verify_binary_available() {
  if [[ -x "$BIN" ]]; then
    log_pass "verification binary available at $BIN"
    return 0
  fi

  log_fail "verification binary missing at $BIN"
  echo "  install hint: run 'make' from the project root before verification modes."
  return 1
}

required_profiles_for_arch() {
  local target_arch="$1"

  if [[ -n "$PROFILE_SET" ]]; then
    echo "$PROFILE_SET"
    return 0
  fi

  case "$target_arch" in
    x86|x64)
      echo "null-only,http-newline"
      ;;
    arm)
      echo "null-only"
      ;;
    *)
      echo "unsupported arch for profile mapping: $target_arch" >&2
      return 1
      ;;
  esac
}

select_profiles_for_fixture() {
  local required_csv="$1"
  local fixture_csv="$2"

  if [[ -z "$fixture_csv" ]]; then
    echo "$required_csv"
    return 0
  fi

  local filtered=()
  local required_profile
  local fixture_profile
  IFS=',' read -r -a required_profiles <<<"$required_csv"
  IFS=',' read -r -a fixture_profiles <<<"$fixture_csv"

  for required_profile in "${required_profiles[@]}"; do
    required_profile="$(echo "$required_profile" | tr -d '[:space:]')"
    [[ -n "$required_profile" ]] || continue

    for fixture_profile in "${fixture_profiles[@]}"; do
      fixture_profile="$(echo "$fixture_profile" | tr -d '[:space:]')"
      if [[ "$required_profile" == "$fixture_profile" ]]; then
        filtered+=("$required_profile")
        break
      fi
    done
  done

  if [[ ${#filtered[@]} -eq 0 ]]; then
    return 1
  fi

  local joined=""
  local profile
  for profile in "${filtered[@]}"; do
    if [[ -n "$joined" ]]; then
      joined+=","
    fi
    joined+="$profile"
  done

  echo "$joined"
}

run_verify_denulled_mode() {
  local mode_failed=0
  local target_arch fixture_rows fixture_row fixture_id fixture_path fixture_profiles
  local fixture_abs safe_id output transform_log required_profiles selected_profiles
  local profile verify_log

  echo ""
  echo "[1/1] Profile-aware bad-byte verification"

  mkdir -p "$ARTIFACTS_DIR"
  resolve_target_arches

  for target_arch in "${TARGET_ARCHES[@]}"; do
    mapfile -t fixture_rows < <(manifest_representatives_for_arch "$target_arch") || {
      log_fail "manifest representative selection failed for arch=$target_arch"
      mode_failed=1
      continue
    }

    if [[ ${#fixture_rows[@]} -eq 0 ]]; then
      log_fail "no representative fixtures found for arch=$target_arch"
      mode_failed=1
      continue
    fi

    for fixture_row in "${fixture_rows[@]}"; do
      IFS=$'\t' read -r fixture_id fixture_path fixture_profiles <<<"$fixture_row"

      fixture_abs="$fixture_path"
      if [[ "$fixture_abs" != /* ]]; then
        fixture_abs="$PROJECT_ROOT/$fixture_path"
      fi

      if [[ ! -r "$fixture_abs" ]]; then
        log_fail "fixture_id=$fixture_id missing/unreadable path=$fixture_abs"
        mode_failed=1
        continue
      fi

      safe_id="$(sanitize_name "$fixture_id")"
      output="$TMPDIR/${target_arch}_${safe_id}.bin"
      transform_log="$ARTIFACTS_DIR/verify-${target_arch}-transform-${safe_id}.log"

      if run_cmd_logged "$transform_log" "$BIN" --arch "$target_arch" "$fixture_abs" "$output"; then
        log_pass "fixture_id=$fixture_id arch=$target_arch transformed"
      else
        log_fail "fixture_id=$fixture_id arch=$target_arch transformation failed (see $transform_log)"
        mode_failed=1
        continue
      fi

      required_profiles="$(required_profiles_for_arch "$target_arch")"
      if ! selected_profiles="$(select_profiles_for_fixture "$required_profiles" "$fixture_profiles")"; then
        log_fail "fixture_id=$fixture_id arch=$target_arch has no matching profiles (required=$required_profiles fixture=$fixture_profiles)"
        mode_failed=1
        continue
      fi

      IFS=',' read -r -a profile_list <<<"$selected_profiles"
      for profile in "${profile_list[@]}"; do
        profile="$(echo "$profile" | tr -d '[:space:]')"
        [[ -n "$profile" ]] || continue

        verify_log="$ARTIFACTS_DIR/verify-${target_arch}-denulled-${profile}-${safe_id}.log"
        if run_cmd_logged "$verify_log" python3 "$PROJECT_ROOT/verify_denulled.py" --profile "$profile" "$output"; then
          log_pass "fixture_id=$fixture_id arch=$target_arch --profile $profile"
        else
          log_fail "fixture_id=$fixture_id arch=$target_arch --profile $profile failed (see $verify_log)"
          mode_failed=1
        fi
      done
    done
  done

  return "$mode_failed"
}

echo "========================================"
echo " byvalver test suite"
echo "========================================"
echo " mode: $MODE"
echo " arch: $ARCH"
echo " profiles: ${PROFILE_SET:-<default-for-arch>}"
echo " artifacts_dir: $ARTIFACTS_DIR"
echo " verbose: $VERBOSE"
echo " output: concise by default, verbose when --verbose is set"
echo ""

# ----------------------------------------------------------
# 0. Preflight checks
# ----------------------------------------------------------
echo "[0/4] Preflight checks"
if preflight_check; then
  :
else
  echo ""
  echo "Preflight failed -- cannot continue."
  exit 1
fi

if [[ "$MODE" == "verify-denulled" ]]; then
  echo ""
  echo "[1/4] Verification-mode prerequisites"
  if verify_binary_available; then
    :
  else
    echo ""
    echo "Verification mode prerequisites failed -- cannot continue."
    exit 1
  fi

  run_verify_denulled_mode

  echo ""
  echo "========================================"
  printf " Results: %d passed, %d failed, %d skipped\n" "$PASS" "$FAIL" "$SKIP"
  echo "========================================"

  [[ "$FAIL" -eq 0 ]] && exit 0 || exit 1
fi

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
