# byvalver Documentation

Complete documentation for the byvalver shellcode bad-byte elimination framework.

## Quick Reference

| Document | Description |
|----------|-------------|
| [USAGE.md](USAGE.md) | Comprehensive usage guide with examples for all CLI options |
| [BUILD.md](BUILD.md) | Build instructions, compiler flags, platform-specific notes |
| [TUI_README.md](TUI_README.md) | Interactive TUI interface — menus, file browser, batch processing |
| [BAD_BYTE_PROFILES.md](BAD_BYTE_PROFILES.md) | 13 pre-configured bad-byte profiles for common exploit contexts |

## Strategy Documentation

| Document | Description |
|----------|-------------|
| [DENULL_STRATS.md](DENULL_STRATS.md) | 170+ denullification strategy catalog with technical details |
| [OBFUSCATION_STRATS.md](OBFUSCATION_STRATS.md) | 30+ obfuscation techniques (Pass 1 of biphasic architecture) |
| [BADBYTEELIM_STRATS.md](BADBYTEELIM_STRATS.md) | Extended bad-byte elimination strategies — comprehensive reference |
| [STRATEGY_HIERARCHY.md](STRATEGY_HIERARCHY.md) | Strategy priority ordering and organization system |
| [ADVANCED_STRATEGIES.md](ADVANCED_STRATEGIES.md) | Advanced transformation techniques for edge cases |

## Research

| Document | Description |
|----------|-------------|
| [WHITEPAPER.md](WHITEPAPER.md) | Technical whitepaper v4.2 — architecture, algorithms, performance analysis |

## Cross-References

- **New users**: Start with [USAGE.md](USAGE.md) for command examples, then [BAD_BYTE_PROFILES.md](BAD_BYTE_PROFILES.md) for profile selection
- **Building from source**: See [BUILD.md](BUILD.md) for platform-specific instructions
- **TUI users**: See [TUI_README.md](TUI_README.md) for interactive interface documentation
- **Strategy developers**: Read [STRATEGY_HIERARCHY.md](STRATEGY_HIERARCHY.md) first, then [DENULL_STRATS.md](DENULL_STRATS.md) for implementation patterns
- **Researchers**: See [WHITEPAPER.md](WHITEPAPER.md) for academic-level technical analysis
- **Contributors**: See [CONTRIBUTING.md](../CONTRIBUTING.md) for contribution guidelines
