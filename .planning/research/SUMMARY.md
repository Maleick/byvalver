# Project Research Summary

**Project:** byvalver
**Domain:** Multi-architecture shellcode transformation reliability and release assurance
**Researched:** 2026-02-26
**Confidence:** MEDIUM

## Executive Summary

v4.4 should focus on one primary outcome: restore trust in release-gate decisions. The current release policy is correct (fail-closed) but representative equivalence checks still fail, so release confidence is constrained by reliability and triage quality rather than missing gate plumbing.

The most effective approach is to treat deterministic equivalence outcomes as the first milestone deliverable, then expand ARM64 strategy coverage behind safe-decline constraints, and finally strengthen architecture-assist guidance so users can quickly rerun with the correct architecture assumptions.

Key risk is adding feature breadth before gate reliability. Mitigation is strict phase ordering: reliability first, expansion second, assist polish third.

## Key Findings

### Recommended Stack

The existing C + Capstone + Python verification stack is still appropriate. No stack rewrite is required. Focus should remain on deterministic runner behavior, stable summary schemas, and CI/local parity.

**Core technologies:**
- C (C99): transformation engine and strategy runtime - maintain existing performance and architecture control.
- Capstone 5.x: architecture-aware disassembly and mismatch heuristics - foundational to rewrite and assist logic.
- Python 3.10+: verification scripts and summary generation - keep outputs structured and stable for gates.

### Expected Features

**Must have (table stakes):**
- Deterministic equivalence gate outcomes on representative fixtures.
- Actionable per-architecture failure artifacts and rerun guidance.
- Local command path that mirrors CI release-gate behavior.

**Should have (competitive):**
- ARM64 strategy family expansion with deterministic fixture evidence.
- Stronger architecture-assist scoring and fallback guidance.

**Defer (v5+):**
- Silent automatic architecture switching in mutation path.
- Full-corpus required gating for every release attempt.

### Architecture Approach

Keep the existing architecture and evolve the orchestration boundaries: runner contract stability, manifest-driven fixture determinism, and explicit strategy safety constraints. Most v4.4 work should touch `tests/run_tests.sh`, verification scripts, ARM64 strategy files, and architecture-assist messaging in CLI/core paths.

**Major components:**
1. Gate orchestration layer (`tests/run_tests.sh`) - deterministic check execution and artifact schema.
2. Strategy layer (`src/*strategies*.c`) - controlled ARM64 expansion with safe-decline behavior.
3. Assist/diagnostics layer (`src/core.c`, `src/main.c`, `src/cli.c`) - architecture recommendation and operator guidance.

### Critical Pitfalls

1. **Non-deterministic equivalence results** - lock fixture ordering and summary schema.
2. **Architecture-mismatched verification** - emit scored recommendations and explicit rerun commands.
3. **Unsafe ARM64 strategy expansion** - gate every family behind fixtures + semantic checks.
4. **Non-actionable release failures** - include failing tuple and rerun path in summaries.

## Implications for Roadmap

Based on research, suggested phase structure:

### Phase 6: Equivalence Reliability and Gate Diagnostics
**Rationale:** Release confidence is blocked here today.
**Delivers:** Deterministic equivalence outcomes and artifact-level triage guidance.
**Addresses:** release reliability table stakes.
**Avoids:** non-deterministic gate outcomes and opaque failures.

### Phase 7: ARM64 Core Strategy Expansion
**Rationale:** Expand architecture maturity only after gate stability.
**Delivers:** Core ARM64 strategy family coverage with deterministic fixture evidence.
**Uses:** existing C/Capstone strategy architecture.
**Implements:** safe-decline expansion pattern.

### Phase 8: Architecture Assist and Operator Guidance
**Rationale:** Reduce wrong-arch reruns and improve first-pass success.
**Delivers:** stronger architecture recommendations and deterministic fallback workflows.
**Addresses:** mismatch triage latency and user decision friction.

### Phase Ordering Rationale

- Deterministic gating precedes strategy expansion to preserve trust in results.
- ARM64 expansion precedes assist polish so guidance can reference real capability improvements.
- Assist improvements complete the milestone by reducing operational error rates in daily use.

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 7:** ARM64 instruction-family prioritization and safest rollout ordering.
- **Phase 8:** architecture scoring thresholds tuned to avoid false-positive noise.

Phases with standard patterns (skip deep research-phase if needed):
- **Phase 6:** deterministic runner/artifact contract hardening uses established in-repo patterns.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | Existing stack already supports target outcomes; work is mostly behavioral hardening |
| Features | MEDIUM | Priority is clear, but requirement granularity depends on current verifier failure root causes |
| Architecture | MEDIUM | Structure is stable; risk is in execution details of ARM64 strategy rollout |
| Pitfalls | MEDIUM | Pitfalls are well-known from v4.3 outcomes, but threshold tuning still needs empirical iteration |

**Overall confidence:** MEDIUM

### Gaps to Address

- Root-cause taxonomy for current representative `verify-equivalence` failures should be captured early in Phase 6 planning.
- ARM64 fixture coverage inventory needs explicit prioritization by instruction-family impact.

## Sources

### Primary (HIGH confidence)
- `/opt/byvalver/.planning/PROJECT.md` - current milestone goals and constraints.
- `/opt/byvalver/tests/run_tests.sh` - release-gate and parity orchestration contract.
- `/opt/byvalver/tests/fixtures/manifest.yaml` - deterministic fixture policy.
- `/opt/byvalver/.planning/phases/05-reproducible-release-gate/05-0*-SUMMARY.md` - known gate behavior and failure context.

### Secondary (MEDIUM confidence)
- `/opt/byvalver/src/main.c` and `/opt/byvalver/src/core.c` - architecture warning and assist behavior.
- `/opt/byvalver/docs/USAGE.md` and `/opt/byvalver/tests/README.md` - contributor guidance and diagnostics workflow.

### Tertiary (LOW confidence)
- Historical notes in `README.md` about ARM64 maturity labeling; requires validation against current code coverage.

---
*Research completed: 2026-02-26*
*Ready for roadmap: yes*
