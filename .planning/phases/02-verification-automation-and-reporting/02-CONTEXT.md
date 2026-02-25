# Phase 2: Verification Automation and Reporting - Context

**Gathered:** 2026-02-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Add objective CI verification evidence for bad-byte safety and semantic/functional equivalence, and publish per-architecture artifacts that make failures diagnosable without local reruns.

</domain>

<decisions>
## Implementation Decisions

### Bad-byte profile coverage policy
- Phase 2 verification must use `verify_denulled.py` with explicit profile configuration (not implicit defaults).
- Required profile set for CI evidence is `null-only` and `http-newline`.
- `null-only` runs for all supported CI architectures (`x86`, `x64`, `arm`).
- `http-newline` runs for `x86` and `x64` in this phase; ARM remains `null-only` for Phase 2 due current ARM maturity.
- Profile coverage must be visible in CI output and artifacts (profile name included in filenames and summary rows).

### Representative fixture selection policy
- Verification runs on a deterministic, curated representative subset (not full corpus).
- Selection source of truth is repository-tracked metadata/manifest data, not ad-hoc glob order.
- Fixture ordering in CI must be stable and deterministic across runs.
- If a selected fixture is missing or invalid, CI should fail explicitly with fixture ID/path context.
- Subset should keep runtime practical while covering each architecture and at least one profile-sensitive sample.

### Verification runner behavior policy
- CI verification stage must execute all configured checks for each architecture and surface full results before failing (fail-late result visibility).
- Verification checks include: bad-byte profile validation, functionality validation, and semantic validation.
- Each check must return machine-parseable pass/fail status and retain raw logs for debugging.
- Local invocation path should remain scriptable and mirror CI verification gate logic.

### Reporting and artifact contract
- Per-architecture artifact directory structure must be consistent and predictable.
- Artifact naming must include architecture and check type (and profile where applicable).
- Each architecture publishes a compact summary artifact plus detailed raw logs.
- GitHub step summary should include per-architecture pass/fail rows with artifact references.
- Aggregate summary job should report final matrix verdict and keep all failed-arch artifacts available.

### Claude's Discretion
- Exact report file format (JSON/Markdown/plain text) as long as it is deterministic and human-readable.
- Internal helper script decomposition between CI YAML and shell/Python wrappers.
- Minor optimization of fixture subset size if runtime pressure appears, while preserving coverage policy above.

</decisions>

<specifics>
## Specific Ideas

- Prefer a single verification entry script that emits structured summary files consumed by CI summary steps.
- Keep artifact naming stable for copy/paste triage (for example, `verify-<arch>-<check>.log`, `summary-<arch>.json`).
- Fail-late behavior should preserve diagnostics breadth first, then enforce gate failure.

</specifics>

<deferred>
## Deferred Ideas

- Expanding profile set beyond `null-only` and `http-newline` (for example `sql-injection`, `alphanumeric-only`) to later hardening phases.
- Full exhaustive fixture corpus verification on every PR.
- Rich dashboard/UI visualization of verification results.

</deferred>

---

*Phase: 02-verification-automation-and-reporting*
*Context gathered: 2026-02-25*
