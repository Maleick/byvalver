# Roadmap: byvalver v4.3 Stabilization

## Overview

This roadmap hardens byvalver's existing transformation engine by making verification repeatable, expanding ARM rewrite maturity, and enforcing reproducible release gates. The sequence starts with fixture and CI foundations, then automates correctness checks, then closes ARM reliability gaps, and ends with release reproducibility.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

- [ ] **Phase 1: CI Foundation and Fixture Canonicalization** - Standardize fixtures and baseline CI entry points.
- [ ] **Phase 2: Verification Automation and Reporting** - Automate bad-byte and semantic checks in CI with clear artifacts.
- [ ] **Phase 3: ARM Strategy Expansion** - Implement missing high-impact ARM rewrite strategies.
- [ ] **Phase 4: ARM Diagnostic Safety Nets** - Improve branch correctness and architecture mismatch diagnostics.
- [ ] **Phase 5: Reproducible Release Gate** - Lock in Docker-based reproducibility and release verification checks.

## Phase Details

### Phase 1: CI Foundation and Fixture Canonicalization
**Goal**: Contributors can run the same fixture-driven build and smoke checks locally and in CI.
**Depends on**: Nothing (first phase)
**Requirements**: TEST-01, TEST-02, REL-02
**Success Criteria** (what must be TRUE):
  1. Fixture corpus is grouped by architecture with documented expectations and ownership.
  2. CI executes baseline build and smoke checks on every PR without manual setup steps.
  3. New contributors can run one documented command path to replicate the CI baseline locally.
**Plans**: 3 plans

Plans:
- [ ] 01-01: Define fixture taxonomy and normalize sample metadata/documentation.
- [ ] 01-02: Update CI workflow to run baseline build/smoke jobs using canonical fixture paths.
- [ ] 01-03: Publish contributor runbook for local build + baseline verification entry point.

### Phase 2: Verification Automation and Reporting
**Goal**: PRs provide objective evidence that transformations remain bad-byte-safe and semantically equivalent.
**Depends on**: Phase 1
**Requirements**: TEST-03, TEST-04, TEST-05
**Success Criteria** (what must be TRUE):
  1. CI runs bad-byte profile checks across representative fixture subsets.
  2. CI runs functionality and semantic verification scripts with deterministic pass/fail behavior.
  3. CI uploads per-architecture reports that identify failures without rerunning locally first.
**Plans**: 3 plans

Plans:
- [ ] 02-01: Integrate `verify_denulled.py` into matrix jobs and parameterize bad-byte profiles.
- [ ] 02-02: Integrate semantic/functionality verification scripts with stable fixture selection.
- [ ] 02-03: Emit summarized artifacts and annotate workflow output per architecture.

### Phase 3: ARM Strategy Expansion
**Goal**: ARM users can transform common arithmetic/load-store/conditional patterns with higher success rates.
**Depends on**: Phase 2
**Requirements**: ARM-01, ARM-02, ARM-04
**Success Criteria** (what must be TRUE):
  1. ARM ADD/SUB immediate cases are transformed without introducing banned bytes in covered fixtures.
  2. ARM LDR/STR displacement rewrites preserve runtime behavior in verification runs.
  3. Conditional-instruction alternatives cover documented high-frequency transformation failures.
**Plans**: 3 plans

Plans:
- [ ] 03-01: Implement and test ADD/SUB immediate splitting rewrite paths.
- [ ] 03-02: Implement and test LDR/STR displacement rewrite paths.
- [ ] 03-03: Implement conditional-instruction alternative strategy set with regression fixtures.

### Phase 4: ARM Diagnostic Safety Nets
**Goal**: ARM transformation outcomes are safer through correct branch handling and clearer diagnostics.
**Depends on**: Phase 3
**Requirements**: ARM-03, ARM-05, ARM-06
**Success Criteria** (what must be TRUE):
  1. Branch offsets remain correct after instruction expansion across verification fixtures.
  2. Architecture mismatch heuristics produce actionable warnings before destructive processing.
  3. Experimental ARM warnings clearly describe limits and recommended fallback paths.
**Plans**: 2 plans

Plans:
- [ ] 04-01: Harden branch offset recalculation and add focused fixture tests for edge cases.
- [ ] 04-02: Improve mismatch heuristics and warning text with clear recovery guidance.

### Phase 5: Reproducible Release Gate
**Goal**: Releases are reproducible and blocked if correctness guarantees are not met.
**Depends on**: Phase 4
**Requirements**: REL-01, REL-03
**Success Criteria** (what must be TRUE):
  1. Docker workflow can run end-to-end transformations and verification on canonical fixtures.
  2. Release process includes enforced verification gates for bad-byte freedom and semantic equivalence.
  3. Maintainers can execute a documented release checklist and obtain consistent outcomes.
**Plans**: 2 plans

Plans:
- [ ] 05-01: Finalize Docker build/run workflow and validate parity with host verification outputs.
- [ ] 05-02: Add formal release checklist and gate checks to prevent unverifiable tags.

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4 → 5

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. CI Foundation and Fixture Canonicalization | 0/3 | Not started | - |
| 2. Verification Automation and Reporting | 0/3 | Not started | - |
| 3. ARM Strategy Expansion | 0/3 | Not started | - |
| 4. ARM Diagnostic Safety Nets | 0/2 | Not started | - |
| 5. Reproducible Release Gate | 0/2 | Not started | - |
