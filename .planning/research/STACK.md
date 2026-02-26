# Stack Research

**Domain:** Shellcode transformation reliability and release-gate enforcement
**Researched:** 2026-02-26
**Confidence:** MEDIUM

## Recommended Stack

### Core Technologies

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| C (GCC/Clang toolchain) | C99 + modern compiler | Core transformation engine and strategy execution | Existing production code is C-first and performance sensitive; extending this avoids migration risk |
| Capstone | 5.x (pkg-config `capstone`) | Multi-arch disassembly and instruction classification | Already embedded in the transform pipeline; required for architecture-aware rewrite decisions |
| Python | 3.10+ | Verification orchestration (`verify_denulled.py`, `verify_functionality.py`, `verify_semantic.py`) | Current gate semantics depend on Python verification scripts and JSON evidence outputs |

### Supporting Libraries

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Docker Engine | 24+ | Host-vs-container parity execution | Required for `verify-parity` and release-gate reproducibility checks |
| Docker Compose | v2 | Multi-service gate execution (`verify`, `parity`) | Required for local parity mirroring of CI release gate |
| GitHub Actions | Current hosted runners | Tag-triggered and manual gate enforcement | Required for release tag gating and artifact publication |

### Development Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| `make` | Canonical build and gate entrypoints | Keep `ci-baseline` and `release-gate` as source-of-truth operator commands |
| `nasm`, `xxd`, `objdump` | Build and low-level verification support | Must stay in dependency preflight to keep local/CI parity deterministic |
| `rg` + artifact JSON summaries | Fast triage and deterministic diagnostics | Keep summary file naming stable for report parsing |

## Installation

```bash
# Core build/verification deps (Debian/Ubuntu example)
sudo apt-get install -y build-essential pkg-config libcapstone-dev nasm xxd python3 binutils docker.io docker-compose-plugin

# macOS (Homebrew example)
brew install capstone nasm pkg-config python binutils docker
```

## Alternatives Considered

| Recommended | Alternative | When to Use Alternative |
|-------------|-------------|-------------------------|
| Capstone-backed decode + C strategies | Full LLVM/MC rewrite backend | Only if instruction-level transformations require richer encode/decode primitives than current strategy model |
| Python verification scripts | Single compiled verifier binary | Consider only after verification behavior is stable and equivalent across all arches |
| Docker parity gate | VM-only reproducibility checks | Use only where Docker is unavailable and VM images are operationally required |

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| Non-deterministic fixture sampling in gates | Causes flaky release results and weak failure triage | Manifest representatives (`ci_representative: true`) with fixed ordering |
| Byte-identical host-vs-docker release criteria | Over-constrains toolchain differences without improving correctness confidence | Outcome parity on required verification checks |
| Silent architecture auto-switch in mutation path | Can transform under wrong assumptions and hide user intent errors | Warn-and-continue with explicit architecture recommendation |

## Stack Patterns by Variant

**If working on release-gate reliability:**
- Use host + Docker dual execution with shared summary schema.
- Keep failure policy fail-closed on required check mismatches.

**If working on ARM64 strategy expansion:**
- Use safe-decline patterns and deterministic fixture coverage before broad strategy activation.
- Gate new strategy families with targeted cross-arch regression tests.

## Version Compatibility

| Package A | Compatible With | Notes |
|-----------|-----------------|-------|
| `capstone` (pkg-config) | GCC/Clang C99 build | Keep include path normalization in Makefile (`/include` vs `/include/capstone`) |
| Python verification scripts | CPython 3.10+ | Maintain argparse and JSON output stability for CI parsers |
| Docker Compose v2 | release-gate workflow and local target | Keep mount-free container execution for consistent `/opt` workspace behavior |

## Sources

- `/opt/byvalver/Makefile` - compiler/dependency/toolchain contract
- `/opt/byvalver/tests/run_tests.sh` - gate modes and artifact schema
- `/opt/byvalver/docs/BUILD.md` - parity and release-gate operator flows
- `/opt/byvalver/.planning/phases/05-reproducible-release-gate/05-0*-SUMMARY.md` - known parity and gate constraints

---
*Stack research for: byvalver v4.4 equivalence reliability*
*Researched: 2026-02-26*
