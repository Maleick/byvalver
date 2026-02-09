# byvalver Test Suite

## Overview

This directory contains test infrastructure for verifying byvalver builds and transformations.

## Directory Structure

```
tests/
  run_tests.sh          -- Master test runner
  fixtures/
    x86/                -- x86 test binaries (copy from assets/tests/)
    x64/                -- x64 test binaries
    arm/                -- ARM test binaries
```

## Running Tests

```bash
# Run the full test suite
bash tests/run_tests.sh

# Run individual verification scripts from the project root
python3 verify_denulled.py output.bin
python3 verify_functionality.py input.bin output.bin
python3 verify_semantic.py input.bin output.bin
```

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

Place binary test files in the appropriate architecture subdirectory under `fixtures/`.
Small, representative samples are preferred to keep the test suite fast.
