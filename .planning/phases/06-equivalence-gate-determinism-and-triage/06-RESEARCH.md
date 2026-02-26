# Phase 6: Equivalence Gate Determinism and Triage - Research

**Researched:** 2026-02-26  
**Phase:** 06-equivalence-gate-determinism-and-triage  
**Requirements:** REL-04, REL-05, REL-06

## User Constraints

No `06-CONTEXT.md` exists for this phase.

Locked constraints inherited from milestone/project artifacts:
- Release-gate parity mismatches remain hard-fail conditions.
- Representative fixture scope remains manifest-driven and deterministic.
- Local and CI gate command paths must remain behaviorally equivalent.

## Objective

Define a low-risk plan to make representative equivalence outcomes deterministic, improve failure diagnosability, and align local/CI gate semantics so release decisions are reproducible and auditable.

## Current Baseline (Repository Findings)

- `tests/run_tests.sh` already supports `verify-equivalence`, `verify-parity`, and `release-gate` modes.
- `release-gate` workflow exists and executes `tests/run_tests.sh --mode release-gate --arch all`.
- Representative fixture selection already relies on `tests/fixtures/manifest.yaml` + `ci_representative: true`.
- Equivalence verification path still has historical instability/failure signals at representative scope (known blocker in state/project docs).
- Artifact paths exist, but failure diagnosis still requires manual stitching between logs, fixture IDs, and rerun commands.

## Gap Analysis vs Phase 6 Requirements

### REL-04 (deterministic equivalence outcomes)

**Gap:** Representative equivalence checks are not yet trustworthy as stable release inputs.

**Implication:** Release-gate status can be blocked without fast confidence on whether the issue is deterministic regression, harness variance, or architecture mismatch noise.

### REL-05 (actionable release-gate artifacts)

**Gap:** Failures are logged but not consistently surfaced as a single tuple: `{arch, fixture_id, check, status, rerun}`.

**Implication:** Maintainers spend extra cycles reconstructing failures before they can debug.

### REL-06 (local/CI gate semantic parity)

**Gap:** Local and CI both invoke release gate, but required-check scope, artifact expectations, and summary interpretation can drift if not explicitly codified.

**Implication:** Local green does not always guarantee CI green (or vice versa) with the same diagnostic context.

## Standard Stack (Phase-Specific)

- Core execution: `bash tests/run_tests.sh`
- Verification engines: `verify_functionality.py`, `verify_semantic.py`, `verify_denulled.py`
- Gate orchestrators: `Makefile` (`release-gate`) and `.github/workflows/release-gate.yml`
- Deterministic fixture metadata: `tests/fixtures/manifest.yaml`

## Architecture Patterns to Apply

1. **Deterministic tuple-first reporting**
- Every failing required check must emit canonical fields (arch, fixture_id, check, status, message, rerun command).

2. **Single source of required-check truth**
- Required check list and decision semantics should be defined once in runner logic and consumed by both local and CI paths.

3. **Fail-late, fail-closed release reporting**
- Run full representative matrix for visibility, then fail release gate if any required tuple is non-pass.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Ad hoc release summaries | Free-form log grep pipelines | Stable JSON summary schema + deterministic markdown renderer | Keeps triage tooling reliable across edits |
| Per-environment gate policy | Separate local vs CI check lists | Shared runner mode with explicit required checks | Prevents semantic drift |
| Manual fixture selection | Hardcoded per-arch arrays in scripts | Manifest representative metadata | Keeps fixture governance centralized |

## Common Pitfalls

1. **Architecture fallback masking root cause**
- Wrong-arch handling can look like equivalence failure if triage metadata is incomplete.
- Mitigation: include assist context and rerun command in failure tuple.

2. **Changing fixture scope while fixing determinism**
- Quietly swapping representatives can hide regressions.
- Mitigation: treat manifest scope changes as explicit task output with rationale.

3. **Overcoupling parity status to log text**
- Text-only parsing breaks easily.
- Mitigation: bind parity and gate decisions to summary JSON fields.

## Code Examples (Project-Local)

### Deterministic representative selection pattern
- `tests/run_tests.sh` -> `manifest_representatives_for_arch`
- `tests/fixtures/manifest.yaml` -> `ci_representative: true`

### Gate orchestration pattern
- `Makefile` -> `release-gate` target invoking `tests/run_tests.sh --mode release-gate`
- `.github/workflows/release-gate.yml` -> same runner mode with artifact upload

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | shell runner + Python verification scripts |
| Config file | `tests/run_tests.sh` + manifest metadata |
| Quick run command | `bash tests/run_tests.sh --mode verify-equivalence --arch all --artifacts-dir ci-artifacts/phase6-quick` |
| Full suite command | `make release-gate` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| REL-04 | deterministic equivalence outcomes | runner regression | `bash tests/run_tests.sh --mode verify-equivalence --arch all --artifacts-dir ci-artifacts/phase6-det` | yes |
| REL-05 | actionable failing tuples + rerun guidance | artifact contract | `python3 -m json.tool ci-artifacts/release-gate/summary-x86-verify-equivalence.json` | yes |
| REL-06 | local/CI parity in required checks | gate parity | `make release-gate` and workflow dry-run parity review | yes |

### Sampling Rate
- **Per task commit:** `bash tests/run_tests.sh --mode verify-equivalence --arch <target>`
- **Per plan completion:** `bash tests/run_tests.sh --mode verify-parity --arch all --artifacts-dir ci-artifacts/phase6-parity`
- **Phase gate:** `make release-gate`

## Plan Decomposition Recommendation

- **06-01 (Wave 1):** Stabilize representative equivalence harness inputs, ordering, and status semantics (REL-04 foundation).
- **06-02 (Wave 2):** Standardize release-gate summary schema and deterministic rerun guidance outputs (REL-05).
- **06-03 (Wave 3):** Resolve blocker paths and lock local/CI required-check parity behavior (REL-06 + REL-04 closeout).

## Open Questions

1. Is current representative failure root cause primarily transformation regression or verifier invocation mismatch per architecture?
- Recommendation: include explicit baseline classification artifact in 06-01 before remediation edits.

2. Should phase 6 introduce new representative fixtures?
- Recommendation: only if current representatives are invalid for stated equivalence goals; document every scope change in manifest and summary.

## Confidence

- **Overall:** MEDIUM
- **Why:** Existing gate/runner infrastructure is mature, but deterministic blocker root-cause details still require implementation-time validation.

---
*Phase: 06-equivalence-gate-determinism-and-triage*  
*Research completed: 2026-02-26*
