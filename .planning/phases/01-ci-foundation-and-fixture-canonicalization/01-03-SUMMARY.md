---
phase: 01-ci-foundation-and-fixture-canonicalization
plan: "03"
subsystem: contributor-workflow
tags: [makefile, baseline, docs, parity, preflight]
requires:
  - phase: 01-02
    provides: CI baseline matrix and architecture-scoped runner behavior
provides:
  - Canonical `make ci-baseline` local baseline command
  - Preflight dependency hints with concise/verbose runner UX
  - Contributor baseline runbook and cross-doc command convergence
affects: [testing, ci, docs, contributor-workflow]
tech-stack:
  added: []
  patterns:
    - Single command local baseline parity path
    - Consistent baseline command surfaced across contributor docs
key-files:
  created:
    - docs/CONTRIBUTOR_BASELINE.md
  modified:
    - Makefile
    - tests/run_tests.sh
    - README.md
    - tests/README.md
    - docs/BUILD.md
key-decisions:
  - "Keep make ci-baseline as the only canonical local CI-parity entry point"
  - "Default baseline output to concise mode while preserving explicit verbose troubleshooting"
patterns-established:
  - "Contributor baseline workflow always runs dependency preflight, build, then baseline fixture checks"
requirements-completed: [REL-02, TEST-01]
duration: 1 min
completed: 2026-02-25
---

# Phase 1 Plan 03: Local Baseline Command and Contributor Runbook Summary

**Single-command local CI-parity baseline workflow with preflight guidance and converged docs**

## Performance

- **Duration:** 1 min
- **Started:** 2026-02-25T13:25:56-06:00
- **Completed:** 2026-02-25T13:34:13-06:00
- **Tasks:** 3
- **Files modified:** 6

## Accomplishments
- Added the canonical `ci-baseline` Make target to run dependency checks, build, and baseline fixture validation in one command.
- Updated the baseline runner UX to include preflight dependency hints while preserving concise default output and explicit verbose mode.
- Published a dedicated contributor runbook and aligned top-level docs to one canonical local baseline path.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add canonical local baseline Make target** - `8316c51` (feat)
2. **Task 2: Implement preflight hints and concise/verbose output behavior** - `5b69645` (feat)
3. **Task 3: Publish and link contributor baseline runbook** - `d6872b2` (docs)

## Files Created/Modified
- `Makefile` - Adds `ci-baseline` target with `check-deps`, build, and baseline runner chaining.
- `tests/run_tests.sh` - Adds dependency preflight checks with install hints and concise/verbose output controls.
- `docs/CONTRIBUTOR_BASELINE.md` - Defines canonical command path, CI parity expectations, and troubleshooting flow.
- `README.md` - Promotes `make ci-baseline` as canonical contributor baseline and links runbook.
- `tests/README.md` - Adds canonical baseline command examples and runbook link.
- `docs/BUILD.md` - Adds CI-parity baseline command section and runbook reference.

## Decisions Made
- Keep all contributor-facing baseline documentation converged on `make ci-baseline` to avoid split command paths.
- Preserve strict local/CI parity by routing local baseline mode through the same baseline gate semantics.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Phase 1 plan set is complete (`01-01`, `01-02`, `01-03` summaries present).
- Ready for phase verification against `TEST-01`, `TEST-02`, and `REL-02`.

---
*Phase: 01-ci-foundation-and-fixture-canonicalization*
*Completed: 2026-02-25*
