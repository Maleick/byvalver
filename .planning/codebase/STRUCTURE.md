# Codebase Structure

**Analysis Date:** 2026-02-25

## Directory Layout

```text
byvalver/
├── src/                  # Core C engine, strategy modules, ML, and CLI
├── src/tui/              # Ncurses interactive UI for menu-driven execution
├── agents/               # Python pipeline stages for auto-generating strategies
├── AjintK/               # Embedded multi-agent framework used by `agents/`
├── docs/                 # Current documentation set
├── assets/               # Large corpus: shellcodes, test artifacts, images, legacy docs
├── tests/                # Shell test runner and fixture layout docs
├── ml_models/            # Serialized ML model artifacts
├── .github/workflows/    # CI build and test workflow definitions
├── .planning/codebase/   # Generated codebase mapping documents
├── Makefile              # Build orchestration for CLI and training binary
├── Dockerfile            # Containerized build/runtime definition
└── run_technique_generator.py  # Agent pipeline entry script
```

## Directory Purposes

**`src/`:**
- Purpose: Main product code for shellcode transformation.
- Contains: core pipeline (`core.c`), CLI/config (`main.c`, `cli.c`), registries (`strategy_registry.c`), strategy modules (`*_strategies.c`), ML/training files (`ml_*.c`, `training_pipeline.c`), and utilities.
- Key files: `src/main.c`, `src/core.c`, `src/strategy_registry.c`, `src/obfuscation_strategy_registry.c`, `src/utils.c`

**`src/tui/`:**
- Purpose: Optional interactive mode enabled when ncurses is available.
- Contains: screen rendering, widgets, file browser, config builder.
- Key files: `src/tui/tui_menu.c`, `src/tui/tui_screens.c`, `src/tui/tui_file_browser.c`, `src/tui/tui_config_builder.c`

**`agents/`:**
- Purpose: AI-assisted strategy discovery/proposal/codegen/implementation pipeline.
- Contains: Python agent classes that scan `src/`, generate strategy code, patch registries, and verify builds.
- Key files: `agents/strategy_discovery_agent.py`, `agents/technique_proposal_agent.py`, `agents/code_generation_agent.py`, `agents/implementation_agent.py`

**`AjintK/`:**
- Purpose: Agent framework dependency vendored in-repo for orchestration primitives.
- Contains: base agent abstractions, orchestration, provider adapters, examples.
- Key files: `AjintK/framework/base_agent.py`, `AjintK/framework/orchestrator.py`, `AjintK/config_template.yaml`, `AjintK/README.md`

**`tests/`:**
- Purpose: CI-facing smoke and regression test entrypoints.
- Contains: shell runner and usage documentation for fixture layout.
- Key files: `tests/run_tests.sh`, `tests/README.md`

**`assets/`:**
- Purpose: Data-heavy project assets for shellcode corpora, docs snapshots, and supplementary tests.
- Contains: shellcode datasets (`assets/shellcodes/`), extended test files (`assets/tests/`), docs/images (`assets/docs/`, `assets/images/`).
- Key files: `assets/tests/test_bad_bytes.sh`, `assets/tests/test_full_pipeline.c`, `assets/docs/STRATEGY_HIERARCHY.md`

**`docs/`:**
- Purpose: User and contributor documentation for build, usage, strategy catalogs, and TUI.
- Contains: Markdown guides mirrored from project README sections.
- Key files: `docs/USAGE.md`, `docs/BUILD.md`, `docs/TUI_README.md`, `docs/WHITEPAPER.md`

**`ml_models/`:**
- Purpose: Model artifact storage for ML strategy selection.
- Contains: binary model files loaded by `--ml` mode and training pipeline output.
- Key files: `ml_models/byvalver_ml_model.bin`

## Key File Locations

**Entry Points:**
- `src/main.c`: Primary CLI entry point and runtime orchestrator.
- `src/train_model.c`: Standalone training executable entry point.
- `run_technique_generator.py`: Python orchestrator for the 4-stage agent pipeline.
- `tests/run_tests.sh`: Top-level automated test entrypoint.

**Configuration:**
- `src/cli.h`: Runtime option schema, architecture enums, and exit-code contract.
- `src/cli.c`: Option parsing, defaults, profile/bad-byte config loading.
- `Makefile`: Build flags, source inclusion/exclusion, target definitions.
- `.github/workflows/ci.yml`: CI dependency install, build, and test flow.
- `docker-compose.yml`: Containerized execution modes (`byvalver-dev`, `verify`).

**Core Logic:**
- `src/core.c`: Disassembly, transformation planning, bad-byte elimination, and biphasic execution.
- `src/strategy_registry.c`: Registration and selection of transformation strategies.
- `src/obfuscation_strategy_registry.c`: Pass-1 obfuscation registry for biphasic mode.
- `src/utils.c`: Encoding helpers and bad-byte-safe generation primitives.
- `src/batch_processing.c`: Recursive input discovery and batch statistics.
- `src/ml_strategist.c`: ML feature extraction, prediction, and feedback integration.

**Testing:**
- `tests/run_tests.sh`: Build/CLI/fixture/batch smoke tests.
- `verify_denulled.py`: Bad-byte verification tool.
- `verify_functionality.py`: Instruction-level functionality heuristics.
- `verify_semantic.py`: Pattern-level semantic-preservation heuristics.
- `assets/tests/`: Supplemental C/ASM/Python/sh test assets and fixtures.

## Naming Conventions

**Files:**
- `*_strategies.c` and `*_strategies.h`: transformation strategy modules, for example `src/mov_strategies.c` and `src/lea_displacement_strategies.h`.
- `*_obfuscation.c`: pass-1 obfuscation strategy modules, for example `src/mutated_junk_insertion_obfuscation.c`.
- `ml_*.c` and `ml_*.h`: ML subsystem files, for example `src/ml_metrics.c`.
- `tui_*.c` and `tui_*.h`: interactive UI files under `src/tui/`.

**Directories:**
- Core C code is flat under `src/` with a single focused subdirectory `src/tui/`.
- Supporting Python automation is split into `agents/` (pipeline stages) and `AjintK/` (framework).
- Documentation and datasets are separated into `docs/` (current docs) and `assets/` (large content and historical/support material).

## Where to Add New Code

**New Feature:**
- Primary code: `src/` (or `src/tui/` if UI-only), with strategy changes usually in a new `src/<feature>_strategies.c` + `src/<feature>_strategies.h`.
- Tests: `tests/run_tests.sh` for CI flow changes and `assets/tests/` plus `verify_*.py` for behavior validation.

**New Component/Module:**
- Implementation: `src/` with header/source pair and explicit registration hook in `src/strategy_registry.c` (or `src/obfuscation_strategy_registry.c` for pass-1 only modules).

**Utilities:**
- Shared helpers: `src/utils.c` and `src/utils.h`; batch-specific helpers in `src/batch_processing.c`.

## Special Directories

**`bin/`:**
- Purpose: Compiled object files and final binaries from `make`.
- Generated: Yes
- Committed: No

**`assets/`:**
- Purpose: Reference shellcode corpora, supplemental tests, media, and legacy docs.
- Generated: No
- Committed: Yes

**`.planning/codebase/`:**
- Purpose: Generated architecture/structure/convention/concern mapping documents for planning workflows.
- Generated: Yes
- Committed: Yes

**`ml_models/`:**
- Purpose: Runtime/training ML model artifacts.
- Generated: Yes
- Committed: Yes

---

*Structure analysis: 2026-02-25*
