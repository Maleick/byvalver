# byvalver Test Suite

## Overview

This directory contains test infrastructure for verifying byvalver builds and transformations.

## Directory Structure

```
tests/
  run_tests.sh          -- Master test runner
  fixtures/
    README.md           -- Canonical fixture taxonomy and policy
    manifest.yaml       -- Fixture metadata (expected outcomes + ownership)
    x86/                -- x86 curated fixture binaries
    x64/                -- x64 curated fixture binaries
    arm/                -- ARM curated fixture binaries
```

The canonical fixture catalog lives under `tests/fixtures/` and is architecture
first. Expected outcomes and ownership are tracked in `tests/fixtures/manifest.yaml`.
Canonical architecture roots are `tests/fixtures/x86`, `tests/fixtures/x64`, and
`tests/fixtures/arm`.

## Running Tests

```bash
# Canonical CI-parity local baseline path
make ci-baseline

# Verbose CI-parity baseline output
VERBOSE=1 make ci-baseline

# Run the full test suite directly
bash tests/run_tests.sh

# Run individual verification scripts from the project root
python3 verify_denulled.py output.bin
python3 verify_functionality.py input.bin output.bin
python3 verify_semantic.py input.bin output.bin

# Verify host-vs-Docker reproducibility parity (representative fixtures)
bash tests/run_tests.sh --mode verify-parity --arch all --artifacts-dir ci-artifacts
```

The canonical contributor baseline runbook is documented in
`docs/CONTRIBUTOR_BASELINE.md`.

## Deterministic `verify-equivalence` Contract

`verify-equivalence` in Phase 6 is intentionally deterministic at representative scope.
Runner behavior is fixed so the same inputs yield the same tuple outcomes.

Deterministic scope rules:
- Fixture scope is manifest-governed only: `tests/fixtures/manifest.yaml`.
- A fixture is eligible only when `ci_representative: true` is explicitly set.
- Representative entries must include required metadata (`fixture_id`, `arch`, `path`,
  `expected_outcome`, `owner`, `notes`, `ci_representative`, `profiles`).
- Representatives are executed in stable sorted order (`fixture_id`, then `path`).

Deterministic rerun command:

```bash
bash tests/run_tests.sh --mode verify-equivalence --arch <x86|x64|arm> --artifacts-dir ci-artifacts/<run-id>
```

Machine-readable tuple contract (per summary JSON):
- `arch`
- `fixture_id`
- `check`
- `status`
- `message`
- `log_path`

The tuple contract is emitted in `summary-<arch>-verify-equivalence.json` under
`tuple_fields` and `tuples`. This schema is used for release-gate triage and
must remain stable across equivalent reruns.

## Verification Artifacts (CI Triage)

Phase 2 CI verification publishes per-architecture artifacts with stable names so
failures can be diagnosed without rerunning locally first.

- Artifact bundle name: `verification-<arch>` (for example `verification-x86`)
- Bad-byte logs: `ci-artifacts/verify-<arch>-denulled-<profile>-<fixture_id>.log`
- Functionality logs: `ci-artifacts/verify-<arch>-functionality-<fixture_id>.log`
- Semantic logs: `ci-artifacts/verify-<arch>-semantic-<fixture_id>.log`
- Bad-byte summary JSON: `ci-artifacts/summary-<arch>-verify-denulled.json`
- Equivalence summary JSON: `ci-artifacts/summary-<arch>-verify-equivalence.json`

Per-architecture step summaries in GitHub Actions report both verification groups:
- `bad-byte profiles` (`null-only`, `http-newline` where applicable)
- `functionality + semantic`

Suggested triage order:
1. Check `summary-<arch>-verify-denulled.json` and `summary-<arch>-verify-equivalence.json` for failing check groups.
2. Open the corresponding raw log (`verify-...log`) for the failed fixture/check pair.
3. Reproduce locally with `bash tests/run_tests.sh --mode verify-denulled|verify-equivalence --arch <arch>`.

Phase 5 adds host-vs-Docker parity artifact groups:
- Host summaries/logs: `ci-artifacts/parity-host/`
- Docker summaries/logs: `ci-artifacts/parity-docker/`
- Parity comparisons: `ci-artifacts/parity-compare/summary-<arch>-<mode>-parity.json`

`verify-parity` exits non-zero when:
- any required verification group fails in host or Docker runs, or
- parity comparisons detect host-vs-Docker mismatches.

## Architecture Diagnostics (Phase 4)

When `byvalver` emits a pre-transform architecture mismatch warning, treat it as
an actionable signal, not a hard failure. Default behavior is warn-and-continue.

Interpretation:
- Warning indicates decode coverage looked significantly better for another architecture.
- Processing still ran with the selected `--arch`; output must be validated before use.

Recommended follow-up:
1. Re-run preflight without mutation:
   `./bin/byvalver --arch <suggested-arch> --dry-run <input.bin>`
2. Re-run transformation with explicit architecture selection:
   `./bin/byvalver --arch <suggested-arch> <input.bin> <output.bin>`
3. Validate output artifacts:
   `python3 verify_denulled.py <output.bin>`
   `python3 verify_functionality.py <input.bin> <output.bin>`

## ARM Conditional Scope (Phase 3)

Phase 3 ARM conditional rewrites are intentionally limited to branch-first alternatives.
Covered scope is conditional `B<cond>` rewrite variants that preserve target and fall-through semantics.

Deferred scope in this phase:
- Predicated ALU conditional rewrites.
- Predicated memory conditional rewrites.
- Broader non-branch conditional families beyond branch-first alternatives.

## Test Categories

### Build Verification
- Compiles the project with `make clean && make`
- Verifies the executable exists and runs

### Transformation Verification
- Processes test fixtures through byvalver
- Checks bad-byte profile compliance (via `verify_denulled.py`)
- Validates functional equivalence (via `verify_functionality.py`)
- Validates semantic preservation (via `verify_semantic.py`)

### Batch Processing
- Runs batch mode on the fixture corpus
- Verifies summary statistics

## Adding Test Fixtures

Place binary test files in the appropriate architecture subdirectory under
`tests/fixtures/` and add corresponding metadata to `tests/fixtures/manifest.yaml`.
Small, representative samples are preferred to keep the test suite fast and
deterministic.

See `tests/fixtures/README.md` for full fixture taxonomy and governance details.
