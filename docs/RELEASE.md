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
  `parity-host` / `parity-docker` summaries.
- Verification check failure (bad-byte/equivalence): treat as release blocker;
  resolve transformation or fixture correctness before retagging.
