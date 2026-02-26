# Milestones

## v4.3 stabilization (Shipped: 2026-02-25)

**Phases completed:** 5 phases, 13 plans, 39 tasks

**Key accomplishments:**
- Standardized fixture governance and contributor CI baseline parity command paths.
- Automated bad-byte plus equivalence verification with deterministic per-architecture artifacts.
- Expanded ARM rewrite strategy coverage (ADD/SUB immediates, LDR/STR displacement, conditional alternatives).
- Added ARM branch-safety and architecture mismatch diagnostics with actionable warn-and-continue guidance.
- Enforced a fail-closed reproducible release gate across local and tag-triggered CI workflows.

**Git range:** `e9ec2cf` -> `e9b4122`
**Scope note:** No `v4.3-MILESTONE-AUDIT.md` was present at closeout.

---

## v4.4 Equivalence Reliability (Shipped: 2026-02-26)

**Phases delivered:** 1 of 3 milestone phases (3/8 plans, 9 tasks)
**Closure mode:** Shipped with accepted gaps

**Key accomplishments:**
- Stabilized representative fixture selection and tuple schema output for deterministic equivalence verification.
- Added tuple-first release diagnostics with deterministic rerun metadata in local and CI artifact paths.
- Aligned local and CI release-gate execution around one required-check contract.
- Resolved host/Docker parity instability for representative release-gate checks.
- Completed and verified Phase 6 requirement set (`REL-04`, `REL-05`, `REL-06`).

**Known gaps accepted at closeout:**
- `ARM64-01`, `ARM64-02`, `ARM64-03` (Phase 7 not executed)
- `ARCH-01`, `ARCH-02`, `ARCH-03` (Phase 8 not executed)

**Git range:** `6d4f321` -> `112c744`
**Audit:** `v4.4-MILESTONE-AUDIT.md` status `gaps_found` (accepted by user decision)

---
