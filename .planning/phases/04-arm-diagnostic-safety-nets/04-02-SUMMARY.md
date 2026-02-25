---
phase: 04-arm-diagnostic-safety-nets
plan: 02
subsystem: diagnostics
tags: [architecture, warning, arm, cli, docs, fallback]
requires:
  - phase: 04-arm-diagnostic-safety-nets
    provides: Branch safety hardening and deterministic ARM edge coverage from 04-01
provides:
  - Pre-transform architecture mismatch heuristic warnings with actionable next steps
  - Runtime and CLI-aligned ARM experimental guidance under warn-and-continue policy
  - Deterministic mismatch warning threshold validation and contributor triage notes
affects: [operator diagnostics, cli help, usage docs, contributor test triage]
tech-stack:
  added: []
  patterns: [decode-coverage mismatch warning heuristic, warn-and-continue diagnostics]
key-files:
  created: []
  modified:
    - src/core.c
    - src/main.c
    - src/cli.c
    - docs/USAGE.md
    - assets/tests/test_cross_arch.c
    - tests/README.md
key-decisions:
  - "Keep mismatch handling warn-and-continue with explicit fallback commands instead of fail-closed behavior."
  - "Use conservative decode-coverage thresholds to reduce false-positive architecture mismatch warnings."
patterns-established:
  - "Architecture mismatch diagnostics must be emitted before destructive processing."
  - "ARM experimental messaging must include --dry-run and explicit --arch fallback guidance."
requirements-completed: [ARM-05, ARM-06]
duration: 2 min
completed: 2026-02-25
---

# Phase 4 Plan 02: ARM Diagnostic Guidance Summary

**Pre-transform architecture mismatch heuristics now emit actionable warn-and-continue diagnostics, with aligned ARM experimental fallback guidance across runtime, CLI, and contributor docs.**

## Performance

- **Duration:** 2 min
- **Started:** 2026-02-25T21:33:39Z
- **Completed:** 2026-02-25T21:35:24Z
- **Tasks:** 3
- **Files modified:** 6

## Accomplishments
- Added conservative decode-coverage mismatch heuristics that warn before mutation and preserve processing continuity.
- Unified ARM experimental warnings and fallback instructions in runtime output, CLI help, and usage documentation.
- Added deterministic validation tests for warning threshold behavior and contributor triage guidance.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add pre-transform architecture mismatch heuristic warnings** - `e04efc6` (feat)
2. **Task 2: Align ARM experimental guidance across help and runtime diagnostics** - `8af0128` (docs)
3. **Task 3: Add deterministic diagnostic validation tests and contributor triage notes** - `b1457f9` (test)

## Files Created/Modified
- `src/core.c` - Adds decode-coverage mismatch heuristic scoring and architecture suggestion logic.
- `src/main.c` - Emits pre-transform mismatch warnings and ARM experimental runtime guidance with explicit fallback steps.
- `src/cli.c` - Updates help text with warn-and-continue policy and verification/fallback workflow.
- `docs/USAGE.md` - Adds Phase 4 ARM diagnostic and fallback operating guidance.
- `assets/tests/test_cross_arch.c` - Adds deterministic mismatch warning threshold coverage.
- `tests/README.md` - Documents contributor triage flow for architecture diagnostics.

## Decisions Made
- Kept diagnostics non-blocking by default per locked Phase 4 warn-and-continue policy.
- Chose conservative heuristic thresholds (`coverage >= 55%`, delta `>= 30%`, min instruction signal) to avoid noisy warnings.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Both Phase 4 plans now include deterministic summary evidence and complete requirement coverage for diagnostics behavior.
- Ready for phase-level verifier gate and completion updates.

## Self-Check: PASSED

---
*Phase: 04-arm-diagnostic-safety-nets*
*Completed: 2026-02-25*
