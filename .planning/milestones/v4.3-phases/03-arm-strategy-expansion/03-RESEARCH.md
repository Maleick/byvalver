# Phase 3: ARM Strategy Expansion - Research

**Researched:** 2026-02-25  
**Phase:** 03-arm-strategy-expansion  
**Requirements:** ARM-01, ARM-02, ARM-04

## Objective

Define a low-risk, testable ARM strategy expansion plan that improves rewrite success for high-frequency arithmetic/load-store/conditional cases while preserving deterministic verification behavior introduced in Phase 2.

## Current Baseline (Repository Findings)

- ARM strategy registration is isolated in `src/arm_strategies.c` via `register_arm_strategies()`.
- Existing ARM transformations are narrow:
  - MOV original + MOV->MVN
  - ADD original + ADD->SUB
  - LDR original
  - STR original
  - B/BL original
- Immediate helper support exists in `src/arm_immediate_encoding.c` for encodability and MVN equivalence.
- CI/test runner supports ARM verification modes in `tests/run_tests.sh`, with deterministic fixture manifest selection via `tests/fixtures/manifest.yaml`.
- ARM fixture inventory is currently sparse and needs targeted additions for rewrite-family regressions.

## Gap Analysis vs Phase 3 Requirements

### ARM-01 (ADD/SUB immediate rewrite paths)

**Gap:** current ARM arithmetic support is mostly pass-through plus simple ADD->SUB negative-immediate fallback. There is no structured immediate splitting strategy for cases where immediate bytes/opcodes violate profile constraints.

**Implication:** frequent arithmetic transformations fail when single-immediate forms cannot satisfy bad-byte policy.

### ARM-02 (LDR/STR displacement rewrite paths)

**Gap:** load/store handling is currently pass-through only. Displacement-heavy operands are not rewritten when displacement encoding introduces bad bytes.

**Implication:** memory-access transformations fail for common stack/global offset patterns.

### ARM-04 (conditional alternatives)

**Gap:** branch strategy exists only as pass-through. Conditional alternatives are missing for high-frequency branch cases.

**Implication:** conditional bad-byte failures cannot be recovered in common branch scenarios.

## Locked Constraints from Context

- Scope is conservative-core only: roadmap plans `03-01`, `03-02`, `03-03`.
- Conditional scope is branch-first only in this phase.
- Every rewrite family touched must include deterministic ARM regression fixtures.
- No ARM64 scope expansion.
- Unsupported patterns must decline rewrite safely.

## Recommended Implementation Shape

### 03-01: ADD/SUB Immediate Splitting

- Add bounded immediate split helpers for arithmetic rewrites.
- Support only explicitly safe operand forms first (register/immediate variants with clear semantic mapping).
- Preserve flag behavior contracts for covered instructions and avoid implicit side effects.
- If no safe split candidate exists under active bad-byte profile, strategy should decline.

### 03-02: LDR/STR Displacement Rewrites

- Implement displacement rewrite logic for selected immediate-offset forms only.
- Keep addressing-mode support explicit and narrow in Phase 3 (with clear unsupported list).
- Ensure transformed sequence preserves effective address and read/write semantics.
- Protect against unsafe scratch-register usage by enforcing explicit policy in strategy preconditions.

### 03-03: Branch-First Conditional Alternatives

- Implement conditional-branch alternatives (for example, invert-condition + control-flow-preserving sequence) for documented high-frequency failures.
- Keep alternatives constrained to branch instructions; do not include predicated ALU/memory in this phase.
- Ensure generated branch alternatives preserve target and fall-through semantics for covered patterns.

## Fixture and Verification Strategy

- Add deterministic ARM fixtures per rewrite family:
  - arithmetic-split cases
  - load/store displacement cases
  - conditional-branch alternative cases
- Register fixtures in `tests/fixtures/manifest.yaml` with stable IDs and notes.
- Reuse Phase 2 verification entry points:
  - `bash tests/run_tests.sh --mode verify-denulled --arch arm`
  - `bash tests/run_tests.sh --mode verify-equivalence --arch arm`
- Add focused strategy-level regression tests in `assets/tests/test_arm_strategies.c` (or equivalent ARM test harness files).

## Risks and Mitigations

1. Semantic drift from rewrite expansion.
- Mitigation: explicit supported-pattern contracts + deterministic equivalence checks + safe decline behavior.

2. Addressing-mode corner cases for LDR/STR.
- Mitigation: constrain Phase 3 coverage to well-defined displacement forms and defer broader modes.

3. Conditional rewrite overreach.
- Mitigation: branch-first scope guard; defer predicated ALU/memory alternatives.

4. Insufficient ARM fixture coverage.
- Mitigation: mandatory new fixtures for each rewrite family before marking tasks complete.

## Plan Decomposition Recommendation

- Wave 1: `03-01` (ARM-01) arithmetic split foundation.
- Wave 2: `03-02` (ARM-02) displacement rewrites built on immediate helper confidence.
- Wave 3: `03-03` (ARM-04) branch-first conditional alternatives + docs/fixture updates.

This sequencing minimizes compounding risk and keeps each requirement independently verifiable.

## Definition of Done (Phase Planning Inputs)

- Each plan maps to one primary requirement (`ARM-01`, `ARM-02`, `ARM-04`).
- Each plan includes deterministic fixture obligations in tasks.
- Each plan includes concrete verification hooks and must-have artifacts/key links.
- Plans are autonomous and wave-ordered for execution workflow compatibility.

---

*Phase: 03-arm-strategy-expansion*  
*Research completed: 2026-02-25*
