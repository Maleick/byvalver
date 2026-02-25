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
```

The canonical contributor baseline runbook is documented in
`docs/CONTRIBUTOR_BASELINE.md`.

## Test Categories

### Build Verification
- Compiles the project with `make clean && make`
- Verifies the executable exists and runs

### Transformation Verification
- Processes test fixtures through byvalver
- Checks output contains no null bytes (via `verify_denulled.py`)
- Validates functional equivalence (via `verify_functionality.py`)

### Batch Processing
- Runs batch mode on the fixture corpus
- Verifies summary statistics

## Adding Test Fixtures

Place binary test files in the appropriate architecture subdirectory under
`tests/fixtures/` and add corresponding metadata to `tests/fixtures/manifest.yaml`.
Small, representative samples are preferred to keep the test suite fast and
deterministic.

See `tests/fixtures/README.md` for full fixture taxonomy and governance details.
