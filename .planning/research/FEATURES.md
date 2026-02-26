# Feature Research

**Domain:** Release-gate reliability and architecture maturity for shellcode transformation
**Researched:** 2026-02-26
**Confidence:** MEDIUM

## Feature Landscape

### Table Stakes (Users Expect These)

Features users assume exist. Missing these = release process feels unreliable.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Deterministic equivalence pass/fail across representative fixtures | Release gate must produce stable, reproducible outcomes | HIGH | Requires consistent fixture inputs, transform flags, and verifier semantics |
| Actionable failure artifacts per architecture | Teams need immediate triage path when release gate fails | MEDIUM | Preserve stable JSON/log naming and per-check failure context |
| Local command parity with CI release gate | Maintainers expect to reproduce CI decisions locally | MEDIUM | `make release-gate` must mirror workflow semantics exactly |
| Architecture recommendation clarity before mutation | Users need confidence they selected the right `--arch` | MEDIUM | Must remain explicit guidance, not silent auto-mutation |

### Differentiators (Competitive Advantage)

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Manifest-driven representative fixture policy | Keeps verification fast, deterministic, and maintainable | MEDIUM | Strongly reduces flakiness and undefined fixture drift |
| ARM64 strategy family expansion with safe-decline behavior | Improves real-world ARM64 success while preserving safety | HIGH | Must be coupled with targeted fixtures and semantic validation |
| Cross-arch triage summary linking failed checks to rerun commands | Reduces release-debug time significantly | MEDIUM | Valuable for contributors and maintainers under tight release windows |

### Anti-Features (Commonly Requested, Often Problematic)

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| Full-corpus verification on every gate run | Appears "more thorough" | Slows feedback loop and encourages bypass behavior | Representative deterministic subset in gate + periodic expanded suites |
| Silent automatic architecture switching | Appears "smart" | Hides operator intent errors and can mutate against wrong assumptions | Explicit architecture suggestion and operator-confirmed rerun |
| Byte-identical parity between host and Docker | Appears strict | Breaks on benign environment/toolchain differences | Outcome parity for required verification checks |

## Feature Dependencies

```
[Deterministic equivalence gate]
    └──requires──> [Representative fixture policy]
                       └──requires──> [Stable manifest metadata]

[ARM64 strategy expansion] ──requires──> [ARM64 fixture + semantic checks]

[Architecture assist improvements] ──enhances──> [Release-gate triage clarity]
```

### Dependency Notes

- **Deterministic equivalence gate requires fixture policy:** without fixed representatives, pass/fail is not reproducible.
- **ARM64 strategy expansion requires fixture + semantic checks:** strategy growth without regression evidence raises break risk.
- **Architecture assist enhances release triage:** faster mismatch detection reduces wasted reruns under wrong architecture.

## MVP Definition

### Launch With (v4.4)

- [ ] Deterministic representative equivalence verification that can pass/fail consistently for x86/x64/arm.
- [ ] Failure artifacts that identify fixture/check/architecture and provide direct rerun guidance.
- [ ] Initial ARM64 strategy expansion for core arithmetic/load-store families with safe decline and fixtures.
- [ ] Improved architecture assist messaging with scored recommendation and explicit fallback instructions.

### Add After Validation (v4.4.x)

- [ ] Broader ARM64 strategy breadth beyond core families once v4.4 baseline is stable.
- [ ] Extended representative fixture set for additional exploit pattern families after flake-free baseline period.

### Future Consideration (v5+)

- [ ] Interactive auto-assist workflow for architecture selection (operator approval required).
- [ ] Adaptive fixture-selection policies based on historical flake telemetry.

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| Deterministic equivalence reliability fixes | HIGH | HIGH | P1 |
| Release-gate artifact/rerun clarity | HIGH | MEDIUM | P1 |
| ARM64 core strategy expansion | HIGH | HIGH | P1 |
| Architecture assist scoring improvements | MEDIUM | MEDIUM | P2 |

**Priority key:**
- P1: Must have for milestone success
- P2: Should have, add within milestone if dependencies stay green
- P3: Nice to have, defer

## Competitor Feature Analysis

| Feature | Legacy one-off denull tooling | byvalver v4.3 baseline | v4.4 target approach |
|---------|-------------------------------|-----------------------|----------------------|
| Reproducible release gate | Usually ad-hoc scripts | Present but blocked by equivalence instability | Deterministic and fail-closed with actionable artifacts |
| ARM64 transformation depth | Often absent | Basic experimental coverage | Core strategy family expansion with fixture-backed validation |
| Architecture mismatch guidance | Minimal/manual | Warn-and-continue text present | Scored assist and clearer rerun decision support |

## Sources

- `/opt/byvalver/.planning/PROJECT.md`
- `/opt/byvalver/tests/run_tests.sh`
- `/opt/byvalver/tests/README.md`
- `/opt/byvalver/.planning/phases/04-arm-diagnostic-safety-nets/04-02-SUMMARY.md`
- `/opt/byvalver/.planning/phases/05-reproducible-release-gate/05-0*-SUMMARY.md`

---
*Feature research for: byvalver v4.4 equivalence reliability*
*Researched: 2026-02-26*
