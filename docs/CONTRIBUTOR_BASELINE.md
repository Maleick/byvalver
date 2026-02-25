# Contributor Baseline (CI Parity)

This runbook defines the single local command path that mirrors the repository's
baseline CI checks.

## Canonical Command

```bash
make ci-baseline
```

For troubleshooting with full command output:

```bash
VERBOSE=1 make ci-baseline
```

## What This Runs

`make ci-baseline` executes the same gate sequence used by the baseline CI flow:

1. `make check-deps` (toolchain and Capstone preflight checks)
2. `make` (clean project build)
3. `bash tests/run_tests.sh --mode baseline --arch all` (architecture baseline checks)

The test runner defaults to concise output and exits non-zero on any baseline
gate failure.

## CI Parity Expectations

- Baseline mode requires canonical fixture roots under `tests/fixtures/{x86,x64,arm}`.
- Baseline mode fails if expected architecture fixture directories are missing or
  contain no `.bin` fixture files.
- The local baseline command intentionally mirrors CI pass/fail semantics.

## Troubleshooting Flow

1. Run `VERBOSE=1 make ci-baseline` to see full command output.
2. If preflight fails, install the missing dependency using the hint printed by
   `tests/run_tests.sh`.
3. If fixture checks fail, verify fixture placement and metadata:
   - `tests/fixtures/README.md`
   - `tests/fixtures/manifest.yaml`
4. Re-run `make ci-baseline` after correcting issues.

