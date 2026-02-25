# Phase 1: CI Foundation and Fixture Canonicalization - Research

**Researched:** 2026-02-25
**Domain:** Fixture taxonomy, CI baseline orchestration, and contributor parity workflow for byvalver
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Fixture taxonomy
- Top-level fixture grouping is by architecture.
- Phase 1 fixture scope is a curated core set (representative and maintainable), not full exhaustive corpus.
- Each canonical fixture has a structured metadata sidecar defining expected outcomes.
- Fixture-set changes require PR checklist/rationale updates.

### CI baseline policy
- Baseline CI runs on every PR and main-branch push.
- Baseline gate includes build + smoke matrix.
- CI runs all architecture jobs and fails at the end with the full result surface.
- Docs-only changes may bypass baseline checks via path filters.

### Contributor command UX
- Canonical local baseline entry point is one Make target.
- Default command output is concise summary with optional verbose mode.
- Local run performs preflight prerequisite checks and prints actionable install hints.
- Local baseline mirrors CI baseline checks and gates exactly.

### Claude's Discretion
- CI report presentation details: default to per-architecture summary plus artifact links.

### Deferred Ideas (OUT OF SCOPE)
None - discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| TEST-01 | CI runs build and CLI smoke checks for supported stable architectures on every PR and push. | Existing `.github/workflows/ci.yml` already runs build/smoke; phase work should expand this into an architecture matrix with fail-late behavior and deterministic fixture-backed smoke execution. |
| TEST-02 | Repository includes a categorized fixture corpus for x86, x64, and ARM samples with documented expected outcomes. | `tests/run_tests.sh` already consumes `tests/fixtures/<arch>/*.bin`, but fixture directories are missing. Canonicalization should create curated architecture folders plus metadata sidecars and ownership expectations. |
| REL-02 | Contributor workflow provides a single documented command path for local build + verification. | `make check-deps`, `make`, and `bash tests/run_tests.sh` exist today; consolidate into one make target and document as the single local CI-parity baseline entry point. |
</phase_requirements>

## Summary

The current baseline is close but incomplete for the phase goal. CI already executes on PR and main push, and local verification already has a functional runner (`tests/run_tests.sh`), but fixture taxonomy is not canonicalized in-repo (`tests/fixtures/` does not exist) and contributor parity is spread across multiple commands. The fastest path is to normalize around existing entry points rather than introducing a new framework.

For this phase, use the current Bash and Make-based workflow as the standard stack: keep `tests/run_tests.sh` as the smoke/verifier harness, use `.github/workflows/ci.yml` as the single CI baseline pipeline, and expose a one-command local entry path through Make. This avoids replacement risk and keeps behavior aligned with existing tooling and constraints.

**Primary recommendation:** Canonicalize `tests/fixtures/<arch>/` with metadata sidecars first, then wire CI and local execution to the same baseline command path.

## Standard Stack

### Core
| Library/Tool | Version | Purpose | Why Standard |
|--------------|---------|---------|--------------|
| GNU Make (`Makefile`) | Repo-defined | Build orchestration and dependency checks | Already the canonical build interface (`check-deps`, `make`, `test`) and used in CI. |
| Bash test harness (`tests/run_tests.sh`) | Repo-defined | Build/CLI smoke/fixture transformation checks | Already the current baseline test entry point and CI-integrated. |
| GitHub Actions (`.github/workflows/ci.yml`) | v4 actions | PR/push baseline automation | Existing CI system, no migration required. |
| Python verification scripts (`verify_denulled.py`, `verify_functionality.py`, `verify_semantic.py`) | Repo-defined | Post-transform correctness verification | Existing trusted validation scripts used by contributors and docs. |

### Supporting
| Library/Tool | Version | Purpose | When to Use |
|--------------|---------|---------|-------------|
| `rg` | runtime | Fast static assertions in plan verify commands | Use in automated checks to confirm docs/workflow wiring. |
| PR template (`.github/PULL_REQUEST_TEMPLATE.md`) | Repo-defined | Fixture-change governance surface | Use to enforce rationale/metadata updates when fixture set changes. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Existing Bash runner | Introduce pytest/nox harness | Adds migration risk and cross-language test rewrite cost in a stabilization phase. |
| Existing Make entry points | New custom wrapper script | Creates duplicate orchestration and drift between CI/local checks. |

**Installation baseline for CI/local parity:**
```bash
make check-deps
make
bash tests/run_tests.sh
```

## Architecture Patterns

### Recommended Project Structure
```text
tests/
├── fixtures/
│   ├── x86/
│   ├── x64/
│   ├── arm/
│   ├── README.md
│   └── manifest.yaml
└── run_tests.sh
```

### Pattern 1: Architecture-First Canonical Fixture Taxonomy
**What:** Keep fixture roots grouped by architecture (`x86`, `x64`, `arm`) with curated representative bins and explicit metadata expectations.
**When to use:** For all baseline smoke/fixture checks in phase 1 and any future CI fixture expansion.
**Example:**
```bash
# Source: tests/run_tests.sh
for arch_dir in "$FIXTURES"/*/; do
  arch=$(basename "$arch_dir")
  for input in "$arch_dir"/*.bin; do
    "$BIN" --arch "$arch" "$input" "$output"
  done
done
```

### Pattern 2: Single Baseline Command Path for CI and Local
**What:** CI and contributors run the same chained baseline steps (`check-deps -> build -> smoke/fixtures`) through one make target.
**When to use:** Always for phase 1 baseline checks and onboarding docs.
**Example:**
```makefile
# Source: Makefile + tests/run_tests.sh (pattern to preserve)
ci-baseline:
	@$(MAKE) check-deps
	@$(MAKE)
	@bash tests/run_tests.sh --mode baseline --arch all
```

### Pattern 3: Fail-Late Multi-Architecture CI Surface
**What:** Execute each architecture baseline in CI without early matrix cancellation; surface complete architecture outcomes per run.
**When to use:** PR and main push baseline checks.
**Example:**
```yaml
# Source: .github/workflows/ci.yml (to evolve)
strategy:
  fail-fast: false
  matrix:
    arch: [x86, x64, arm]
```

### Anti-Patterns to Avoid
- **Ad-hoc fixture placement under `assets/tests/` for baseline CI:** breaks deterministic curated corpus ownership.
- **Separate CI-only scripts:** causes local/CI drift and undermines REL-02.
- **Early-fail matrix with no full result surface:** conflicts with locked decision to show all architecture outcomes.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Bad-byte output verification | New bespoke byte scanner | `verify_denulled.py` | Existing script is already wired and documented. |
| Build dependency detection | New shell probe script | `make check-deps` | Existing dependency contract should remain single source of truth. |
| Fixture execution flow | New independent runner | `tests/run_tests.sh` extended minimally | Preserves current behavior while enabling canonical fixture policy. |
| CI orchestration | Additional CI workflow for same baseline | Existing `ci.yml` expanded with matrix/path filters | Avoids duplicate gates and inconsistent enforcement. |

**Key insight:** phase 1 should standardize and wire existing mechanisms, not replace them.

## Common Pitfalls

### Pitfall 1: Fixture Taxonomy Exists in Docs but Not in Runner Inputs
**What goes wrong:** docs describe canonical fixtures, but `tests/run_tests.sh` still resolves unexpected or missing paths.
**Why it happens:** taxonomy and runner evolve separately.
**How to avoid:** couple taxonomy docs/manifest and runner path logic in the same plan; verify with fixture-path assertions.
**Warning signs:** test suite prints "no fixture binaries found in tests/fixtures" in CI.

### Pitfall 2: Local and CI Baselines Drift
**What goes wrong:** contributors run one command set while CI runs another (extra flags/steps differ).
**Why it happens:** multiple entry points with no single make target.
**How to avoid:** define one canonical Make target and reuse it in docs and CI.
**Warning signs:** onboarding docs differ from workflow commands in `.github/workflows/ci.yml`.

### Pitfall 3: Docs-Only Path Filter Becomes Over-Broad
**What goes wrong:** CI skips baseline checks for changes that actually affect execution (e.g., scripts under `tests/`).
**Why it happens:** path filters are not constrained tightly to docs-only file globs.
**How to avoid:** include explicit include/exclude patterns and validate with a small PR matrix of path-only changes.
**Warning signs:** workflow not triggered for non-doc baseline file edits.

### Pitfall 4: Architecture Label Mismatch
**What goes wrong:** fixture directory names and `--arch` values diverge (`x64` vs `amd64`, etc.).
**Why it happens:** ungoverned fixture additions.
**How to avoid:** lock architecture IDs to CLI-supported names and validate manifest entries against runner arch args.
**Warning signs:** transformations fail before processing with arch parsing errors.

## Code Examples

Verified patterns from repository sources:

### Baseline CI Build + Smoke Chain
```yaml
# Source: .github/workflows/ci.yml
- name: Check dependencies
  run: make check-deps

- name: Build
  run: make

- name: Run test suite
  run: bash tests/run_tests.sh
```

### Architecture-Scoped Fixture Loop
```bash
# Source: tests/run_tests.sh
for arch_dir in "$FIXTURES"/*/; do
    arch=$(basename "$arch_dir")
    for input in "$arch_dir"/*.bin; do
        "$BIN" --arch "$arch" "$input" "$output"
    done
done
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Fixture assets mostly under `assets/tests/` without canonical baseline corpus in `tests/fixtures/` | Canonical curated fixture corpus under `tests/fixtures/<arch>/` with metadata sidecars and ownership docs | Phase 1 target | Deterministic baseline coverage and contributor discoverability |
| CI single job with full-suite invocation | CI matrix by architecture with fail-fast disabled and full result surface | Phase 1 target | Better signal isolation and complete per-arch outcomes |
| Multi-command local baseline (`make`, then script manually) | One documented make target for CI-parity baseline | Phase 1 target | Reduces onboarding friction and REL-02 risk |

**Deprecated/outdated for phase baseline:**
- Using uncatalogued fixture blobs as default CI corpus source.
- Treating PR template fixture expectations as optional/untracked.

## Open Questions

1. **Curated fixture cardinality per architecture**
   - What we know: phase decision requires a curated core set, not exhaustive corpus.
   - What's unclear: exact minimum fixture count per arch for stable signal/noise.
   - Recommendation: set an initial minimum in fixture README and tighten in phase 2 after CI runtime data.

2. **Artifact format for sidecar metadata**
   - What we know: metadata sidecar is mandatory per canonical fixture.
   - What's unclear: per-file sidecars (`*.yaml`) vs central manifest (`manifest.yaml`) as source of truth.
   - Recommendation: adopt central `manifest.yaml` plus optional per-file sidecars only when fixture-specific notes exceed manifest ergonomics.

## Sources

### Primary (HIGH confidence)
- `.github/workflows/ci.yml` - existing CI trigger/build/smoke path
- `tests/run_tests.sh` - current baseline test runner semantics and fixture loop
- `tests/README.md` - expected fixture topology and runner contract
- `Makefile` - dependency/build command contract for baseline entry points
- `.planning/ROADMAP.md` - phase goal, requirements, and success criteria
- `.planning/REQUIREMENTS.md` - requirement definitions (TEST-01, TEST-02, REL-02)
- `.planning/phases/01-ci-foundation-and-fixture-canonicalization/01-CONTEXT.md` - locked implementation decisions

### Secondary (MEDIUM confidence)
- `.planning/codebase/STACK.md` - current tooling baseline and CI/testing stack summary
- `.planning/codebase/TESTING.md` - runner organization and testing pattern summary

### Tertiary (LOW confidence)
- None

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - grounded in in-repo workflow/scripts and no external dependency changes.
- Architecture patterns: HIGH - directly inferred from current runner/CI contracts and locked context decisions.
- Pitfalls: HIGH - based on current observable gaps (`tests/fixtures` absent, CI/local split commands) and known workflow failure modes.

**Research date:** 2026-02-25
**Valid until:** 2026-03-27
