# Phase 1: CI Foundation and Fixture Canonicalization - Context

**Gathered:** 2026-02-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver canonical fixture organization and baseline CI entry points for contributor parity.

</domain>

<decisions>
## Implementation Decisions

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

</decisions>

<specifics>
## Specific Ideas

- Keep contributor loop fast but deterministic.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 01-ci-foundation-and-fixture-canonicalization*
*Context gathered: 2026-02-25*
