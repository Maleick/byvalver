# Coding Conventions

**Analysis Date:** 2026-02-25

## Naming Patterns

**Files:**
- Use `snake_case` file names across the repository; C strategy modules use `<topic>_strategies.c/.h` in `src/` (for example `src/mov_strategies.c`, `src/loop_comprehensive_strategies.h`), TUI modules use `tui_<feature>.c/.h` in `src/tui/`, and Python modules/scripts use `snake_case` in `agents/`, `AjintK/framework/`, and root scripts like `verify_denulled.py`.

**Functions:**
- Use lower `snake_case` function names in C and Python, with strategy triplets in C (`can_handle_*`, `get_size_*`, `generate_*`) as shown in `src/mov_strategies.c`, plus operation-focused helpers like `batch_stats_add_failed_file` in `src/batch_processing.c` and `parse_bad_chars` in `verify_denulled.py`.

**Variables:**
- Use lower `snake_case` for locals/fields (`bad_byte_count`, `output_file_specified_via_flag` in `src/cli.h`), and use `g_` prefixes for mutable C globals (`g_bad_byte_context` in `src/core.c`, `g_ml_strategist` in `src/strategy_registry.c`).

**Types:**
- Use `_t` suffix for C typedefs (`strategy_t` in `src/strategy.h`, `byvalver_config_t` in `src/cli.h`, `batch_stats_t` in `src/batch_processing.h`), and `UPPER_SNAKE_CASE` for enum constants/macros (`BYVAL_ARCH_X64`, `EXIT_PROCESSING_FAILED` in `src/cli.h`).

## Code Style

**Formatting:**
- Use `clang-format` via `make format` in `Makefile`; the canonical formatter config is `.clang-format`.
- Keep C formatting aligned with `.clang-format` settings (`IndentWidth: 4`, `ColumnLimit: 100`, `BreakBeforeBraces: Linux`, `PointerAlignment: Right`, `SortIncludes: false`) and compile-target style from `Makefile` (`-std=c99 -Wall -Wextra -pedantic`).

**Linting:**
- Use optional `cppcheck` via `make lint` in `Makefile` for C static analysis.
- No repo-level Python linter config is detected at root (no `pyproject.toml`, `.flake8`, `ruff.toml`), so follow existing typed, PEP8-style patterns in `agents/*.py` and `AjintK/framework/*.py`.

## Import Organization

**Order:**
1. Project-local headers/modules first (for example `#include "strategy.h"` in `src/mov_strategies.c`, local framework imports in `agents/strategy_discovery_agent.py` after path setup).
2. Standard/system libraries second (for example `<stdio.h>`, `<string.h>` in `src/core.c`; `import os`, `import asyncio` in `AjintK/framework/base_agent.py`).
3. Third-party libraries last or adjacent to system imports depending on file (for example `<capstone/capstone.h>` in `src/core.h`, `import httpx` in `AjintK/framework/tools.py`); preserve file-local ordering because `.clang-format` sets `SortIncludes: false`.

**Path Aliases:**
- Not detected; C uses relative include paths (for example `#include "tui/tui_menu.h"` in `src/main.c`) and Python uses explicit `sys.path.insert(...)` path bootstrapping in `agents/*.py` and `run_technique_generator.py`.

## Error Handling

**Patterns:**
- Return explicit status/exit codes in C and fail fast on invalid state (`EXIT_*` in `src/cli.h`, early `return` checks in `src/main.c` and `src/batch_processing.c`).
- Print user-facing errors to `stderr` with contextual values (`src/cli.c`, `src/core.c`, `src/batch_processing.c`).
- In Python scripts, return integer status from `main()` and terminate with `sys.exit(...)` (`assets/tests/test_new_strategies.py`, `AjintK/test_integration.py`, `verify_semantic.py`).

## Logging

**Framework:** console

**Patterns:**
- C uses `fprintf(stderr, ...)` for errors and `printf(...)` for progress/metrics (`src/batch_processing.c`, `src/ml_metrics.c`), with debug macros gated by `#ifdef DEBUG` in `src/core.h`.
- Python uses direct progress prints (for example `[ImplementationAgent]` lines in `agents/implementation_agent.py`, test status output in `assets/tests/test_new_strategies.py`) with no centralized logging policy required.

## Comments

**When to Comment:**
- Add file-level and section-level comments for module intent and architecture context (`src/byvalver.h`, `src/obfuscation_strategy_registry.c`).
- Add inline comments for non-obvious instruction encoding and transformation rationale (`src/mov_strategies.c`, `src/utils.c`, `assets/tests/.tests/test_movzx_sib.py`).

**JSDoc/TSDoc:**
- Not applicable (no JS/TS source detected). Use Doxygen-style C comments where needed (`src/byvalver.h`, `src/core.c`) and Python docstrings for scripts/agents (`agents/code_generation_agent.py`, `verify_denulled.py`).

## Function Design

**Size:** Keep per-strategy handlers small and focused (`can_handle_*`, `get_size_*`, `generate_*` in `src/mov_strategies.c`) and keep orchestration in dedicated pipeline functions (`process_single_file` in `src/main.c`, `verify_functionality` in `verify_functionality.py`).

**Parameters:** Prefer explicit pointer/context passing in C (`byvalver_config_t *config` in `src/main.c`, `batch_stats_t *stats` in `src/batch_processing.c`) and the `(task, context=None)` signature in Python agent entrypoints (`agents/strategy_discovery_agent.py`, `AjintK/framework/base_agent.py`).

**Return Values:** Use C return codes and structs for result transport (`int` status + `struct buffer` in `src/core.c` and `src/main.c`) and Python structured dict payloads with `status`/`findings`/metadata (`agents/implementation_agent.py`, `AjintK/agents/example_agent.py`).

## Module Design

**Exports:** Expose C module APIs through paired headers with include guards (`src/cli.h` + `src/cli.c`, `src/batch_processing.h` + `src/batch_processing.c`) and register strategy modules through explicit `register_*` hooks in `src/strategy_registry.c`; Python packages re-export key classes via `__init__.py` (`AjintK/framework/__init__.py`, `AjintK/agents/__init__.py`).

**Barrel Files:** C barrel files are not used; Python uses lightweight barrel-style exports in `__init__.py` (`AjintK/framework/__init__.py`, `AjintK/agents/__init__.py`).

---

*Convention analysis: 2026-02-25*
