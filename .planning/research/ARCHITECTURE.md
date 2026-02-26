# Architecture Research

**Domain:** Multi-architecture shellcode transformation with verification-driven release gating
**Researched:** 2026-02-26
**Confidence:** MEDIUM

## Standard Architecture

### System Overview

```
+-------------------------------------------------------------------+
| CLI + Config Layer                                                |
|  parse args/profile/arch -> validate policy -> emit warnings      |
+---------------------------+---------------------------------------+
                            |
+---------------------------v---------------------------------------+
| Transformation Core                                               |
|  disassemble (Capstone) -> strategy registry -> mutate or decline |
+---------------------------+---------------------------------------+
                            |
+---------------------------v---------------------------------------+
| Verification Harness                                               |
|  verify_denulled + verify_functionality + verify_semantic         |
+---------------------------+---------------------------------------+
                            |
+---------------------------v---------------------------------------+
| Release-Gate Orchestration                                         |
|  tests/run_tests.sh modes -> JSON/log artifacts -> CI summaries   |
+-------------------------------------------------------------------+
```

### Component Responsibilities

| Component | Responsibility | Typical Implementation |
|-----------|----------------|------------------------|
| `src/cli.c` + `src/main.c` | Operator contract, warning policy, execution mode entry | Explicit flags and warn-and-continue guardrails |
| `src/core.c` + strategy files | Decode/transform path by architecture | Registry-driven strategy execution with safe fallback |
| `tests/run_tests.sh` | Deterministic gate orchestration and artifact schema | Mode-based runner (`verify-*`, `release-gate`, parity compare) |
| Verification scripts (`verify_*.py`) | Equivalence and bad-byte correctness checks | Fixture-driven script invocations with machine-readable summaries |
| GitHub workflow (`release-gate.yml`) | Enforced release checks on tags/manual dispatch | Fail-closed job with uploaded triage artifacts |

## Recommended Project Structure

```
src/
├── core.c                         # arch selection and transform flow
├── cli.c                          # user-facing flags and guidance
├── main.c                         # runtime orchestration and warnings
├── strategy_registry.c            # strategy registration dispatch
├── arm_strategies.c               # ARM rewrite families
├── arm64_strategies.c             # ARM64 rewrite families (v4.4 expansion target)
└── arm_immediate_encoding.*       # bounded encoding helper utilities

tests/
├── run_tests.sh                   # deterministic gate orchestration
├── fixtures/manifest.yaml         # representative fixture source-of-truth
└── README.md                      # operator and triage workflow docs

.github/workflows/
└── release-gate.yml               # enforced tag/manual gate path
```

### Structure Rationale

- **`src/*` strategy boundaries:** allows architecture-specific growth without destabilizing x86/x64 baseline.
- **`tests/run_tests.sh` as orchestration spine:** keeps local and CI gate behavior aligned.
- **Manifest-first fixtures:** prevents hidden fixture drift and stabilizes parity outcomes.

## Architectural Patterns

### Pattern 1: Deterministic Representative Selection

**What:** Select fixtures only from manifest metadata (`ci_representative: true`) with stable ordering.
**When to use:** Any required gate signal (release, CI status checks).
**Trade-offs:** Faster and stable, but limited breadth per run.

### Pattern 2: Safe-Decline Strategy Expansion

**What:** New rewrite families must decline safely when constraints are not satisfied.
**When to use:** ARM/ARM64 strategy additions where semantic uncertainty exists.
**Trade-offs:** Lower immediate coverage, higher correctness confidence.

### Pattern 3: Fail-Closed Gate with Artifact-First Triage

**What:** Required verification mismatches fail release; diagnostics are emitted as structured artifacts.
**When to use:** Release and milestone confidence boundaries.
**Trade-offs:** Stricter release process, but clearer quality contracts.

## Data Flow

### Request Flow

```
[CLI invocation]
    -> [core transform by --arch/profile]
    -> [verification scripts on output]
    -> [runner writes summary JSON + logs]
    -> [CI/maintainer consumes artifacts]
```

### State Management

```
[manifest.yaml + runner mode args]
    -> determine fixture/check set
    -> execute checks deterministically
    -> emit stable status model (PASS/FAIL/SKIP)
```

### Key Data Flows

1. **Transform correctness flow:** input shellcode -> transformed output -> functional/semantic verification.
2. **Release parity flow:** host summaries vs docker summaries -> parity compare -> gate decision.
3. **Architecture assist flow:** decode-coverage signal -> warning/recommendation -> operator rerun path.

## Scaling Considerations

| Scale | Architecture Adjustments |
|-------|--------------------------|
| Single maintainer release flow | Representative fixture subset with mandatory artifacts |
| Frequent release cadence | More granular artifact partitioning and quick rerun commands |
| Broader architecture matrix | Split verification stages by architecture family and keep fail-late summaries |

### Scaling Priorities

1. **First bottleneck:** verification runtime growth as fixture set expands; solve with representative-core plus scheduled extended runs.
2. **Second bottleneck:** triage latency across architectures; solve with per-arch failure summaries and rerun scripts.

## Anti-Patterns

### Anti-Pattern 1: Coupling gate policy to ad-hoc script output

**What people do:** Parse free-form logs as contract.
**Why it's wrong:** Small message changes break automation and triage.
**Do this instead:** Keep a stable JSON summary schema and treat logs as supplemental.

### Anti-Pattern 2: Expanding ARM64 strategies without phase-gated fixtures

**What people do:** Add strategies broadly and rely on manual testing.
**Why it's wrong:** Regression risk rises and failure source becomes ambiguous.
**Do this instead:** Expand by rewrite family with deterministic fixtures and requirement mapping.

## Integration Points

### External Services

| Service | Integration Pattern | Notes |
|---------|---------------------|-------|
| GitHub Actions | tag/manual workflow -> run release gate -> publish artifacts | Keep trigger contract explicit (`v*`, `workflow_dispatch`) |
| Docker/Compose | containerized verifier execution for parity checks | Keep mount-free patterns to avoid host path constraints |

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| `src/core.c` <-> strategy files | in-process API and strategy registry | maintain explicit architecture capability checks |
| runner <-> verification scripts | command invocation + summary files | stable check IDs and status enums required |
| manifest <-> runner | metadata contract | missing fields must fail fast in preflight |

## Sources

- `/opt/byvalver/src/core.c`
- `/opt/byvalver/src/main.c`
- `/opt/byvalver/src/cli.c`
- `/opt/byvalver/tests/run_tests.sh`
- `/opt/byvalver/tests/fixtures/manifest.yaml`
- `/opt/byvalver/.github/workflows/release-gate.yml`

---
*Architecture research for: byvalver v4.4 equivalence reliability*
*Researched: 2026-02-26*
