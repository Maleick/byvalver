# byvalver

## Current State

- Latest shipped milestone: `v4.4 Equivalence Reliability` (2026-02-26).
- Delivered in v4.4: deterministic equivalence/release-gate reliability foundation (Phase 6).
- v4.4 closed with accepted gaps:
  - `ARM64-01`, `ARM64-02`, `ARM64-03`
  - `ARCH-01`, `ARCH-02`, `ARCH-03`
- Release policy remains strict: required parity/equivalence mismatches are release-blocking.

## Next Milestone Goals

- Deliver deferred ARM64 core strategy expansion from v4.4.
- Deliver architecture-assist diagnostics and operator guidance flows.
- Keep deterministic release-gate guarantees while expanding architecture coverage.

## What This Is

byvalver is a C-based CLI that rewrites shellcode to remove forbidden bad bytes
while preserving behavior across x86/x64 and experimental ARM/ARM64 targets.

## Core Value

Given an input payload and bad-byte policy, byvalver must produce functionally
equivalent shellcode that is verifiably free of forbidden bytes.

## Constraints

- **Compatibility:** Preserve current CLI flags and expected behavior.
- **Correctness:** Functional equivalence and bad-byte elimination are non-negotiable.
- **Performance:** Avoid material regression in core transformation throughput.
- **Security:** Do not place secrets in planning/docs artifacts.

## Key Decisions

| Decision | Rationale | Outcome |
|---|---|---|
| Keep C/Capstone core architecture | Existing transformation engine is mature and proven | Good |
| Enforce host-vs-Docker parity as release contract | Operational confidence depends on equivalent verification outcomes | Good |
| Close v4.4 after Phase 6 with explicit accepted gaps | User-directed tradeoff to preserve delivery momentum | Accepted debt |

<details>
<summary>Previous v4.4 kickoff context</summary>

- Milestone originally scoped to Phases 6-8.
- Phase 6 delivered deterministic equivalence gate behavior and tuple-based triage.
- Phase 7/8 were deferred at closeout and should be re-planned in the next milestone.

</details>

---
*Last updated: 2026-02-26 after v4.4 milestone closeout*
