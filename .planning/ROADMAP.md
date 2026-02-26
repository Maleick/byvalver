# Roadmap: byvalver

## Milestones

- ✅ **v4.3 stabilization** - Phases 1-5 (shipped 2026-02-25) ([archive](milestones/v4.3-ROADMAP.md))
- 🚧 **v4.4 Equivalence Reliability** - Phases 6-8 (in progress)

## Phases

<details>
<summary>✅ v4.3 stabilization (Phases 1-5) - SHIPPED 2026-02-25</summary>

- [x] Phase 1: CI Foundation and Fixture Canonicalization (3/3 plans)
- [x] Phase 2: Verification Automation and Reporting (3/3 plans)
- [x] Phase 3: ARM Strategy Expansion (3/3 plans)
- [x] Phase 4: ARM Diagnostic Safety Nets (2/2 plans)
- [x] Phase 5: Reproducible Release Gate (2/2 plans)

</details>

### 🚧 v4.4 Equivalence Reliability (In Progress)

**Milestone Goal:** Restore deterministic release-gate confidence, expand practical ARM64 coverage, and reduce architecture-selection error loops.

#### Phase 6: Equivalence Gate Determinism and Triage
**Goal**: Make representative equivalence checks deterministic and release-gate failures immediately actionable.
**Depends on**: Phase 5
**Requirements**: REL-04, REL-05, REL-06
**Success Criteria** (what must be TRUE):
  1. Representative `verify-equivalence` outcomes are deterministic across repeated local and CI runs.
  2. Release-gate artifacts identify failing architecture, fixture, and check with deterministic rerun commands.
  3. Local `make release-gate` and CI release workflow enforce the same required-check pass/fail logic.
**Plans**: 3 plans

Plans:
- [ ] 06-01: Stabilize representative equivalence harness inputs, ordering, and status semantics.
- [ ] 06-02: Standardize release-gate summary schema and deterministic rerun guidance outputs.
- [ ] 06-03: Resolve current representative equivalence blockers and lock parity behavior across host and Docker.

#### Phase 7: ARM64 Core Strategy Expansion
**Goal**: Expand ARM64 strategy coverage for core arithmetic/load-store families with deterministic regression evidence.
**Depends on**: Phase 6
**Requirements**: ARM64-01, ARM64-02, ARM64-03
**Success Criteria** (what must be TRUE):
  1. ARM64 arithmetic rewrite paths handle representative bad-byte cases while preserving semantic checks.
  2. ARM64 load/store displacement rewrite paths preserve addressing behavior in deterministic verification runs.
  3. ARM64 representative fixtures are integrated into manifest-driven verification and reported by architecture.
**Plans**: 3 plans

Plans:
- [ ] 07-01: Implement bounded ARM64 arithmetic rewrite families with safe-decline guardrails.
- [ ] 07-02: Implement ARM64 load/store displacement rewrites with deterministic regression fixtures.
- [ ] 07-03: Integrate ARM64 representative verification wiring and artifact reporting in runner/CI flows.

#### Phase 8: Architecture Assist and Operator Guidance
**Goal**: Reduce wrong-architecture mutation attempts with clear scored recommendations and triage context.
**Depends on**: Phase 7
**Requirements**: ARCH-01, ARCH-02, ARCH-03
**Success Criteria** (what must be TRUE):
  1. Mismatch diagnostics provide scored architecture recommendations and explicit fallback commands.
  2. Non-destructive assist flow (`--dry-run` + docs) is clear and validated for contributor triage.
  3. Verification artifacts include architecture-assist context to separate mismatch issues from transformation regressions.
**Plans**: 2 plans

Plans:
- [ ] 08-01: Implement scored architecture-assist diagnostics and deterministic fallback messaging.
- [ ] 08-02: Integrate assist context into verification artifacts and contributor/operator guidance docs.

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 1. CI Foundation and Fixture Canonicalization | v4.3 | 3/3 | Complete | 2026-02-25 |
| 2. Verification Automation and Reporting | v4.3 | 3/3 | Complete | 2026-02-25 |
| 3. ARM Strategy Expansion | v4.3 | 3/3 | Complete | 2026-02-25 |
| 4. ARM Diagnostic Safety Nets | v4.3 | 2/2 | Complete | 2026-02-25 |
| 5. Reproducible Release Gate | v4.3 | 2/2 | Complete | 2026-02-25 |
| 6. Equivalence Gate Determinism and Triage | v4.4 | Complete    | 2026-02-26 | - |
| 7. ARM64 Core Strategy Expansion | v4.4 | 0/3 | Not started | - |
| 8. Architecture Assist and Operator Guidance | v4.4 | 0/2 | Not started | - |
