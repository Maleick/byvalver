---
phase: 06-equivalence-gate-determinism-and-triage
plan: 02
subsystem: release-gate-triage
tags: [release-gate, tuples, rerun, parity, diagnostics]
requires:
  - phase: 06-equivalence-gate-determinism-and-triage
    plan: 01
    provides: Deterministic representative tuple schema
provides:
  - Failed required tuples include deterministic rerun metadata
  - Release workflow summary renders tuple-level diagnostics and artifact pointers
  - Release runbook maps failure classes to canonical rerun commands
affects: [tests runner summaries, release workflow summary output, release triage docs]
tech-stack:
  added: []
  patterns: [tuple-oriented failure triage, artifact-first rerun routing]
key-files:
  created: []
  modified:
    - tests/run_tests.sh
    - .github/workflows/release-gate.yml
    - docs/RELEASE.md
key-decisions:
  - "Emit failed required tuples from runner summaries with stable failure classification and rerun command fields."
  - "Render tuple diagnostics directly in GitHub step summaries to remove log-archeology dependency."
patterns-established:
  - "Release triage starts from `failed_required_tuples` and follows emitted rerun command metadata."
requirements-completed: [REL-05]
duration: 19 min
completed: 2026-02-26
---

# Phase 6 Plan 02: Actionable Release Diagnostics Summary

**Release-gate artifacts now expose failing tuples with deterministic rerun metadata and CI summary rendering for immediate triage.**

## Performance

- **Duration:** 19 min
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments
- Extended runner summary payloads with `failed_required_tuples`, `failure_class`, and deterministic `rerun_command`.
- Updated release workflow step summary to display tuple rows, mismatch details, and artifact pointers.
- Added tuple-based triage runbook and rerun matrix to release documentation.

## Task Commits

1. **Task 1: Extend runner summaries with rerun metadata for failing tuples**
   - `3c5bc61` feat(06-02): add tuple rerun metadata to parity summaries
2. **Task 2: Surface tuple failures in release workflow step summary**
   - `fc410e4` feat(06-02): surface tuple diagnostics in release gate summary
3. **Task 3: Document tuple-based triage workflow and rerun matrix**
   - `59799d6` docs(06-02): add tuple-based release triage runbook

## Files Created/Modified
- `tests/run_tests.sh` - Adds failure classification, rerun command metadata, and parity-compare tuple aggregation.
- `.github/workflows/release-gate.yml` - Publishes tuple-level diagnostics and artifact pointers in step summary output.
- `docs/RELEASE.md` - Documents tuple contract and failure-class rerun routing.

## Validation Evidence
- `bash tests/run_tests.sh --mode verify-parity --arch all --artifacts-dir /opt/byvalver/ci-artifacts/phase6-rerun-meta`
- `rg -n "Release Gate Parity Summary|tuple|artifact|mismatch|workflow_dispatch" .github/workflows/release-gate.yml`
- `rg -n "tuple|rerun|release-gate|artifact|verify-equivalence" docs/RELEASE.md`
- Parity compare JSON now includes `failed_required_tuples[*].rerun_command` and `failure_class`.

## Deviations from Plan

None - plan intent implemented as scoped.

## Issues Encountered
- Representative `verify-equivalence` functionality failures remain for all architectures; tuple diagnostics now expose deterministic rerun paths to resolve these blockers in later wave work.

## Next Phase Readiness
- Wave 3 can focus on harness-level instability remediation and required-check parity finalization using tuple-level diagnostics already in place.

## Self-Check: PASSED

---
*Phase: 06-equivalence-gate-determinism-and-triage*
*Completed: 2026-02-26*
