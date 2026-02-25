# byvalver

## Current State

- Latest shipped milestone: `v4.3 stabilization` (2026-02-25).
- Delivery scope: 5 phases, 13 plans, 39 tasks.
- Release policy: required parity/verification mismatches are release-blocking.

## What This Is

byvalver is a C-based CLI that rewrites shellcode to remove forbidden bad bytes while preserving behavior across x86/x64 and experimental ARM/ARM64 targets. It is built for exploit developers, red-team engineers, and reverse engineers who need deterministic bad-byte-safe payload transforms with verification tooling.

## Core Value

Given an input payload and bad-byte policy, byvalver must produce functionally equivalent shellcode that is verifiably free of forbidden bytes.

## Next Milestone Goals

- Define the next milestone scope and requirements after v4.3 closeout.
- Address remaining verification-equivalence failures currently surfaced by the release gate.
- Prioritize next architecture maturity work (ARM64 strategy coverage and architecture detection improvements).

## Requirements

### Validated

- ✓ Cross-architecture CLI processing for x86/x64 with experimental ARM/ARM64 support — existing
- ✓ Profile-based and explicit bad-byte configuration via `--profile` and `--bad-bytes` — existing
- ✓ Multi-strategy transformation engine with fallback behavior and optional biphasic mode — existing
- ✓ Verification scripts for bad-byte checks and functionality/semantic comparison — existing
- ✓ Batch mode and multiple output formats (raw/C/Python/PowerShell/hex string) — existing
- ✓ CI baseline plus deterministic fixture governance for x86/x64/arm — v4.3
- ✓ Automated CI bad-byte + semantic verification evidence and per-architecture reporting — v4.3
- ✓ ARM rewrite maturity and diagnostic safety-net coverage for Phase 3/4 scope — v4.3
- ✓ Reproducible host-vs-Docker release gate path with strict failure policy — v4.3

### Active

- [ ] Resolve representative `verify-equivalence` failures currently blocking green release-gate runs.
- [ ] Expand ARM64 strategy coverage toward beta-level reliability for common AArch64 patterns.
- [ ] Improve architecture-detection assist so operators rely less on manual `--arch` selection.

### Out of Scope

- MIPS support — deferred until post ARM/ARM64 maturity.
- Full TUI redesign — deferred in favor of correctness and verification depth.
- Plugin architecture for third-party strategy packs — deferred until core v4.x stabilization completes

## Context

The codebase is a mature C project with a strategy-registry architecture centered in `src/core.c` and `src/strategy_registry.c`, plus optional ML-assisted prioritization in `src/ml_strategist.c`. Milestone `v4.3` is now shipped and archived in `.planning/milestones/`. A current codebase map exists in `.planning/codebase/` and should be treated as the source of truth for structure/conventions.

## Constraints

- **Compatibility**: Preserve current CLI behavior and existing command flags — maintain backward compatibility for current users.
- **Correctness**: Functional equivalence and bad-byte elimination are non-negotiable — verification must gate changes.
- **Performance**: New checks should not regress core transformation throughput materially — avoid heavy runtime overhead.
- **Scope**: Prioritize v4.3 hardening work before v5/v6 feature expansion — reduce delivery risk.
- **Security**: Documentation and planning artifacts must not include secret values — repository is public-facing.

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Keep C/Capstone core architecture for v4.3 work | Existing transformation engine is mature and proven; rewrite risk is unnecessary | ✓ Good |
| Treat current shipped capabilities as validated baseline | Brownfield repo already contains production-level functionality | ✓ Good |
| Target v4.3 stabilization before v5/v6 expansion | Test and ARM quality gaps were the highest current bottlenecks | ✓ Good |
| Keep verification scripts central in acceptance flow | Functional equivalence and bad-byte guarantees define product trust | ✓ Good |
| Define release reproducibility as host-vs-Docker outcome parity | Operational confidence depends on equivalent verification outcomes, not byte-identical artifacts | ✓ Good |

---
*Last updated: 2026-02-25 after v4.3 milestone completion*
