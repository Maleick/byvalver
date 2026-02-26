# Phase 3: ARM Strategy Expansion - Context

**Gathered:** 2026-02-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Implement missing high-impact ARM rewrite coverage for immediate arithmetic, load/store displacement handling, and conditional alternatives that improve transformation success rates while keeping runtime behavior intact.

Scope is fixed to roadmap plans `03-01`, `03-02`, and `03-03` only.

</domain>

<decisions>
## Implementation Decisions

### Scope and sequencing policy
- Phase 3 must remain conservative-core and follow roadmap decomposition only (`03-01`, `03-02`, `03-03`).
- `03-01` focuses on ADD/SUB immediate rewrite paths with semantic safety first.
- `03-02` focuses on LDR/STR displacement rewrite paths with addressing-behavior preservation.
- `03-03` focuses on branch-first conditional alternatives only.
- No capability expansion beyond these three tracks is allowed in this phase.

### Conditional strategy boundary
- Conditional coverage in this phase is branch-first only (conditional branch alternatives).
- Predicated ALU rewrites (`MOV<cond>`, `ADD<cond>`, `SUB<cond>`) are deferred.
- Conditional memory and compare/test sequence rewrites are deferred.
- Phase 3 deliverables must explicitly document deferred conditional families.

### Regression fixture policy
- Each changed rewrite family must add deterministic ARM regression fixtures.
- Fixture metadata must be tracked in `tests/fixtures/manifest.yaml` with stable fixture IDs.
- New fixtures should be representative and minimal while still catching semantic regressions.
- CI-facing verification must stay deterministic and reproducible for ARM checks.

### Semantic-safety policy
- Rewrite candidates must preserve original instruction semantics for covered operand patterns.
- Plans must define explicit supported operand/addressing subsets and deterministic fallback behavior.
- Unsupported patterns should fail safely (decline rewrite) rather than emit risky transformations.
- ARM64 is out of scope for this phase and must not be pulled into implementation work.

### Claude's Discretion
- Exact fixture binary contents and naming conventions within the deterministic manifest policy.
- Internal helper decomposition and file-level implementation layout across ARM strategy modules.
- Specific test harness split between focused ARM strategy tests and existing verification scripts.

</decisions>

<specifics>
## Specific Ideas

- Keep Phase 3 as a correctness-first expansion: fewer rewrite families, higher confidence.
- Prefer deterministic fixture-driven checks that can be reused by later ARM safety-net phases.
- Keep `03-03` limited to conditional branch alternatives to avoid hidden branch-offset risk expansion in this phase.

</specifics>

<deferred>
## Deferred Ideas

- Predicated ALU conditional alternatives and conditional-memory rewrite families.
- Broader conditional transformation coverage beyond branch-first alternatives.
- Any ARM64 strategy breadth work.

</deferred>

---

*Phase: 03-arm-strategy-expansion*
*Context gathered: 2026-02-25*
