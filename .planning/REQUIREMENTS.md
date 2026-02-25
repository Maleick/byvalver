# Requirements: byvalver

**Defined:** 2026-02-25
**Core Value:** Given an input payload and bad-byte policy, byvalver must produce functionally equivalent shellcode that is verifiably free of forbidden bytes.

## v1 Requirements

Requirements for the next stabilization release (v4.3 track).

### CI and Fixture Infrastructure

- [x] **TEST-01**: CI runs build and CLI smoke checks for supported stable architectures on every PR and push.
- [x] **TEST-02**: Repository includes a categorized fixture corpus for x86, x64, and ARM samples with documented expected outcomes.
- [ ] **TEST-03**: CI executes bad-byte verification for configured byte profiles using `verify_denulled.py` across representative fixtures.
- [ ] **TEST-04**: CI executes semantic/functional verification using `verify_functionality.py` and `verify_semantic.py` for representative fixtures.
- [ ] **TEST-05**: CI publishes a per-architecture pass/fail summary artifact that contributors can inspect after each run.

### ARM Transformation Maturity

- [ ] **ARM-01**: ARM ADD/SUB immediate rewrite paths handle bad-byte avoidance without breaking instruction semantics.
- [ ] **ARM-02**: ARM LDR/STR displacement rewrite paths preserve addressing behavior after transformation.
- [ ] **ARM-03**: ARM branch offset recalculation remains correct after instruction expansion and relocation.
- [ ] **ARM-04**: ARM conditional-instruction alternatives are implemented for common transformation cases.
- [ ] **ARM-05**: Architecture mismatch heuristics surface actionable warnings when input likely targets a different architecture.
- [ ] **ARM-06**: ARM execution path emits explicit experimental-mode guidance and fallback recommendations.

### Release and Reproducibility

- [ ] **REL-01**: Project ships a reproducible Docker workflow that runs `byvalver` end-to-end against sample inputs.
- [x] **REL-02**: Contributor workflow provides a single documented command path for local build + verification.
- [ ] **REL-03**: Release checklist enforces bad-byte and functional-equivalence verification before tagging.

## v2 Requirements

Deferred to later milestones.

### Future Expansion

- **ARM64-01**: ARM64 strategy coverage expands to beta-level breadth for common AArch64 patterns.
- **ARCH-01**: Automatic architecture detection reduces dependence on manual `--arch` selection.
- **ML-01**: ML model retraining supports generic multi-byte bad-byte elimination scenarios.
- **EXT-01**: Plugin architecture supports third-party strategy packs.

## Out of Scope

Explicitly excluded from v4.3 stabilization scope.

| Feature | Reason |
|---------|--------|
| MIPS support | Reserved for later milestone after ARM/ARM64 maturity improves |
| Full TUI redesign | Current interface is functional; focus is correctness/test hardening |
| Rewriting core engine out of C | High risk with no direct v4.3 value |

## Traceability

Which phases cover which requirements.

| Requirement | Phase | Status |
|-------------|-------|--------|
| TEST-01 | Phase 1 | Complete |
| TEST-02 | Phase 1 | Complete |
| TEST-03 | Phase 2 | Pending |
| TEST-04 | Phase 2 | Pending |
| TEST-05 | Phase 2 | Pending |
| ARM-01 | Phase 3 | Pending |
| ARM-02 | Phase 3 | Pending |
| ARM-03 | Phase 4 | Pending |
| ARM-04 | Phase 3 | Pending |
| ARM-05 | Phase 4 | Pending |
| ARM-06 | Phase 4 | Pending |
| REL-01 | Phase 5 | Pending |
| REL-02 | Phase 1 | Complete |
| REL-03 | Phase 5 | Pending |

**Coverage:**
- v1 requirements: 14 total
- Mapped to phases: 14
- Unmapped: 0 ✓

---
*Requirements defined: 2026-02-25*
*Last updated: 2026-02-25 after roadmap creation*
