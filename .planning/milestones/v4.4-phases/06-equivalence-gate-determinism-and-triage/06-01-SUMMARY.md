---
phase: 06-equivalence-gate-determinism-and-triage
plan: 01
subsystem: equivalence-determinism
tags: [equivalence, deterministic, manifest, tuples]
requires: []
provides:
  - Deterministic manifest-governed representative selection for verify-equivalence
  - Stable tuple schema output for equivalence summaries
  - Contributor guidance for deterministic reruns and representative governance
affects: [tests runner, fixture metadata policy, release-gate triage artifacts]
tech-stack:
  added: []
  patterns: [manifest-locked representative scope, tuple-first status reporting]
key-files:
  created: []
  modified:
    - tests/run_tests.sh
    - tests/fixtures/manifest.yaml
    - tests/README.md
key-decisions:
  - "Treat representative metadata completeness as mandatory for deterministic scope."
  - "Normalize equivalence summaries around tuple fields used by release triage."
patterns-established:
  - "Representative fixture ordering is stable by fixture_id/path."
  - "Summary tuple contract is stable across reruns for equivalent inputs."
requirements-completed: [REL-04]
duration: 18 min
completed: 2026-02-26
---

# Phase 6 Plan 01: Deterministic Equivalence Foundation Summary

**Representative verify-equivalence execution is now manifest-locked and emits a stable tuple schema for deterministic triage.**

## Performance

- **Duration:** 18 min
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments
- Hardened representative selection with strict metadata validation, duplicate detection, and stable ordering.
- Added canonical tuple schema (`arch`, `fixture_id`, `check`, `status`, `message`, `log_path`) in summary JSON output.
- Documented deterministic `verify-equivalence` rerun behavior and manifest governance constraints.

## Task Commits

1. **Task 1: Normalize representative fixture selection and ordering**
   - `d65454e` feat(06-01): enforce deterministic representative fixture selection
2. **Task 2: Normalize equivalence tuple status semantics**
   - `810e9c5` feat(06-01): stabilize verify-equivalence tuple schema
3. **Task 3: Document deterministic equivalence scope and rerun expectations**
   - `ee5b59a` docs(06-01): document deterministic equivalence scope

## Files Created/Modified
- `tests/run_tests.sh` - Enforces representative metadata integrity and emits sorted tuple schema output.
- `tests/fixtures/manifest.yaml` - Declares representative metadata fields as required by schema.
- `tests/README.md` - Documents deterministic representative contract and tuple-field expectations.

## Validation Evidence
- `bash tests/run_tests.sh --mode verify-equivalence --arch all --artifacts-dir /opt/byvalver/ci-artifacts/phase6-ordering`
- `bash tests/run_tests.sh --mode verify-equivalence --arch x64 --artifacts-dir /opt/byvalver/ci-artifacts/phase6-tuples`
- Summary JSON now includes:
  - `schema_version: 2`
  - `tuple_fields: ["arch","fixture_id","check","status","message","log_path"]`
  - deterministic `tuples` ordering

## Deviations from Plan

None - plan intent implemented as scoped.

## Issues Encountered
- Existing functionality verification failures remain for representative fixtures; deterministic triage output now isolates failing tuples for follow-on remediation in later wave plans.

## Next Phase Readiness
- Wave 2 can consume the stabilized tuple schema to publish actionable CI release summaries and rerun metadata.

## Self-Check: PASSED

---
*Phase: 06-equivalence-gate-determinism-and-triage*
*Completed: 2026-02-26*
