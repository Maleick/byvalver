# Phase 4: ARM Diagnostic Safety Nets - Research

**Researched:** 2026-02-25  
**Phase:** 04-arm-diagnostic-safety-nets  
**Requirements:** ARM-03, ARM-05, ARM-06

## Objective

Define a low-risk implementation plan that improves ARM branch correctness after rewrite expansion and adds actionable architecture/experimental diagnostics while preserving current processing continuity.

## Current Baseline (Repository Findings)

- ARM branch transformations currently live in `src/arm_strategies.c`, including:
  - `arm_branch_original`
  - `arm_branch_conditional_alt`
- ARM branch helpers in `src/arm_immediate_encoding.{c,h}` provide:
  - `decode_arm_branch_offset`
  - `encode_arm_branch_instruction`
  - condition inversion helpers
- `src/core.c` contains relocation-aware relative jump processing, but this flow is x86-centric (`is_relative_jump`, x86 opcode handling in `process_relative_jump`).
- CLI/runtime messaging currently includes generic "ARM/ARM64 support is experimental" wording in `src/cli.c` and architecture display in `src/main.c`.
- Deterministic fixture selection is already manifest-driven via `tests/fixtures/manifest.yaml` and consumed by `tests/run_tests.sh`.

## Gap Analysis vs Phase 4 Requirements

### ARM-03 (branch offset recalculation correctness)

**Gap:** ARM branch alternative rewriting currently derives branch offset from original instruction context and does not have an explicit relocation-aware safety contract tied to expanded instruction layout.

**Implication:** edge-case branch distances (forward/backward and near-limit offsets) risk incorrect control flow when instruction expansion changes placement assumptions.

### ARM-05 (architecture mismatch heuristic warnings)

**Gap:** there is no explicit pre-transform warning path for likely architecture mismatch beyond argument-level architecture selection.

**Implication:** users can process inputs with likely wrong architecture intent without actionable early warnings.

### ARM-06 (experimental guidance and fallback recommendations)

**Gap:** ARM experimental messaging exists but is generic and not tightly linked to runtime diagnostic/fallback guidance.

**Implication:** users do not consistently receive concrete "what next" instructions when ARM path risk signals appear.

## Locked Constraints from Context

- Scope is Phase 4 only (`04-01`, `04-02`).
- Warning policy is **warn-and-continue** by default (no fail-by-default mismatch blocking).
- Branch correctness hardening must use deterministic regression fixtures.
- Capability expansion outside branch safety nets and diagnostics is out of scope.

## Recommended Implementation Shape

### 04-01: Branch Offset Recalculation Safety Net (ARM-03)

- Add explicit ARM branch offset recalculation safeguards for expanded branch rewrite paths.
- Validate transformed branch targets/fall-through semantics across:
  - forward offsets
  - backward offsets
  - boundary-sensitive offsets
- Ensure unsupported/high-risk branch forms decline rewrite safely.
- Add deterministic branch edge-case fixture coverage and focused ARM strategy tests.

### 04-02: Mismatch + Experimental Diagnostics (ARM-05, ARM-06)

- Add pre-transform mismatch heuristics and emit actionable warnings before destructive processing.
- Keep default behavior warn-and-continue.
- Unify and improve ARM experimental messaging in help/runtime output with explicit fallback recommendations.
- Add test/doc updates that validate warning path expectations and triage guidance.

## Fixture and Verification Strategy

- Add deterministic ARM branch safety fixtures in `tests/fixtures/manifest.yaml`.
- Extend focused ARM tests (`assets/tests/test_arm_strategies.c` or dedicated ARM diagnostic test files) to cover edge-case offset behavior.
- Reuse deterministic verification paths:
  - `bash tests/run_tests.sh --mode verify-denulled --arch arm`
  - `bash tests/run_tests.sh --mode verify-equivalence --arch arm`
- Add targeted architecture-warning validation tests in `assets/tests/` for mismatch diagnostics behavior.

## Risks and Mitigations

1. Incorrect branch target after expansion.
- Mitigation: explicit offset recalculation checks + deterministic edge-case fixtures + safe decline behavior.

2. Overly noisy mismatch warnings.
- Mitigation: bounded heuristics and actionable-but-concise warning format.

3. Behavioral regression from diagnostics changes.
- Mitigation: warn-and-continue default plus focused tests/doc triage updates.

4. Scope creep into auto-detection platform work.
- Mitigation: keep mismatch detection as heuristic warning layer only in Phase 4.

## Plan Decomposition Recommendation

- Wave 1: `04-01` (ARM-03) branch offset recalculation hardening + fixtures/tests.
- Wave 2: `04-02` (ARM-05, ARM-06) mismatch/experimental diagnostics + docs/tests.

This ordering reduces control-flow risk first, then adds operator-facing diagnostics on top of a safer branch base.

## Definition of Done (Phase Planning Inputs)

- `04-01` maps to `ARM-03` with must-have truths for branch correctness and deterministic evidence.
- `04-02` maps to `ARM-05` and `ARM-06` with warn-and-continue diagnostics contract.
- Both plans include executable tasks, explicit artifacts, and key links usable by phase verifier.

---

*Phase: 04-arm-diagnostic-safety-nets*  
*Research completed: 2026-02-25*
