# byvalver

## What This Is

byvalver is a C-based CLI that rewrites shellcode to remove forbidden bad bytes while preserving behavior across x86/x64 and experimental ARM/ARM64 targets. It is built for exploit developers, red-team engineers, and reverse engineers who need deterministic bad-byte-safe payload transforms with verification tooling. The current brownfield focus is hardening v4.3 around test infrastructure and ARM maturity.

## Core Value

Given an input payload and bad-byte policy, byvalver must produce functionally equivalent shellcode that is verifiably free of forbidden bytes.

## Requirements

### Validated

- ✓ Cross-architecture CLI processing for x86/x64 with experimental ARM/ARM64 support — existing
- ✓ Profile-based and explicit bad-byte configuration via `--profile` and `--bad-bytes` — existing
- ✓ Multi-strategy transformation engine with fallback behavior and optional biphasic mode — existing
- ✓ Verification scripts for bad-byte checks and functionality/semantic comparison — existing
- ✓ Batch mode and multiple output formats (raw/C/Python/PowerShell/hex string) — existing

### Active

- [ ] CI pipeline executes build and architecture-aware verification automatically on PRs
- [ ] Categorized fixture corpus is standardized and wired into deterministic automated checks
- [ ] ARM strategy coverage is expanded to improve practical success on real-world shellcode
- [ ] ARM mismatch warnings and diagnostics are made more actionable for users
- [ ] Reproducible container workflow is production-ready for contributors and CI parity

### Out of Scope

- ARM64 graduation to beta with broad strategy coverage — planned for later milestone (v5.0+)
- Automatic architecture detection replacing `--arch` — deferred to future milestone
- Plugin architecture for third-party strategy packs — deferred until core v4.x stabilization completes

## Context

The codebase is a mature C project with a strategy-registry architecture centered in `src/core.c` and `src/strategy_registry.c`, plus optional ML-assisted prioritization in `src/ml_strategist.c`. Existing roadmap intent in `ROADMAP.md` identifies v4.3 as the next stabilization release focused on CI, test corpus quality, and ARM expansion. A fresh map exists in `.planning/codebase/` and should be treated as the source of truth for current structure and conventions.

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
| Target v4.3 stabilization before v5/v6 expansion | Test and ARM quality gaps are current bottlenecks | — Pending |
| Keep verification scripts central in acceptance flow | Functional equivalence and bad-byte guarantees define product trust | ✓ Good |

---
*Last updated: 2026-02-25 after initialization*
