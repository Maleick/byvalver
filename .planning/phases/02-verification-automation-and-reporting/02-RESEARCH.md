# Phase 2: Verification Automation and Reporting - Research

**Researched:** 2026-02-25
**Domain:** CI verification integration for bad-byte profile checks, functionality checks, semantic checks, and per-architecture diagnostics
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Bad-byte profile coverage policy
- CI verification must use explicit `verify_denulled.py` profile configuration.
- Required profile set for phase evidence is `null-only` and `http-newline`.
- `null-only` runs on `x86`, `x64`, and `arm`.
- `http-newline` runs on `x86` and `x64` in Phase 2.
- Profile coverage must be visible in artifact names and summary rows.

### Representative fixture selection policy
- Verification uses a deterministic curated representative subset, not full corpus.
- Selection must come from repository-tracked metadata, not glob order.
- Missing/invalid selected fixtures fail with fixture ID/path context.

### Verification runner behavior policy
- Verification should be fail-late by architecture to maximize diagnostic surface.
- Required checks: bad-byte profile validation + functionality + semantic checks.
- Raw logs must be preserved and machine-parseable status emitted.

### Reporting and artifact contract
- Per-architecture artifact structure and naming must be stable.
- Artifact names include architecture, check type, and profile where applicable.
- CI summary should provide per-architecture pass/fail rows plus aggregate verdict.

### Deferred Ideas (OUT OF SCOPE)
- Additional aggressive profile sets (for example `sql-injection`, `alphanumeric-only`).
- Full exhaustive fixture verification per PR.
- Rich dashboard visualization outside GitHub artifact/summary surfaces.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| TEST-03 | CI executes bad-byte verification for configured profiles with `verify_denulled.py` across representative fixtures. | Existing `verify_denulled.py` already supports explicit `--profile` values; phase work should wire profile matrix inputs and deterministic fixture subset selection into CI execution. |
| TEST-04 | CI executes semantic/functional verification using `verify_functionality.py` and `verify_semantic.py`. | Both scripts exist and can run headless in CI; phase work should add deterministic invocation and normalized pass/fail parsing. |
| TEST-05 | CI publishes per-architecture pass/fail summary artifacts. | Current CI already uploads baseline logs; phase work should standardize verification artifact contracts and aggregate summaries with architecture-check breakdowns. |
</phase_requirements>

## Summary

The repository already has the core verification scripts (`verify_denulled.py`, `verify_functionality.py`, `verify_semantic.py`) and a fail-late architecture CI matrix from Phase 1. The main gap is orchestration: there is no deterministic representative fixture subset contract, no profile-aware verification integration, and no consistent per-architecture verification artifact schema for all checks.

Phase 2 should extend the existing test runner and CI workflow rather than introducing new frameworks. The safest path is to add explicit verification mode/selection semantics to the current runner stack, then wire CI jobs to execute all configured checks and publish machine-readable plus human-readable summaries.

**Primary recommendation:** implement in three layers: (1) profile-aware bad-byte verification wiring, (2) deterministic functionality/semantic verification runner integration, (3) artifact/summary publication contract.

## Standard Stack

### Core
| Library/Tool | Version | Purpose | Why Standard |
|--------------|---------|---------|--------------|
| GitHub Actions (`.github/workflows/ci.yml`) | Existing repo workflow | Architecture matrix orchestration and artifact publication | Already used for baseline checks; supports fail-late aggregation and summary generation. |
| Bash runner (`tests/run_tests.sh`) | Existing repo script | Deterministic test and verification orchestration | Already canonical local/CI parity path from Phase 1. |
| `verify_denulled.py` | Existing repo script | Profile-aware bad-byte safety validation | Supports `--profile` and explicit profile listing; no new verifier required. |
| `verify_functionality.py` | Existing repo script | Functional behavior sanity checks | Existing script handles non-interactive execution and return codes. |
| `verify_semantic.py` | Existing repo script | Semantic-equivalence checks | Existing script provides structured analysis suitable for CI gating. |

### Supporting
| Library/Tool | Version | Purpose | When to Use |
|--------------|---------|---------|-------------|
| `tests/fixtures/manifest.yaml` | Phase 1 canonical metadata source | Deterministic fixture selection metadata | Use as source for representative subset and architecture mapping. |
| `actions/upload-artifact@v4` | Existing CI dependency | Persist per-architecture logs and summaries | Use for each verification check output and aggregate summaries. |
| GitHub Step Summary (`$GITHUB_STEP_SUMMARY`) | GitHub built-in | Human-readable run summary | Use per architecture and aggregate sections. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Extending existing runner | Introduce new Python orchestration harness | More moving parts and migration risk during stabilization. |
| Manifest-driven deterministic subset | Dynamic random/sample fixture selection | Non-deterministic CI behavior and hard-to-reproduce failures. |
| Existing CI workflow extension | Separate standalone verification workflow | Fragmented gating and harder contributor parity. |

## Architecture Patterns

### Pattern 1: Deterministic Fixture Selection Contract
**What:** derive verification fixture list from repository metadata with stable ordering.
**When to use:** all profile/functionality/semantic checks in CI and local verification mode.
**Example:** parse `manifest.yaml` entries for representative fixtures by architecture, then run in sorted `fixture_id` order.

### Pattern 2: Architecture-Scoped Verification Pipeline
**What:** execute bad-byte profile checks + functionality + semantic checks for each architecture in one CI job.
**When to use:** architecture matrix jobs for `x86`, `x64`, and `arm`.
**Example:** per-arch pipeline emits `verify-<arch>-denulled-<profile>.log`, `verify-<arch>-functionality.log`, `verify-<arch>-semantic.log`, `summary-<arch>.json`.

### Pattern 3: Fail-Late Diagnostics with Aggregate Gate
**What:** always collect per-architecture results/artifacts before failing final gate.
**When to use:** CI matrix with `fail-fast: false` + final `needs` aggregate job.
**Example:** final summary job fails only after per-arch artifacts are uploaded and summary rows are written.

### Anti-Patterns to Avoid
- Running verification on glob-discovered fixtures with non-deterministic order.
- Profile defaults hidden in scripts instead of explicit CI configuration.
- Gating on first-failure without publishing remaining architecture diagnostics.
- Publishing only raw logs without compact machine-readable summary status.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Bad-byte profile logic | New profile parser | `verify_denulled.py --profile` | Existing profile support is robust and already aligned with byvalver terminology. |
| Functional equivalence checking | New ad-hoc shell assertions | `verify_functionality.py` | Existing script produces meaningful pass/fail checks. |
| Semantic equivalence checking | New custom disassembly comparator | `verify_semantic.py` | Existing script already codifies semantic heuristics. |
| CI result display system | External dashboard for phase gate | GitHub artifacts + step summary | Minimal operational overhead and immediate contributor visibility. |

## Common Pitfalls

### Pitfall 1: ARM profile overreach in early verification
**What goes wrong:** restrictive multi-byte profiles cause unstable ARM failures that drown signal.
**How to avoid:** keep ARM on `null-only` in Phase 2 while preserving explicit profile declarations.

### Pitfall 2: Fixture subset drift
**What goes wrong:** representative fixture set changes silently and destabilizes comparisons.
**How to avoid:** source subset from tracked metadata and fail CI when required fixture metadata/path is missing.

### Pitfall 3: Summary/artifact mismatch
**What goes wrong:** GitHub summary says pass/fail status that doesn't match uploaded artifacts.
**How to avoid:** generate a single per-arch machine-readable summary artifact and render summary rows from that source.

### Pitfall 4: Local/CI verification divergence
**What goes wrong:** local runner behavior differs from CI check ordering or profiles.
**How to avoid:** keep one verification entry path with identical mode/profile/fixture semantics.

## Code Examples

### Existing CI fail-late matrix foundation
```yaml
strategy:
  fail-fast: false
  matrix:
    arch: [x86, x64, arm]
```

### Existing profile-capable bad-byte verifier
```bash
python3 verify_denulled.py --profile http-newline <output.bin>
```

### Existing verification scripts available for integration
```bash
python3 verify_functionality.py <input.bin> <output.bin>
python3 verify_semantic.py <input.bin> <output.bin>
```

## State of the Art

| Old Approach | Current Approach | Phase 2 Direction |
|--------------|------------------|-------------------|
| Baseline smoke checks only | Architecture matrix baseline exists | Extend matrix jobs with profile/functionality/semantic verification |
| No explicit profile matrix in CI | Scripts support profiles but CI doesn't enforce policy | Make profile coverage explicit and auditable in CI |
| Logs available but not fully standardized for all checks | Baseline logs uploaded | Standardize per-check/per-arch logs plus compact summaries |

## Open Questions

1. **Manifest field for representative selection**
- Need to choose whether to encode representative membership by dedicated field (for example `ci_representative: true`) or derive from an external allowlist.
- Recommendation: use manifest-owned field to keep selection and fixture metadata co-located.

2. **Per-arch summary artifact format**
- Need to choose JSON-only vs JSON+Markdown.
- Recommendation: JSON as source of truth plus concise Markdown rendered from JSON in CI summary.

3. **Profile-to-architecture mapping placement**
- Need to decide where mapping lives (CI YAML vs runner config file).
- Recommendation: declare in repository config consumed by runner to prevent YAML duplication drift.

## Sources

### Primary (HIGH confidence)
- `/opt/byvalver/.github/workflows/ci.yml`
- `/opt/byvalver/tests/run_tests.sh`
- `/opt/byvalver/verify_denulled.py`
- `/opt/byvalver/verify_functionality.py`
- `/opt/byvalver/verify_semantic.py`
- `/opt/byvalver/tests/fixtures/manifest.yaml`
- `/opt/byvalver/.planning/ROADMAP.md`
- `/opt/byvalver/.planning/REQUIREMENTS.md`
- `/opt/byvalver/.planning/phases/02-verification-automation-and-reporting/02-CONTEXT.md`

### Secondary (MEDIUM confidence)
- `/opt/byvalver/.planning/phases/01-ci-foundation-and-fixture-canonicalization/01-RESEARCH.md`
- `/opt/byvalver/.planning/phases/01-ci-foundation-and-fixture-canonicalization/01-VERIFICATION.md`

## Metadata

**Confidence breakdown:**
- Verification stack choice: HIGH
- CI orchestration pattern: HIGH
- Deterministic subset strategy: HIGH
- Specific summary format choice: MEDIUM

**Research date:** 2026-02-25
**Valid until:** 2026-03-27
