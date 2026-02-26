# Pitfalls Research

**Domain:** Reliability and architecture expansion pitfalls in shellcode transformation gates
**Researched:** 2026-02-26
**Confidence:** MEDIUM

## Critical Pitfalls

### Pitfall 1: Non-deterministic equivalence outcomes across identical inputs

**What goes wrong:**
Release-gate status flips between runs (or host vs Docker) without code changes.

**Why it happens:**
Fixture selection, transform flags, verifier invocation, or timeout behavior is inconsistent.

**How to avoid:**
Lock representative fixture ordering from manifest metadata and enforce one summary schema across host/docker runs.

**Warning signs:**
Same fixture/check combination alternates PASS/FAIL across consecutive runs.

**Phase to address:**
Phase 6 (equivalence reliability)

---

### Pitfall 2: Architecture-mismatched equivalence checks produce misleading failures

**What goes wrong:**
Verification fails because checks execute under the wrong architecture assumptions, not because transformations are semantically incorrect.

**Why it happens:**
Operator picks wrong `--arch`, or assist guidance is too weak to redirect quickly.

**How to avoid:**
Emit scored architecture recommendations pre-transform and attach explicit rerun commands in failure summaries.

**Warning signs:**
High mismatch-warning frequency paired with broad cross-check failures.

**Phase to address:**
Phase 8 (architecture assist)

---

### Pitfall 3: ARM64 strategy growth without safe-decline constraints

**What goes wrong:**
New strategies increase mutation coverage but introduce semantic regressions or unstable fallbacks.

**Why it happens:**
Strategies are added without bounded eligibility checks or fixture-backed regression gates.

**How to avoid:**
Require safe-decline behavior and deterministic ARM64 fixture evidence per strategy family before expansion.

**Warning signs:**
Strategy success rate rises while semantic verification failures increase.

**Phase to address:**
Phase 7 (ARM64 strategy expansion)

---

### Pitfall 4: Gate failures lack actionable triage paths

**What goes wrong:**
Maintainers cannot quickly reproduce and diagnose release-gate failures.

**Why it happens:**
Artifacts are missing fixture/check-level context or do not include local rerun commands.

**How to avoid:**
Standardize per-arch summary fields and publish deterministic rerun command snippets in CI summaries/docs.

**Warning signs:**
Repeated manual debugging without converging on a failing fixture/check combination.

**Phase to address:**
Phase 6 (equivalence reliability)

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Broad skip-on-failure handling for flaky checks | Faster short-term green pipelines | Hides true regressions and weakens release confidence | Never for required release checks |
| Adding ARM64 strategies without fixtures | Faster apparent coverage growth | Hard-to-debug regressions and unclear ownership | Only for prototype branches, never for milestone gate commits |
| Free-form log parsing for gate decisions | Quick implementation | Fragile automation and broken summaries | Never when JSON summary contract exists |

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| GitHub Actions release gate | Triggering on broad branch events | Restrict to tag pushes + workflow_dispatch and keep explicit gate stage |
| Docker parity runs | Relying on environment-specific mounts | Use mount-free image-contained execution where possible |
| Fixture manifest | Missing metadata fields in new entries | Validate required fields and fail preflight fast |

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Running full fixture corpus in required gate | Slow release checks, timeout pressure | Keep representative deterministic core for release gate | As fixture corpus grows past small baseline |
| Re-running full matrix for single failing fixture | High triage latency | Emit fixture/check-level rerun command and narrow-scope mode | During frequent release iterations |
| Overly verbose artifact payloads | Hard to scan CI outputs | Keep concise summaries and archive full logs separately | Multi-arch failure scenarios |

## Security Mistakes

| Mistake | Risk | Prevention |
|---------|------|------------|
| Logging sensitive payload bytes in CI summaries | Leakage of sensitive shellcode material | Keep summaries metadata-focused; restrict raw bytes to controlled artifacts |
| Relaxing required checks for convenience | Shipping unverifiable transformations | Maintain fail-closed required-check policy |
| Embedding secret values in planning/docs | Accidental credential exposure | Keep planning artifacts secret-free and use environment-based auth only |

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| Ambiguous mismatch warning text | Users rerun with wrong flags repeatedly | Provide explicit suggested `--arch` and dry-run path |
| Too many gate modes with unclear ownership | Contributor confusion | Keep canonical local commands and document mode intent |
| Missing linkage from failure summary to docs | Slow onboarding and triage | Include direct doc pointers and deterministic rerun sequence |

## "Looks Done But Isn't" Checklist

- [ ] **Equivalence reliability:** Gate passes on one machine but not across host/docker parity.
- [ ] **ARM64 expansion:** New strategies compile but lack deterministic fixture + semantic evidence.
- [ ] **Architecture assist:** Warning exists but does not provide a concrete rerun action.
- [ ] **Release diagnostics:** CI reports failure but does not identify fixture/check tuple.

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Non-deterministic equivalence | HIGH | Freeze fixture set, compare host/docker summary deltas, isolate failing tuple, patch runner/script contract |
| Architecture mismatch false failures | MEDIUM | Re-run with suggested architecture, capture decode scores, tune threshold heuristics |
| Unsafe ARM64 strategy rollout | HIGH | Disable offending family behind guard, add safe-decline tests, re-enable incrementally |

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Non-deterministic equivalence outcomes | Phase 6 | Repeatability checks produce stable fixture/check status across reruns |
| Missing actionable gate diagnostics | Phase 6 | CI summary includes failing tuple + rerun command |
| ARM64 strategy regressions | Phase 7 | New families pass deterministic fixture and semantic verification suite |
| Architecture mismatch confusion | Phase 8 | Warning guidance reduces mismatch rerun loop and improves first-correct-run rate |

## Sources

- `/opt/byvalver/tests/run_tests.sh`
- `/opt/byvalver/tests/README.md`
- `/opt/byvalver/src/main.c`
- `/opt/byvalver/src/core.c`
- `/opt/byvalver/.planning/phases/04-arm-diagnostic-safety-nets/04-02-SUMMARY.md`
- `/opt/byvalver/.planning/phases/05-reproducible-release-gate/05-0*-SUMMARY.md`

---
*Pitfalls research for: byvalver v4.4 equivalence reliability*
*Researched: 2026-02-26*
