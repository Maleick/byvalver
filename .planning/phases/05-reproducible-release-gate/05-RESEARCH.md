# Phase 5: Reproducible Release Gate - Research

**Researched:** 2026-02-25  
**Phase:** 05-reproducible-release-gate  
**Requirements:** REL-01, REL-03

## Objective

Define a low-risk implementation plan for reproducible release gating that proves host-vs-Docker verification outcome parity and blocks unverifiable releases before tag publication.

## Current Baseline (Repository Findings)

- Baseline CI verification already exists in `.github/workflows/ci.yml`:
  - matrix architectures: `x86`, `x64`, `arm`
  - bad-byte verification (`--mode verify-denulled`)
  - equivalence verification (`--mode verify-equivalence`)
  - per-arch artifact upload and step summary.
- Local CI-parity baseline command exists: `make ci-baseline`.
- Deterministic representative fixture selection is implemented in `tests/run_tests.sh` using `tests/fixtures/manifest.yaml` (`ci_representative: true`).
- Docker assets exist but are not yet release-gate complete:
  - `Dockerfile` builds `byvalver` and includes verification scripts.
  - `docker-compose.yml` has dev/verify services but no host-vs-Docker parity contract.
- No dedicated release-gate workflow currently exists for tag enforcement and manual dry-run.

## Gap Analysis vs Phase 5 Requirements

### REL-01 (reproducible Docker workflow)

**Gap:** Docker exists, but there is no explicit parity harness proving required host-vs-Docker verification outcomes on deterministic fixture scope.

**Implication:** maintainers cannot consistently prove reproducibility before release.

### REL-03 (release checklist and enforced pre-tag gates)

**Gap:** CI baseline runs on push/PR only; there is no explicit release-gate contract tied to version tags and mirrored local preflight path.

**Implication:** tags can be created without release-specific parity evidence and checklist enforcement.

## Locked Constraints from Context

- Reproducibility means verification outcome parity, not byte-for-byte output identity.
- Parity scope uses deterministic manifest representatives by default.
- Release gate must run in CI and be mirrored locally.
- Gate trigger is version-tag push plus `workflow_dispatch`.
- Required parity mismatches are hard-fail release blockers.

## Recommended Implementation Shape

### 05-01: Reproducibility Parity Harness (REL-01)

- Extend `tests/run_tests.sh` with parity mode(s) that compare required verification outcomes from:
  - host execution
  - Docker execution
- Reuse manifest representative selection to keep runs deterministic and bounded.
- Produce deterministic parity artifacts (per-arch summaries and mismatch reports).
- Update Docker workflow docs and test docs for exact parity invocation and triage flow.

### 05-02: Release Checklist + Enforced Gates (REL-03)

- Add a dedicated release-gate workflow for:
  - version tag push (`v*`)
  - manual dry-run (`workflow_dispatch`)
- Gate workflow runs parity and required verification checks and fails closed on mismatch.
- Provide one canonical local release-gate command/script that mirrors CI checks.
- Add release checklist documentation referencing mandatory checks and artifact review.

## Artifact Contract Recommendation

- Keep architecture-specific output paths stable:
  - `ci-artifacts/summary-<arch>-verify-denulled.json`
  - `ci-artifacts/summary-<arch>-verify-equivalence.json`
  - parity summaries (host vs Docker) with stable names.
- Release gate should publish a concise summary pointing to per-arch artifacts and mismatch details.

## Risks and Mitigations

1. Host-vs-Docker environmental drift causes flaky parity.
- Mitigation: restrict parity decision to required verification outcomes, not binary identity.

2. Release gate introduces operational friction.
- Mitigation: mirrored local command and explicit failure diagnostics.

3. Over-broad fixture scope slows release checks.
- Mitigation: deterministic representative fixture subset as default policy.

4. Scope creep into signing/provenance systems.
- Mitigation: keep Phase 5 strictly on REL-01 and REL-03.

## Plan Decomposition Recommendation

- Wave 1: `05-01` (REL-01) parity harness and deterministic evidence contract.
- Wave 2: `05-02` (REL-03) release checklist and enforced CI/local gate.

This sequence establishes parity mechanics first, then binds release policy to verified parity output.

## Definition of Done (Phase Planning Inputs)

- `05-01` maps to `REL-01` with deterministic host-vs-Docker verification parity evidence.
- `05-02` maps to `REL-03` with explicit release-gate enforcement and documented local mirror path.
- Both plans include clear must-haves, artifacts, and key links for verifier consumption.

---

*Phase: 05-reproducible-release-gate*  
*Research completed: 2026-02-25*
