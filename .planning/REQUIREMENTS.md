# Requirements: byvalver

**Defined:** 2026-02-26
**Core Value:** Given an input payload and bad-byte policy, byvalver must produce functionally equivalent shellcode that is verifiably free of forbidden bytes.

## v1 Requirements

Requirements for milestone v4.4 Equivalence Reliability.

### Release Reliability

- [ ] **REL-04**: Release gate produces deterministic `verify-equivalence` outcomes for representative x86/x64/arm fixtures across repeated runs.
- [ ] **REL-05**: Release-gate artifacts identify failing architecture, fixture, check type, and include deterministic local rerun commands.
- [ ] **REL-06**: Local `make release-gate` and CI `release-gate` enforce the same required checks and status logic for representative fixtures.

### ARM64 Strategy Expansion

- [ ] **ARM64-01**: ARM64 arithmetic rewrite paths (core immediate families) avoid forbidden bytes while preserving semantic behavior.
- [ ] **ARM64-02**: ARM64 load/store displacement rewrite paths preserve addressing behavior under bad-byte constraints.
- [ ] **ARM64-03**: Deterministic ARM64 representative fixtures are added to `tests/fixtures/manifest.yaml` and executed in verification workflows.

### Architecture Assist

- [ ] **ARCH-01**: Architecture mismatch diagnostics emit scored recommendations with explicit suggested `--arch` fallback commands.
- [ ] **ARCH-02**: Non-destructive architecture assist flow (`--dry-run` + guidance) is documented and validated for operator triage.
- [ ] **ARCH-03**: Verification artifacts capture architecture-assist context for failures so wrong-arch root causes are quickly distinguishable.

## v2 Requirements

Deferred to later milestones.

### Future Expansion

- **ARM64-04**: ARM64 branch/conditional rewrite families expand beyond core arithmetic/load-store coverage.
- **ARCH-04**: Optional guided architecture auto-selection mode is introduced with explicit user confirmation.
- **ML-01**: ML model retraining supports multi-byte bad-byte elimination scenarios.
- **EXT-01**: Plugin architecture supports third-party strategy packs.

## Out of Scope

Explicitly excluded from v4.4 scope.

| Feature | Reason |
|---------|--------|
| Full corpus release-gate checks on every tag | Representative deterministic gates preserve reliability without unacceptable runtime cost |
| Silent architecture auto-switch during mutation | Unsafe without operator confirmation and can hide root-cause errors |
| Complete ARM64 parity with x86/x64 strategy count | Too broad for a single reliability-focused milestone |
| TUI redesign or UX overhaul | Milestone focus is gate reliability and architecture correctness |

## Traceability

Which phases cover which requirements.

| Requirement | Phase | Status |
|-------------|-------|--------|
| REL-04 | Phase 6 | Pending |
| REL-05 | Phase 6 | Pending |
| REL-06 | Phase 6 | Pending |
| ARM64-01 | Phase 7 | Pending |
| ARM64-02 | Phase 7 | Pending |
| ARM64-03 | Phase 7 | Pending |
| ARCH-01 | Phase 8 | Pending |
| ARCH-02 | Phase 8 | Pending |
| ARCH-03 | Phase 8 | Pending |

**Coverage:**
- v1 requirements: 9 total
- Mapped to phases: 9
- Unmapped: 0 ✓

---
*Requirements defined: 2026-02-26*
*Last updated: 2026-02-26 after v4.4 milestone requirements definition*
