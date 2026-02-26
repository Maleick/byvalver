---
phase: 05-reproducible-release-gate
plan: 01
subsystem: reproducibility-parity
tags: [release-gate, parity, docker, verification]
provides:
  - Deterministic host-vs-Docker verification parity mode in test runner
  - Mount-free Docker parity execution surface for local and CI parity checks
  - Contributor-facing parity command and artifact triage documentation
affects: [test runner, docker workflow, build docs, test docs]
tech-stack:
  added: []
  patterns: [deterministic manifest representative parity, fail-closed parity gating]
key-files:
  created: []
  modified:
    - tests/run_tests.sh
    - Dockerfile
    - docker-compose.yml
    - docs/BUILD.md
    - tests/README.md
key-decisions:
  - "Implement verify-parity as host and container verification comparison on summary JSON outcomes."
  - "Use mount-free Docker compose services so /opt workspace constraints do not break parity execution."
  - "Treat parity mismatches and required verification failures as non-zero outcomes."
patterns-established:
  - "Release reproducibility evidence is grouped under parity-host, parity-docker, and parity-compare artifacts."
requirements-completed: [REL-01]
duration: 15 min
completed: 2026-02-25
---

# Phase 5 Plan 01: Reproducibility Parity Summary

**Phase 5 now includes a deterministic `verify-parity` runner path comparing host and Docker verification outcomes per architecture and verification mode.**

## Performance

- **Duration:** 15 min
- **Tasks:** 3
- **Files modified:** 5

## Accomplishments
- Added `--mode verify-parity` in `tests/run_tests.sh` with host-vs-Docker summary comparison and parity JSON outputs.
- Hardened verification runner portability (Bash 3 compatibility and robust embedded-summary extraction from container logs).
- Updated Docker surface for parity execution without host bind mounts and documented parity invocation/triage flow.

## Task Commits

1. **Task 1: Add deterministic host-vs-Docker parity mode to test runner**
   - `564ce41` feat(05-01): add host-vs-docker verify-parity mode
   - `71b98d4` fix(05-01): harden verify-parity execution and parsing
2. **Task 2: Align Docker execution surface with parity harness requirements**
   - `92b1335` feat(05-01): align docker parity execution surfaces
3. **Task 3: Document parity command path and artifact triage contract**
   - `558975f` docs(05-01): document parity command and triage artifacts

## Files Created/Modified
- `tests/run_tests.sh` - Added parity mode, docker preflight, per-arch comparison, and compatibility fixes.
- `Dockerfile` - Added deterministic clean/release build inside image and full source copy for parity runtime.
- `docker-compose.yml` - Added `parity` service and updated `verify` service to avoid mount-dependent execution.
- `docs/BUILD.md` - Added parity command and parity artifact interpretation guidance.
- `tests/README.md` - Added parity usage and artifact triage documentation.

## Validation Evidence
- `docker compose run --rm verify python3 verify_denulled.py --help` passes with updated compose settings.
- `bash tests/run_tests.sh --mode verify-parity --arch all --artifacts-dir ci-artifacts` now runs end-to-end and emits parity comparisons.
- Current representative `verify-equivalence` checks fail on both host and Docker for x86/x64/arm; parity comparison still matches.

## Deviations from Plan
- Task 1 required an additional hardening commit for Bash 3 compatibility (`mapfile` replacement) and log-embedded JSON extraction reliability.
- Docker parity implementation uses mount-free image execution due local Docker bind-mount constraints on `/opt` paths.

## Issues Encountered
- Docker Desktop path sharing blocked bind mounts from `/opt/byvalver`; resolved by removing bind-mount dependency from parity/verify flows.

## Next Phase Readiness
- `05-02` can now wire release-gate workflow and local `make release-gate` command against stable parity artifacts and non-zero failure semantics.

## Self-Check: PASSED

---
*Phase: 05-reproducible-release-gate*
*Completed: 2026-02-25*
