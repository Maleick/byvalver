---
phase: 02-verification-automation-and-reporting
plan: "01"
subsystem: testing
tags: [ci, github-actions, verification, fixtures, bad-bytes]
requires:
  - phase: 01-ci-foundation-and-fixture-canonicalization
    provides: baseline ci matrix and canonical fixture metadata path
provides:
  - profile-aware bad-byte verification runner mode
  - deterministic representative fixture selection from manifest metadata
  - per-architecture machine-readable verification evidence artifacts
affects: [phase-02-plan-02, phase-02-plan-03, ci-reporting]
tech-stack:
  added: [none]
  patterns: [manifest-driven fixture selection, explicit arch-profile mapping, fail-late artifact capture]
key-files:
  created: []
  modified: [tests/run_tests.sh, tests/fixtures/manifest.yaml, .github/workflows/ci.yml]
key-decisions:
  - "Use tests/fixtures/manifest.yaml as the deterministic source for representative fixture selection."
  - "Lock profile mapping to null-only on arm and null-only,http-newline on x86/x64."
  - "Emit summary-<arch>-verify-denulled.json as machine-readable verification evidence."
patterns-established:
  - "Verification modes in tests/run_tests.sh produce per-arch logs and JSON summaries."
  - "CI matrix jobs invoke explicit profile checks and always upload evidence artifacts."
requirements-completed: [TEST-03]
duration: 2 min
completed: 2026-02-25
---

# Phase 02 Plan 01: Profile-Aware Bad-Byte Verification Summary

**Manifest-driven, profile-explicit bad-byte verification now runs per architecture with deterministic fixture selection and CI evidence artifacts.**

## Performance

- **Duration:** 2 min
- **Started:** 2026-02-25T20:05:02Z
- **Completed:** 2026-02-25T20:07:10Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments
- Added `verify-denulled` mode in `tests/run_tests.sh` with deterministic representative fixture selection from `tests/fixtures/manifest.yaml`.
- Populated representative fixture metadata for `x86`, `x64`, and `arm`, including explicit profile coverage.
- Wired CI matrix jobs to run profile-aware bad-byte checks and upload raw logs plus per-arch JSON summaries.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add deterministic representative fixture selection and profile mapping to runner** - `7f2f3e0` (feat)
2. **Task 2: Wire CI architecture jobs to bad-byte profile verification mode** - `2550484` (feat)
3. **Task 3: Emit profile-aware pass/fail evidence for downstream reporting** - `bb2ac34` (feat)

## Files Created/Modified
- `tests/run_tests.sh` - Added profile-aware verification mode, manifest-driven fixture selection, and JSON evidence output.
- `tests/fixtures/manifest.yaml` - Added representative fixture metadata and profile coverage declarations.
- `.github/workflows/ci.yml` - Added profile mapping step, denulled verification invocation, and always-on evidence artifact uploads.

## Decisions Made
- Chose metadata-driven representative fixture selection from `manifest.yaml` to keep fixture ordering deterministic and auditable.
- Used explicit architecture/profile policy in CI to avoid implicit defaults and profile drift.
- Standardized evidence path names so downstream reporting can aggregate per-arch outputs.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
- Existing local build issue (`bin/tui` object output directory pathing) was observed but left out of this plan because it is outside `02-01` file scope.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Ready for `02-02` runner/CI equivalence verification integration.
- Bad-byte verification artifacts and profile outputs are now available for later aggregate reporting.

## Self-Check: PASSED
- SUMMARY exists at expected path.
- `git log --grep="02-01"` returns task commits.
- No `## Self-Check: FAILED` marker.

---
*Phase: 02-verification-automation-and-reporting*
*Completed: 2026-02-25*
