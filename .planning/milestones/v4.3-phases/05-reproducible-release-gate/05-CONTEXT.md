# Phase 5: Reproducible Release Gate - Context

**Gathered:** 2026-02-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver reproducible release gating for the existing transformation and verification workflow.
Scope is fixed to roadmap plans `05-01` and `05-02` only:
- `05-01`: Docker/host reproducibility parity flow with deterministic fixtures.
- `05-02`: release checklist plus enforced gate checks before tagging.

</domain>

<decisions>
## Implementation Decisions

### Reproducibility contract (`REL-01`)
- Reproducibility is defined as **verification outcome parity** between host and Docker runs.
- Byte-identical transformed binaries are not required in this phase.
- Required parity checks are bad-byte verification and equivalence verification outcomes.
- Any required parity mismatch is a release-blocking failure for Phase 5 gate paths.

### Fixture and evidence policy
- Parity dataset defaults to deterministic representative fixtures from `tests/fixtures/manifest.yaml` (`ci_representative: true`).
- Evidence artifacts must be stable and comparable across host and Docker runs.
- Artifact naming should be deterministic and architecture-aware for triage.

### Release gate enforcement (`REL-03`)
- Gate checks are enforced in CI and mirrored via a local maintainer command path.
- CI release gate trigger model is version-tag push plus `workflow_dispatch` for dry-run/preview.
- Required checks must fail closed for release gating; warnings alone are insufficient for parity failures.

### Scope guardrails
- No expansion into broader release automation beyond REL-01/REL-03.
- No new runtime feature scope outside reproducibility and gate enforcement.
- No change to product API/schema/type contracts in this phase.

### Claude's Discretion
- Exact workflow file split between CI workflow file(s) and script wrapper(s).
- Exact artifact summary formatting and table/JSON presentation details.
- Minor command naming details as long as one canonical local gate path is preserved.

</decisions>

<specifics>
## Specific Ideas

- Keep parity checks deterministic by using manifest representatives instead of full fixture corpus.
- Ensure maintainers can run the same gate logic locally before tagging.
- Make release-gate failure reasons explicit (which check group diverged and where artifacts are located).

</specifics>

<deferred>
## Deferred Ideas

- Byte-for-byte reproducible artifact guarantees.
- Full provenance/signing/attestation pipeline (SLSA-style release hardening).
- Expanded release orchestration beyond verification gate enforcement.

</deferred>

---

*Phase: 05-reproducible-release-gate*
*Context gathered: 2026-02-25*
