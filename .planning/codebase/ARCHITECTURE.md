# Architecture

**Analysis Date:** 2026-02-25

## Pattern Overview

**Overall:** Strategy-driven transformation pipeline with optional biphasic obfuscation and ML-assisted strategy reprioritization.

**Key Characteristics:**
- CLI-first orchestration in `src/main.c` with shared processing APIs in `src/processing.h`.
- Plugin-style strategy architecture via `strategy_t` in `src/strategy.h` and centralized registration in `src/strategy_registry.c`.
- Multi-pass instruction rewrite flow in `src/core.c` (offset planning, code generation, verification, fallback/rollback).

## Layers

**Interface & Orchestration Layer:**
- Purpose: Parse user input, choose processing mode, and coordinate lifecycle and output.
- Location: `src/main.c`, `src/cli.c`, `src/cli.h`, `src/processing.h`
- Contains: argument parsing, configuration defaults, single-file and batch execution branches, output formatting.
- Depends on: `src/core.c`, `src/strategy_registry.c`, `src/batch_processing.c`, `src/pic_generation.c`, `src/ml_strategist.c`.
- Used by: CLI binary entry point in `src/main.c`.

**Transformation Engine Layer:**
- Purpose: Disassemble instructions, select transformations, generate rewritten shellcode, and enforce bad-byte constraints.
- Location: `src/core.c`, `src/core.h`
- Contains: `remove_null_bytes`, `apply_obfuscation`, `biphasic_process`, relative jump rewriting, fallback generators, buffer primitives.
- Depends on: Capstone, `src/strategy_registry.c`, `src/obfuscation_strategy_registry.c`, `src/utils.c`, `src/ml_strategist.c`.
- Used by: `process_single_file` in `src/main.c`, training/evaluation helpers in `src/training_pipeline.c`.

**Strategy Registry & Strategy Modules Layer:**
- Purpose: Discover applicable strategies per instruction and prioritize execution order.
- Location: `src/strategy_registry.c`, `src/obfuscation_strategy_registry.c`, `src/*_strategies.c`, `src/*_strategies.h`
- Contains: strategy registration order, architecture compatibility filtering, priority sorting, optional ML reprioritization hook.
- Depends on: `src/strategy.h`, Capstone instruction metadata, strategy implementation files such as `src/mov_strategies.c` and `src/conditional_jump_offset_strategies.c`.
- Used by: `src/core.c` during transformation passes.

**Utility & Encoding Layer:**
- Purpose: Provide reusable code-generation helpers and bad-byte-safe encoding primitives.
- Location: `src/utils.c`, `src/utils.h`, `src/hash_utils.c`, `src/hash_utils.h`, `src/profile_aware_sib.c`, `src/profile_aware_sib.h`
- Contains: MOV/immediate construction helpers, buffer write helpers, bad-byte checks (`is_bad_byte_free_*`), x64 REX utilities, directory creation.
- Depends on: `src/core.h` and Capstone register metadata.
- Used by: most strategy modules under `src/` and fallback logic in `src/core.c`.

**Operational Extensions Layer:**
- Purpose: Support advanced runtime modes outside the base single-file transformation.
- Location: `src/batch_processing.c`, `src/pic_generation.c`, `src/tui/*.c`
- Contains: directory scans and stats aggregation, position-independent code wrappers, ncurses-based interactive menu.
- Depends on: `src/main.c` orchestration and shared core/CLI types.
- Used by: flags `--recursive`/`--pattern`, `--pic`, and `--menu`.

**ML & Training Layer:**
- Purpose: Track strategy effectiveness, reprioritize strategies, and train/save the ML model.
- Location: `src/ml_strategist.c`, `src/ml_metrics.c`, `src/ml_strategy_registry.c`, `src/training_pipeline.c`, `src/train_model.c`
- Contains: feature extraction, lightweight feedforward model inference, feedback loop, metrics export, model training workflow.
- Depends on: strategy registry outputs from `src/strategy_registry.c`, instruction data from Capstone, model file in `ml_models/byvalver_ml_model.bin`.
- Used by: CLI path when `--ml` is enabled and training utility in `src/train_model.c`.

## Data Flow

**Single-File Transformation Flow:**

1. `src/main.c` creates `byvalver_config_t` with `config_create_default` and updates it via `parse_arguments` from `src/cli.c`.
2. `src/main.c` initializes strategy registries with `init_strategies` (`src/strategy_registry.c`) and optional pass-1 obfuscation registry via `init_obfuscation_strategies` (`src/obfuscation_strategy_registry.c`).
3. `process_single_file` in `src/main.c` reads input bytes, optionally runs `pic_generate` (`src/pic_generation.c`), then executes `biphasic_process` or `remove_null_bytes` (`src/core.c`).
4. `remove_null_bytes` in `src/core.c` disassembles with Capstone, computes new offsets, applies top strategy from `get_strategies_for_instruction`, and falls back through `fallback_general_instruction` when needed.
5. `src/main.c` validates resulting buffer with `is_bad_byte_free_buffer`, applies optional XOR stub packaging, formats output (raw/C/Python/PowerShell/hex), and writes to disk.

**Batch Directory Flow:**

1. `src/main.c` detects directory input through `is_directory` (`src/batch_processing.c`).
2. `find_files` (`src/batch_processing.c`) resolves file list by pattern and recursion flags.
3. `src/main.c` loops through each file, reusing `process_single_file`, while `set_batch_stats_context` (`src/core.c`) and `batch_stats_add_*` (`src/batch_processing.c`) accumulate telemetry.
4. Batch summary and optional failed-file export are printed/written by `batch_stats_print` and `batch_write_failed_files`.

**State Management:**
- Run-time configuration lives in `byvalver_config_t` (`src/cli.h`) and is created/freed in `src/cli.c`.
- Global transformation context is managed by `g_bad_byte_context` and `g_batch_stats_context` in `src/core.c`.
- Strategy registry state is held in static arrays in `src/strategy_registry.c` and `src/obfuscation_strategy_registry.c`.
- ML state is maintained in static globals in `src/ml_strategist.c` and persisted to `ml_models/byvalver_ml_model.bin`.

## Key Abstractions

**Strategy Interface (`strategy_t`):**
- Purpose: Encapsulate instruction-specific rewrite logic behind a uniform API.
- Examples: `src/strategy.h`, `src/mov_strategies.c`, `src/modrm_sib_badbyte_strategies.c`
- Pattern: each module exposes `can_handle`, `get_size`, and `generate`, then registers via `register_*` called by `init_strategies`.

**Transformation Buffer (`struct buffer`):**
- Purpose: Append-only byte builder used by every transformation path.
- Examples: `src/core.h`, `src/core.c`, `src/utils.c`
- Pattern: initialize with `buffer_init`, append with `buffer_append`/`buffer_write_*`, free with `buffer_free`.

**Bad-Byte Configuration (`bad_byte_config_t` and `bad_byte_context_t`):**
- Purpose: Represent user-selected restricted bytes and expose O(1) checks during generation.
- Examples: `src/cli.h`, `src/cli.c`, `src/core.c`, `src/utils.c`
- Pattern: parse from CLI/profile, initialize global context, enforce through `is_bad_byte_free_byte` and `is_bad_byte_free_buffer`.

**Batch Statistics (`batch_stats_t`):**
- Purpose: Track per-file/per-strategy outcomes during directory processing.
- Examples: `src/batch_processing.h`, `src/batch_processing.c`, `src/main.c`
- Pattern: initialize once per run, update after each file, print/export at end.

**ML Strategist (`ml_strategist_t`):**
- Purpose: Reorder candidate strategies and learn from success/failure feedback.
- Examples: `src/ml_strategist.h`, `src/ml_strategist.c`, `src/ml_strategy_registry.c`
- Pattern: initialized once, invoked from `get_strategies_for_instruction`, updated via `provide_ml_feedback`, optionally saved/exported.

## Entry Points

**CLI Application:**
- Location: `src/main.c`
- Triggers: direct binary execution (`bin/byvalver` from `Makefile` target `all`).
- Responsibilities: parse flags, initialize subsystems, run single/batch transformations, emit outputs and stats.

**Training Utility:**
- Location: `src/train_model.c`
- Triggers: `make train` target in `Makefile`.
- Responsibilities: collect shellcode samples, execute `training_pipeline_execute`, save model artifacts under `ml_models/`.

**Agent Pipeline Orchestrator:**
- Location: `run_technique_generator.py`
- Triggers: `python3 run_technique_generator.py` or `make generate*`.
- Responsibilities: run discovery/proposal/codegen/implementation agents in `agents/` against `src/`.

**Test Runner:**
- Location: `tests/run_tests.sh`
- Triggers: `bash tests/run_tests.sh` and `.github/workflows/ci.yml`.
- Responsibilities: build check, CLI smoke tests, fixture transformations, batch regression check.

## Error Handling

**Strategy:** Defensive early returns with explicit exit/error codes and runtime fallbacks.

**Patterns:**
- Centralized numeric error codes in `src/cli.h` are returned by `parse_arguments`, `process_single_file`, and `main`.
- Transformation safety nets in `src/core.c` rollback failed strategy output and execute fallback generators when a strategy introduces bad bytes.
- Batch mode in `src/main.c`/`src/batch_processing.c` supports continue-on-error semantics and failed-file reporting.

## Cross-Cutting Concerns

**Logging:** Console logging uses `fprintf`/`printf` across `src/main.c`, `src/core.c`, and `src/ml_*.c`, with conditional debug macros in `src/core.h`.
**Validation:** Input/config validation is enforced in `src/cli.c`; output bad-byte verification is enforced in `src/main.c` and `src/core.c`; post-run verification scripts live in `verify_denulled.py`, `verify_functionality.py`, and `verify_semantic.py`.
**Authentication:** Core C processing in `src/` has no authentication layer; optional API-key checks for LLM providers are enforced in `run_technique_generator.py`.

---

*Architecture analysis: 2026-02-25*
