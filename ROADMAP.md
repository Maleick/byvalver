# byvalver Roadmap

Public roadmap for feature development and maturity graduation.

## Feature Maturity Levels

| Level | Label | Meaning |
|-------|-------|---------|
| 0 | **Planned** | Design phase, not yet implemented |
| 1 | **Experimental** | Initial implementation, limited testing, API may change |
| 2 | **Beta** | Functional with known limitations, feedback welcome |
| 3 | **Stable** | Production-ready, fully tested, backwards-compatible |

## Current Feature Maturity (v4.2)

| Feature | Maturity | Notes |
|---------|----------|-------|
| x86 denullification | Stable v4.2 | 150+ strategies, 100% success on test corpus |
| x64 denullification | Stable v4.2 | 150+ strategies with x86 compat layer, REX prefix support |
| Bad-byte profiles | Stable v3.0 | 13 pre-configured profiles for common exploit contexts |
| Biphasic processing | Stable v3.0 | Obfuscation (Pass 1) + Denullification (Pass 2) |
| XOR encoding | Stable v2.0 | JMP-CALL-POP decoder stub generation |
| Batch processing | Stable v3.0 | Recursive directory processing with statistics |
| Interactive TUI | Stable v3.0 | Full CLI parity with ncurses interface |
| Output formats | Stable v2.0 | Raw, C, Python, PowerShell, hex string |
| ML strategy selection | Beta v2.0 | 336-dim neural network, trained on null-byte elimination only |
| ARM support | Experimental v0.1 | 7 core strategies (MOV, arithmetic, loads/stores) |
| ARM64 support | Experimental v0.1 | Framework ready, basic strategies only |

## v4.3 (Next Release)

**Focus**: Testing infrastructure and ARM expansion

- [ ] CI/CD pipeline with GitHub Actions
- [ ] Categorized test fixture corpus (x86, x64, ARM)
- [ ] Automated verification suite in CI
- [ ] ARM strategy expansion to 20+ strategies
  - ADD/SUB immediate splitting
  - LDR/STR displacement transformations
  - Branch offset recalculation
  - Conditional instruction alternatives
- [ ] ARM architecture mismatch detection improvements
- [ ] Docker container for reproducible builds

## v5.0

**Focus**: ARM64 graduation and architecture intelligence

- [ ] ARM64 full support (graduate to Beta)
  - 30+ strategies covering common AArch64 patterns
  - ADRP/ADD pair transformations
  - Conditional select alternatives
  - STP/LDP displacement handling
- [ ] Automatic architecture detection (eliminate `--arch` flag requirement)
- [ ] Multi-profile chaining (apply multiple profiles sequentially)
- [ ] ML model retraining for generic bad-byte elimination
- [ ] Verification suite coverage metrics (LCOV integration)

## v6.0

**Focus**: ML intelligence and extensibility

- [ ] ML v3.0: Multi-byte pattern training
  - Profile-aware strategy selection
  - Architecture-specific model variants
  - Online learning during batch processing
- [ ] Plugin architecture for third-party strategies
- [ ] MIPS architecture support (Experimental)
- [ ] Strategy composition — combine multiple transformations per instruction
- [ ] Performance benchmarking suite with regression detection

## Contributing to the Roadmap

Feature requests and milestone feedback are welcome via [GitHub Issues](https://github.com/umpolungfish/byvalver/issues). See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.
