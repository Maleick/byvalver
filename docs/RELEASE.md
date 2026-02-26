# Release Gate Checklist

This checklist defines the mandatory pre-tag gate for reproducible releases.

## Canonical Local Gate

Run this command from repository root:

```bash
make release-gate
```

For full logs:

```bash
VERBOSE=1 make release-gate
```

## Required Gate Conditions

A release is eligible only when all are true:

1. `verify-parity` runs end-to-end for `x86`, `x64`, and `arm`.
2. Host and Docker verification outcomes match for:
   - `verify-denulled`
   - `verify-equivalence`
3. No required verification failures remain.

Gate policy is fail-closed. Any mismatch or verification failure blocks release.

## Artifact Review

`make release-gate` writes evidence to:

- `ci-artifacts/release-gate/parity-host/`
- `ci-artifacts/release-gate/parity-docker/`
- `ci-artifacts/release-gate/parity-compare/`

Review `parity-compare/summary-<arch>-<mode>-parity.json` first. If status is `FAIL`,
inspect the referenced host/docker logs for the failing check group.

## Tuple-Based Triage Workflow

Release-gate artifacts are tuple-oriented. Use failed required tuples as the
single source of truth for diagnosis.

Tuple fields:
- `arch`
- `fixture_id`
- `check`
- `status`
- `message`
- `log_path`
- `failure_class`
- `rerun_command`

Triage sequence:
1. Open `ci-artifacts/release-gate/parity-compare/summary-<arch>-<mode>-parity.json`.
2. Read `failed_required_tuples` and pick the first tuple for the target architecture.
3. Run the tuple `rerun_command` exactly as emitted.
4. Open the tuple `log_path` from host/docker artifacts and validate the same failure class.
5. Re-run `make release-gate` after remediation.

### Failure Class -> Rerun Matrix

| failure_class | Primary meaning | Canonical rerun command |
|---|---|---|
| `fixture-selection-failure` | Representative scope/metadata invalid | `bash tests/run_tests.sh --mode verify-equivalence --arch <arch> --artifacts-dir ci-artifacts/rerun-<arch>-verify-equivalence` |
| `transformation-failure` | byvalver transform failed before verification | `bash tests/run_tests.sh --mode verify-equivalence --arch <arch> --artifacts-dir ci-artifacts/rerun-<arch>-verify-equivalence` |
| `functionality-verification-failure` | functionality check failed for representative tuple | `bash tests/run_tests.sh --mode verify-equivalence --arch <arch> --artifacts-dir ci-artifacts/rerun-<arch>-verify-equivalence` |
| `semantic-verification-failure` | semantic check failed for representative tuple | `bash tests/run_tests.sh --mode verify-equivalence --arch <arch> --artifacts-dir ci-artifacts/rerun-<arch>-verify-equivalence` |
| `bad-byte-verification-failure` | profile bad-byte check failed | `bash tests/run_tests.sh --mode verify-denulled --arch <arch> --artifacts-dir ci-artifacts/rerun-<arch>-verify-denulled` |
| `profile-selection-failure` | required bad-byte profile coverage missing | `bash tests/run_tests.sh --mode verify-denulled --arch <arch> --artifacts-dir ci-artifacts/rerun-<arch>-verify-denulled` |

## CI Gate Enforcement

The `Release Gate` workflow enforces this policy for:

- Version tag pushes (`v*`)
- Manual dry-runs (`workflow_dispatch`)

Do not publish or keep a release tag if the release-gate workflow is red.

## Tagging Flow

1. Ensure local gate passes: `make release-gate`.
2. Trigger `Release Gate` workflow manually (`workflow_dispatch`) if needed.
3. Create and push version tag only after green gate evidence.
4. Verify tag-triggered `Release Gate` workflow is successful.

## Triage Routing

- Host vs Docker parity mismatch: inspect parity compare JSON and corresponding
  `parity-host` / `parity-docker` summaries plus tuple-level mismatch rows.
- Verification check failure (bad-byte/equivalence): treat as release blocker;
  resolve the specific failing tuple path before retagging.
