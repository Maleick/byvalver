# Phase 4: ARM Diagnostic Safety Nets - Context

**Gathered:** 2026-02-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Improve ARM transformation safety by hardening branch offset correctness under instruction expansion and by adding actionable architecture/experimental diagnostics before destructive processing.

Scope is fixed to roadmap plans `04-01` and `04-02` only.

</domain>

<decisions>
## Implementation Decisions

### Branch offset safety scope (`04-01`)
- Phase 4 must harden ARM branch offset correctness specifically for rewritten/expanded branch paths.
- Coverage must include forward and backward branch offsets plus near-boundary displacement cases.
- If safe branch offset recomputation cannot be guaranteed for a case, logic must decline rewrite safely.
- Regression evidence must be deterministic and fixture-driven.

### Architecture mismatch diagnostics scope (`04-02`)
- Add pre-transform heuristics for likely architecture mismatch and surface actionable warnings before destructive processing.
- Warning output should include reason hints (for example, likely different architecture pattern detected) and next-step guidance.
- Default behavior is **warn-and-continue**; this phase does not introduce fail-by-default mismatch blocking.

### ARM experimental guidance policy
- ARM/ARM64 execution path messaging must clearly describe experimental limits and recommended fallback paths.
- Guidance should include concrete operator actions (`--arch` confirmation, `--dry-run`, verification command path).
- Diagnostic messaging should be consistent between CLI help text and runtime warnings.

### Scope guardrails
- No expansion into new ARM rewrite strategy families beyond branch correctness safety nets.
- No broad architecture auto-detection feature rollout in this phase.
- No hard-fail default mismatch policy in this phase; strict/blocking behavior is deferred.

### Claude's Discretion
- Exact heuristic scoring/threshold details for mismatch warnings.
- Specific fixture binary composition for branch edge-case coverage.
- Minor warning text phrasing and where to place helper decomposition across `core.c`, `main.c`, and `cli.c`.

</decisions>

<specifics>
## Specific Ideas

- Keep diagnostics actionable and short: "what was detected", "why it matters", "what to run next".
- Prefer deterministic branch fixture IDs in `tests/fixtures/manifest.yaml` for relocation edge-case regression tracking.
- Keep warn-and-continue as the operational default to avoid unexpected breakage in existing workflows.

</specifics>

<deferred>
## Deferred Ideas

- Strict/fail-closed mismatch mode as default behavior.
- Full multi-architecture auto-detection with confidence scoring.
- Broader ARM rewrite-family expansion outside branch safety nets.

</deferred>

---

*Phase: 04-arm-diagnostic-safety-nets*
*Context gathered: 2026-02-25*
