#!/usr/bin/env bash
# byvalver test runner
# Usage: bash tests/run_tests.sh [--mode full|baseline|verify-denulled|verify-equivalence|verify-parity|release-gate] [--arch x86|x64|arm|all] [--verbose]

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
RESULT_ROWS_FILE="$TMPDIR/verify-results.tsv"

cleanup() {
  rm -rf "$TMPDIR"
}
trap cleanup EXIT

usage() {
  cat <<USAGE
Usage: bash tests/run_tests.sh [options]

Options:
  --mode MODE             full (default), baseline, verify-denulled, verify-equivalence, verify-parity, or release-gate
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

if [[ "$MODE" != "full" && "$MODE" != "baseline" && "$MODE" != "verify-denulled" && "$MODE" != "verify-equivalence" && "$MODE" != "verify-parity" && "$MODE" != "release-gate" ]]; then
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

record_verify_result() {
  local mode="$1"
  local arch="$2"
  local check="$3"
  local profile="$4"
  local fixture_id="$5"
  local fixture_path="$6"
  local output_path="$7"
  local status="$8"
  local log_path="$9"
  local message="${10}"

  arch="$(echo "$arch" | tr '[:upper:]' '[:lower:]')"
  status="$(echo "$status" | tr '[:lower:]' '[:upper:]')"

  if [[ -z "$profile" ]]; then
    profile="n/a"
  fi

  if [[ -n "$fixture_path" && "$fixture_path" == "$PROJECT_ROOT/"* ]]; then
    fixture_path="${fixture_path#"$PROJECT_ROOT/"}"
  fi

  if [[ -n "$log_path" && "$log_path" == "$PROJECT_ROOT/"* ]]; then
    log_path="${log_path#"$PROJECT_ROOT/"}"
  fi

  if [[ -n "$output_path" && "$output_path" == "$TMPDIR/"* ]]; then
    output_path="tmp://${arch}/${fixture_id}.bin"
  fi

  check="${check//$'\t'/ }"
  check="${check//$'\n'/ }"
  check="${check//$'\r'/ }"
  profile="${profile//$'\t'/ }"
  profile="${profile//$'\n'/ }"
  profile="${profile//$'\r'/ }"
  fixture_id="${fixture_id//$'\t'/ }"
  fixture_id="${fixture_id//$'\n'/ }"
  fixture_id="${fixture_id//$'\r'/ }"
  fixture_path="${fixture_path//$'\t'/ }"
  fixture_path="${fixture_path//$'\n'/ }"
  fixture_path="${fixture_path//$'\r'/ }"
  output_path="${output_path//$'\t'/ }"
  output_path="${output_path//$'\n'/ }"
  output_path="${output_path//$'\r'/ }"
  log_path="${log_path//$'\t'/ }"
  log_path="${log_path//$'\n'/ }"
  log_path="${log_path//$'\r'/ }"
  message="${message//$'\t'/ }"
  message="${message//$'\n'/ }"
  message="${message//$'\r'/ }"

  printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
    "$mode" "$arch" "$check" "$profile" "$fixture_id" "$fixture_path" "$output_path" "$status" "$log_path" "$message" \
    >> "$RESULT_ROWS_FILE"
}

emit_arch_mode_summary() {
  local target_arch="$1"
  local target_mode="$2"
  local output_json="$ARTIFACTS_DIR/summary-${target_arch}-${target_mode}.json"

  python3 - "$RESULT_ROWS_FILE" "$target_arch" "$target_mode" "$output_json" <<'PY'
import csv
import json
import sys
from collections import Counter
from datetime import datetime, timezone

rows_path, target_arch, target_mode, output_json = sys.argv[1:5]
field_names = [
    "mode",
    "arch",
    "check",
    "profile",
    "fixture_id",
    "fixture_path",
    "output_path",
    "status",
    "log_path",
    "message",
]


def normalize_text(value):
    text = "" if value is None else str(value)
    text = text.replace("\t", " ").replace("\n", " ").replace("\r", " ")
    return " ".join(text.split())


def normalize_status(value):
    status = normalize_text(value).upper()
    if status in {"PASS", "FAIL", "SKIP"}:
        return status
    return "UNKNOWN"


checks = []
tuples = []
status_counts = Counter()

try:
    with open(rows_path, "r", encoding="utf-8") as handle:
        reader = csv.DictReader(handle, fieldnames=field_names, delimiter="\t")
        for row in reader:
            if row["mode"] != target_mode or row["arch"] != target_arch:
                continue

            status = normalize_status(row["status"])
            status_counts[status] += 1

            check = {
                "check": normalize_text(row["check"]),
                "profile": normalize_text(row["profile"]) or "n/a",
                "fixture_id": normalize_text(row["fixture_id"]),
                "fixture_path": normalize_text(row["fixture_path"]),
                "output_path": normalize_text(row["output_path"]),
                "status": status,
                "log_path": normalize_text(row["log_path"]) or "n/a",
                "message": normalize_text(row["message"]),
            }
            checks.append(check)

            tuples.append(
                {
                    "arch": target_arch,
                    "fixture_id": check["fixture_id"],
                    "check": check["check"],
                    "status": check["status"],
                    "message": check["message"],
                    "log_path": check["log_path"],
                }
            )
except FileNotFoundError:
    checks = []
    tuples = []

checks.sort(
    key=lambda item: (
        item["fixture_id"],
        item["check"],
        item["profile"],
        item["status"],
        item["message"],
        item["log_path"],
    )
)
tuples.sort(
    key=lambda item: (
        item["fixture_id"],
        item["check"],
        item["status"],
        item["message"],
        item["log_path"],
    )
)

summary = {
    "schema_version": 2,
    "mode": target_mode,
    "arch": target_arch,
    "generated_at": datetime.now(timezone.utc).isoformat(),
    "tuple_fields": ["arch", "fixture_id", "check", "status", "message", "log_path"],
    "totals": {
        "checks": len(tuples),
        "pass": status_counts.get("PASS", 0),
        "fail": status_counts.get("FAIL", 0),
        "skip": status_counts.get("SKIP", 0),
    },
    "tuples": tuples,
    "checks": checks,
}

with open(output_json, "w", encoding="utf-8") as handle:
    json.dump(summary, handle, indent=2)
PY

  log_pass "wrote summary json $output_json"
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
  check_tool objdump "Install binutils (apt install binutils / brew install binutils)."

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
import sys

manifest_path = sys.argv[1]
target_arch = sys.argv[2]

required_fields = [
    "fixture_id",
    "arch",
    "path",
    "expected_outcome",
    "owner",
    "notes",
    "ci_representative",
    "profiles",
]
entries = []
current = None
in_fixtures = False
seen_fixture_ids = set()
selected_ids = set()
selected_paths = set()

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
    fixture_id = entry.get("fixture_id", "<unknown>")

    missing = [field for field in required_fields if not entry.get(field)]
    if missing:
        print(
            f"manifest entry missing required fields {','.join(missing)} for fixture_id={fixture_id}",
            file=sys.stderr,
        )
        sys.exit(2)

    if fixture_id in seen_fixture_ids:
        print(f"manifest duplicate fixture_id detected: {fixture_id}", file=sys.stderr)
        sys.exit(2)
    seen_fixture_ids.add(fixture_id)

    representative_raw = entry.get("ci_representative", "").strip().lower()
    if representative_raw not in ("true", "false"):
        print(
            f"manifest entry has invalid ci_representative value for fixture_id={fixture_id}: {entry.get('ci_representative')}",
            file=sys.stderr,
        )
        sys.exit(2)

    representative = representative_raw == "true"
    if not representative or entry["arch"] != target_arch:
        continue

    fixture_path = entry["path"]
    if fixture_id in selected_ids:
        print(
            f"manifest representative fixture_id is duplicated for arch={target_arch}: {fixture_id}",
            file=sys.stderr,
        )
        sys.exit(2)
    if fixture_path in selected_paths:
        print(
            f"manifest representative path is duplicated for arch={target_arch}: {fixture_path}",
            file=sys.stderr,
        )
        sys.exit(2)
    selected_ids.add(fixture_id)
    selected_paths.add(fixture_path)

    fixture_profiles = entry["profiles"].strip()
    selected.append(
        {
            "fixture_id": fixture_id,
            "path": fixture_path,
            "profiles": fixture_profiles,
        }
    )

selected.sort(key=lambda item: (item["fixture_id"], item["path"]))

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

verify_docker_compose_available() {
  if ! command -v docker > /dev/null 2>&1; then
    log_fail "docker is required for verify-parity mode"
    echo "  install hint: install Docker Desktop or docker engine."
    return 1
  fi

  if ! docker compose version > /dev/null 2>&1; then
    log_fail "docker compose plugin is required for verify-parity mode"
    echo "  install hint: install docker compose v2 plugin."
    return 1
  fi

  log_pass "docker + docker compose available for verify-parity"
  return 0
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
  local target_arch fixture_output fixture_rows fixture_row fixture_id fixture_path fixture_profiles
  local fixture_abs safe_id output transform_log required_profiles selected_profiles
  local profile verify_log

  echo ""
  echo "[1/1] Profile-aware bad-byte verification"

  mkdir -p "$ARTIFACTS_DIR"
  resolve_target_arches
  : > "$RESULT_ROWS_FILE"

  for target_arch in "${TARGET_ARCHES[@]}"; do
    fixture_output="$(manifest_representatives_for_arch "$target_arch" 2>&1)" || {
      log_fail "manifest representative selection failed for arch=$target_arch"
      record_verify_result "verify-denulled" "$target_arch" "fixture-selection" "n/a" "manifest" "$MANIFEST" "n/a" "FAIL" "n/a" "manifest representative selection failed: $fixture_output"
      emit_arch_mode_summary "$target_arch" "verify-denulled"
      mode_failed=1
      continue
    }

    fixture_rows=()
    while IFS= read -r fixture_row; do
      [[ -n "$fixture_row" ]] || continue
      fixture_rows+=("$fixture_row")
    done <<<"$fixture_output"

    if [[ ${#fixture_rows[@]} -eq 0 ]]; then
      log_fail "no representative fixtures found for arch=$target_arch"
      record_verify_result "verify-denulled" "$target_arch" "fixture-selection" "n/a" "manifest" "$MANIFEST" "n/a" "FAIL" "n/a" "no representative fixtures found"
      emit_arch_mode_summary "$target_arch" "verify-denulled"
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
        record_verify_result "verify-denulled" "$target_arch" "fixture-selection" "n/a" "$fixture_id" "$fixture_abs" "n/a" "FAIL" "n/a" "fixture missing or unreadable"
        mode_failed=1
        continue
      fi

      safe_id="$(sanitize_name "$fixture_id")"
      output="$TMPDIR/${target_arch}_${safe_id}.bin"
      transform_log="$ARTIFACTS_DIR/verify-${target_arch}-denulled-transform-${safe_id}.log"

      if run_cmd_logged "$transform_log" "$BIN" --arch "$target_arch" "$fixture_abs" "$output"; then
        log_pass "fixture_id=$fixture_id arch=$target_arch transformed"
        record_verify_result "verify-denulled" "$target_arch" "transform" "n/a" "$fixture_id" "$fixture_abs" "$output" "PASS" "$transform_log" "transformation completed"
      else
        log_fail "fixture_id=$fixture_id arch=$target_arch transformation failed (see $transform_log)"
        record_verify_result "verify-denulled" "$target_arch" "transform" "n/a" "$fixture_id" "$fixture_abs" "$output" "FAIL" "$transform_log" "transformation failed"
        mode_failed=1
        continue
      fi

      required_profiles="$(required_profiles_for_arch "$target_arch")"
      if ! selected_profiles="$(select_profiles_for_fixture "$required_profiles" "$fixture_profiles")"; then
        log_fail "fixture_id=$fixture_id arch=$target_arch has no matching profiles (required=$required_profiles fixture=$fixture_profiles)"
        record_verify_result "verify-denulled" "$target_arch" "profile-selection" "n/a" "$fixture_id" "$fixture_abs" "$output" "FAIL" "n/a" "no matching profiles"
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
          record_verify_result "verify-denulled" "$target_arch" "denulled" "$profile" "$fixture_id" "$fixture_abs" "$output" "PASS" "$verify_log" "verification passed"
        else
          log_fail "fixture_id=$fixture_id arch=$target_arch --profile $profile failed (see $verify_log)"
          record_verify_result "verify-denulled" "$target_arch" "denulled" "$profile" "$fixture_id" "$fixture_abs" "$output" "FAIL" "$verify_log" "verification failed"
          mode_failed=1
        fi
      done
    done

    emit_arch_mode_summary "$target_arch" "verify-denulled"
  done

  return "$mode_failed"
}

functionality_arch_for() {
  local target_arch="$1"
  case "$target_arch" in
    x64)
      echo "x64"
      ;;
    x86|arm)
      # verify_functionality.py accepts x86/x64; keep ARM checks deterministic via x86 analysis mode.
      echo "x86"
      ;;
    *)
      echo "x86"
      ;;
  esac
}

run_verify_equivalence_mode() {
  local mode_failed=0
  local target_arch fixture_output fixture_rows fixture_row fixture_id fixture_path fixture_profiles
  local fixture_abs safe_id output transform_log func_log sem_log func_arch

  echo ""
  echo "[1/1] Deterministic functionality and semantic verification"

  mkdir -p "$ARTIFACTS_DIR"
  resolve_target_arches
  : > "$RESULT_ROWS_FILE"

  for target_arch in "${TARGET_ARCHES[@]}"; do
    fixture_output="$(manifest_representatives_for_arch "$target_arch" 2>&1)" || {
      log_fail "manifest representative selection failed for arch=$target_arch"
      record_verify_result "verify-equivalence" "$target_arch" "fixture-selection" "n/a" "manifest" "$MANIFEST" "n/a" "FAIL" "n/a" "manifest representative selection failed: $fixture_output"
      emit_arch_mode_summary "$target_arch" "verify-equivalence"
      mode_failed=1
      continue
    }

    fixture_rows=()
    while IFS= read -r fixture_row; do
      [[ -n "$fixture_row" ]] || continue
      fixture_rows+=("$fixture_row")
    done <<<"$fixture_output"

    if [[ ${#fixture_rows[@]} -eq 0 ]]; then
      log_fail "no representative fixtures found for arch=$target_arch"
      record_verify_result "verify-equivalence" "$target_arch" "fixture-selection" "n/a" "manifest" "$MANIFEST" "n/a" "FAIL" "n/a" "no representative fixtures found"
      emit_arch_mode_summary "$target_arch" "verify-equivalence"
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
        log_fail "check=fixture-selection arch=$target_arch fixture_id=$fixture_id path=$fixture_abs missing"
        record_verify_result "verify-equivalence" "$target_arch" "fixture-selection" "n/a" "$fixture_id" "$fixture_abs" "n/a" "FAIL" "n/a" "fixture missing or unreadable"
        mode_failed=1
        continue
      fi

      safe_id="$(sanitize_name "$fixture_id")"
      output="$TMPDIR/${target_arch}_${safe_id}.bin"
      transform_log="$ARTIFACTS_DIR/verify-${target_arch}-equivalence-transform-${safe_id}.log"

      if run_cmd_logged "$transform_log" "$BIN" --arch "$target_arch" "$fixture_abs" "$output"; then
        log_pass "check=transform arch=$target_arch fixture_id=$fixture_id"
        record_verify_result "verify-equivalence" "$target_arch" "transform" "n/a" "$fixture_id" "$fixture_abs" "$output" "PASS" "$transform_log" "transformation completed"
      else
        log_fail "check=transform arch=$target_arch fixture_id=$fixture_id failed (see $transform_log)"
        record_verify_result "verify-equivalence" "$target_arch" "transform" "n/a" "$fixture_id" "$fixture_abs" "$output" "FAIL" "$transform_log" "transformation failed"
        mode_failed=1
        continue
      fi

      func_arch="$(functionality_arch_for "$target_arch")"
      func_log="$ARTIFACTS_DIR/verify-${target_arch}-functionality-${safe_id}.log"
      if run_cmd_logged "$func_log" python3 "$PROJECT_ROOT/verify_functionality.py" "$fixture_abs" "$output" --arch "$func_arch"; then
        log_pass "check=functionality arch=$target_arch fixture_id=$fixture_id analysis_arch=$func_arch"
        record_verify_result "verify-equivalence" "$target_arch" "functionality" "$func_arch" "$fixture_id" "$fixture_abs" "$output" "PASS" "$func_log" "functionality verification passed"
      else
        log_fail "check=functionality arch=$target_arch fixture_id=$fixture_id failed (see $func_log)"
        record_verify_result "verify-equivalence" "$target_arch" "functionality" "$func_arch" "$fixture_id" "$fixture_abs" "$output" "FAIL" "$func_log" "functionality verification failed"
        mode_failed=1
      fi

      sem_log="$ARTIFACTS_DIR/verify-${target_arch}-semantic-${safe_id}.log"
      if run_cmd_logged "$sem_log" python3 "$PROJECT_ROOT/verify_semantic.py" "$fixture_abs" "$output" --method pattern; then
        log_pass "check=semantic arch=$target_arch fixture_id=$fixture_id"
        record_verify_result "verify-equivalence" "$target_arch" "semantic" "pattern" "$fixture_id" "$fixture_abs" "$output" "PASS" "$sem_log" "semantic verification passed"
      else
        log_fail "check=semantic arch=$target_arch fixture_id=$fixture_id failed (see $sem_log)"
        record_verify_result "verify-equivalence" "$target_arch" "semantic" "pattern" "$fixture_id" "$fixture_abs" "$output" "FAIL" "$sem_log" "semantic verification failed"
        mode_failed=1
      fi
    done

    emit_arch_mode_summary "$target_arch" "verify-equivalence"
  done

  return "$mode_failed"
}

extract_embedded_summary_from_log() {
  local log_path="$1"
  local output_path="$2"

  awk '
    /^__GSD_PARITY_SUMMARY_BEGIN__$/ { capture=1; next }
    /^__GSD_PARITY_SUMMARY_END__$/   { capture=0; exit }
    capture { print }
  ' "$log_path" > "$output_path"

  [[ -s "$output_path" ]]
}

run_verify_parity_mode() {
  local mode_failed=0
  local artifacts_root host_artifacts docker_artifacts compare_artifacts
  local verify_mode target_arch host_summary docker_summary compare_json
  local host_mode_log docker_mode_log docker_build_log
  local summary_path docker_mode_cmd

  echo ""
  echo "[1/1] Host-vs-Docker verification parity"

  artifacts_root="$ARTIFACTS_DIR"
  if [[ "$artifacts_root" != /* ]]; then
    artifacts_root="$PROJECT_ROOT/$artifacts_root"
  fi

  case "$artifacts_root" in
    "$PROJECT_ROOT"|"$PROJECT_ROOT"/*)
      ;;
    *)
    log_fail "verify-parity requires --artifacts-dir under project root"
    echo "  received: $artifacts_root"
    echo "  expected prefix: $PROJECT_ROOT"
    return 1
      ;;
  esac

  host_artifacts="$artifacts_root/parity-host"
  docker_artifacts="$artifacts_root/parity-docker"
  compare_artifacts="$artifacts_root/parity-compare"

  mkdir -p "$host_artifacts" "$docker_artifacts" "$compare_artifacts"
  resolve_target_arches

  if [[ ! -x "$BIN" ]]; then
    host_mode_log="$artifacts_root/parity-host-build.log"
    if run_cmd_logged "$host_mode_log" /bin/bash -lc "mkdir -p \"$PROJECT_ROOT/bin/tui\" && make -C \"$PROJECT_ROOT\""; then
      log_pass "host binary built for parity mode"
    else
      log_fail "failed to build host binary for parity mode (see $host_mode_log)"
      return 1
    fi
  fi

  docker_build_log="$artifacts_root/parity-docker-build.log"
  if run_cmd_logged "$docker_build_log" docker compose run --rm -T --entrypoint /bin/bash parity -lc "mkdir -p /opt/byvalver/bin/tui && make -C /opt/byvalver"; then
    log_pass "docker parity environment built project binary"
  else
    log_fail "docker parity environment build failed (see $docker_build_log)"
    mode_failed=1
  fi

  for target_arch in "${TARGET_ARCHES[@]}"; do
    for verify_mode in verify-denulled verify-equivalence; do
      host_mode_log="$artifacts_root/parity-host-${target_arch}-${verify_mode}.log"
      host_cmd=(bash "$PROJECT_ROOT/tests/run_tests.sh" --mode "$verify_mode" --arch "$target_arch" --artifacts-dir "$host_artifacts")
      if [[ -n "$PROFILE_SET" ]]; then
        host_cmd+=(--profiles "$PROFILE_SET")
      fi
      if [[ "$VERBOSE" -eq 1 ]]; then
        host_cmd+=(--verbose)
      fi

      if run_cmd_logged "$host_mode_log" "${host_cmd[@]}"; then
        log_pass "host $verify_mode completed for arch=$target_arch"
      else
        log_fail "host $verify_mode failed for arch=$target_arch (see $host_mode_log)"
        mode_failed=1
      fi

      docker_mode_log="$artifacts_root/parity-docker-${target_arch}-${verify_mode}.log"
      docker_mode_cmd="tests/run_tests.sh --mode ${verify_mode} --arch ${target_arch} --artifacts-dir /tmp/parity-docker"
      if [[ -n "$PROFILE_SET" ]]; then
        docker_mode_cmd+=" --profiles ${PROFILE_SET}"
      fi
      if [[ "$VERBOSE" -eq 1 ]]; then
        docker_mode_cmd+=" --verbose"
      fi
      docker_mode_cmd+="; rc=\$?; summary=/tmp/parity-docker/summary-${target_arch}-${verify_mode}.json; if [ -f \"\$summary\" ]; then printf '__GSD_PARITY_SUMMARY_BEGIN__\n'; cat \"\$summary\"; printf '\n__GSD_PARITY_SUMMARY_END__\n'; fi; exit \$rc"

      if run_cmd_logged "$docker_mode_log" docker compose run --rm -T --entrypoint /bin/bash parity -lc "$docker_mode_cmd"; then
        log_pass "docker $verify_mode completed for arch=$target_arch"
      else
        log_fail "docker $verify_mode failed for arch=$target_arch (see $docker_mode_log)"
        mode_failed=1
      fi

      host_summary="$host_artifacts/summary-${target_arch}-${verify_mode}.json"
      docker_summary="$docker_artifacts/summary-${target_arch}-${verify_mode}.json"
      compare_json="$compare_artifacts/summary-${target_arch}-${verify_mode}-parity.json"
      summary_path="$docker_artifacts/summary-${target_arch}-${verify_mode}.json"

      if extract_embedded_summary_from_log "$docker_mode_log" "$summary_path"; then
        log_pass "extracted docker summary for arch=$target_arch mode=$verify_mode"
      else
        log_fail "missing embedded docker summary for arch=$target_arch mode=$verify_mode (see $docker_mode_log)"
        mode_failed=1
      fi

      if python3 - "$host_summary" "$docker_summary" "$compare_json" <<'PY'
import json
import os
import sys

host_path, docker_path, out_path = sys.argv[1:4]
mismatches = []

def load(path, label):
    if not os.path.exists(path):
        mismatches.append(f"{label} summary missing: {path}")
        return None
    with open(path, "r", encoding="utf-8") as handle:
        return json.load(handle)

def normalize(payload):
    totals = payload.get("totals", {})
    normalized_totals = {
        "checks": int(totals.get("checks", 0)),
        "pass": int(totals.get("pass", 0)),
        "fail": int(totals.get("fail", 0)),
        "skip": int(totals.get("skip", 0)),
    }

    checks = []
    for check in payload.get("checks", []):
        checks.append(
            {
                "check": str(check.get("check", "")),
                "profile": str(check.get("profile", "")),
                "fixture_id": str(check.get("fixture_id", "")),
                "status": str(check.get("status", "")).upper(),
            }
        )

    checks.sort(key=lambda item: (item["check"], item["profile"], item["fixture_id"], item["status"]))
    return {"totals": normalized_totals, "checks": checks}

host_payload = load(host_path, "host")
docker_payload = load(docker_path, "docker")

result = {
    "host_summary": host_path,
    "docker_summary": docker_path,
    "status": "FAIL",
    "mismatches": [],
}

if host_payload is not None and docker_payload is not None:
    host = normalize(host_payload)
    docker = normalize(docker_payload)

    if host["totals"] != docker["totals"]:
        mismatches.append(f"totals differ: host={host['totals']} docker={docker['totals']}")

    if host["checks"] != docker["checks"]:
        host_set = {json.dumps(item, sort_keys=True) for item in host["checks"]}
        docker_set = {json.dumps(item, sort_keys=True) for item in docker["checks"]}
        only_host = [json.loads(item) for item in sorted(host_set - docker_set)]
        only_docker = [json.loads(item) for item in sorted(docker_set - host_set)]
        if only_host:
            mismatches.append(f"check results only in host: {only_host}")
        if only_docker:
            mismatches.append(f"check results only in docker: {only_docker}")

    if not mismatches:
        result["status"] = "PASS"

result["mismatches"] = mismatches
os.makedirs(os.path.dirname(out_path), exist_ok=True)
with open(out_path, "w", encoding="utf-8") as handle:
    json.dump(result, handle, indent=2)

sys.exit(0 if result["status"] == "PASS" else 1)
PY
      then
        log_pass "parity matched for arch=$target_arch mode=$verify_mode"
      else
        log_fail "parity mismatch for arch=$target_arch mode=$verify_mode (see $compare_json)"
        mode_failed=1
      fi
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

if [[ "$MODE" == "verify-denulled" || "$MODE" == "verify-equivalence" || "$MODE" == "verify-parity" || "$MODE" == "release-gate" ]]; then
  echo ""
  echo "[1/4] Verification-mode prerequisites"
  if [[ "$MODE" != "verify-parity" && "$MODE" != "release-gate" ]]; then
    if verify_binary_available; then
      :
    else
      echo ""
      echo "Verification mode prerequisites failed -- cannot continue."
      exit 1
    fi
  fi

  if [[ "$MODE" == "verify-parity" || "$MODE" == "release-gate" ]]; then
    if verify_docker_compose_available; then
      :
    else
      echo ""
      echo "verify-parity prerequisites failed -- cannot continue."
      exit 1
    fi
  fi

  case "$MODE" in
    verify-denulled)
      run_verify_denulled_mode
      ;;
    verify-equivalence)
      run_verify_equivalence_mode
      ;;
    verify-parity)
      run_verify_parity_mode
      ;;
    release-gate)
      run_verify_parity_mode
      ;;
    *)
      log_fail "unexpected verification mode: $MODE"
      ;;
  esac

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
