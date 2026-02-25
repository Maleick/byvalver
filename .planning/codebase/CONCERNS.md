# Codebase Concerns

**Analysis Date:** 2026-02-25

## Tech Debt

**Strategy Quality Drift:**
- Issue: Large parts of the strategy corpus are stubbed, placeholder-based, or explicitly marked broken/disabled while still living in the active code paths.
- Files: `src/obfuscation_strategy_registry.c`, `src/runtime_selfmod_obfuscation.c`, `src/short_conditional_jump_strategies.c`, `src/setcc_jump_elimination_strategies.c`, `src/register_allocation_strategies.c`, `src/atomic_operation_encoding_strategies.c`
- Impact: High maintenance cost, unpredictable transformation behavior, and frequent rollback-to-fallback execution.
- Fix approach: Introduce strategy maturity levels (stable/experimental/disabled), register only stable strategies by default, and gate experimental modules behind explicit flags.

**Bit-Rotted Library Surface and Legacy Artifacts:**
- Issue: A library API implementation exists but is excluded from builds and out of sync with current headers/signatures.
- Files: `Makefile`, `src/lib_api.c`, `src/byvalver.h`, `src/core.h`
- Impact: Public API cannot be trusted or shipped; contributors can accidentally rely on stale interfaces.
- Fix approach: Either remove the library API and related docs, or fully re-enable it with CI coverage and aligned function signatures.

**Header and Build Hygiene Debt:**
- Issue: Build output shows pervasive prototype-style and header formatting warnings, creating high warning noise.
- Files: `src/strategy.h`, `src/core.h`, `src/cli.h`, `src/utils.h`
- Impact: Real warnings are harder to detect, and compiler/toolchain upgrades become riskier.
- Fix approach: Enforce clean `-Wall -Wextra` for project-owned headers in CI and standardize prototypes/newline/style automatically.

## Known Bugs

**Default Build Breaks with TUI Enabled:**
- Symptoms: `make` fails with `unable to open output file 'bin/tui/tui_config_builder.o': 'No such file or directory'`.
- Files: `Makefile`
- Trigger: Run `make` on a clean tree when TUI sources are included.
- Workaround: Manually create `bin/tui` before build or use `make no-tui`.

**Invalid Free in Config Loader for Default String Literals:**
- Symptoms: Potential crash/heap corruption when loading config keys that replace default literal pointers.
- Files: `src/cli.c`
- Trigger: Use `--config` with `[ml] metrics_output_file=...` or `[batch] file_pattern=...`; loader frees default literal-backed pointers before `strdup`.
- Workaround: Avoid these config keys and set equivalent values via CLI flags until pointer ownership is fixed.

**Incorrect Arithmetic Fallback for `EAX` Destination:**
- Symptoms: Fallback path can produce semantically incorrect arithmetic transformations.
- Files: `src/core.c`
- Trigger: Fallback path for `ADD/SUB/AND/OR/XOR/CMP EAX, imm` when strategy handling fails and immediate contains bad bytes.
- Workaround: Force stable strategy handling for these opcodes or disable the affected fallback branch for `EAX`.

**Argument Parser Performs Process Exit Internally:**
- Symptoms: `--list-profiles` exits from inside argument parsing, bypassing normal cleanup/flow.
- Files: `src/cli.c`
- Trigger: Invoke `byvalver --list-profiles`.
- Workaround: None in current design; parser behavior must be refactored to return control to `main`.

## Security Considerations

**Unconditional Payload Logging:**
- Risk: Input shellcode bytes and processing details are written to stderr regardless of quiet intent.
- Files: `src/core.c`
- Current mitigation: Minimal; some logs are behind `DEBUG`, but several are unconditional.
- Recommendations: Guard all payload-bearing logs behind explicit verbosity/debug levels and default to redacted output.

**Recursive Traversal Follows Symlinks Without Loop Protection:**
- Risk: Batch mode can traverse outside intended trees and can recurse indefinitely on symlink cycles.
- Files: `src/batch_processing.c`
- Current mitigation: None (uses `stat`, not `lstat`, and has no visited inode tracking).
- Recommendations: Add symlink policy controls, use `lstat`, and track visited `(dev,inode)` pairs during recursive walks.

**Unsafe Output File Open Semantics:**
- Risk: Output writes use plain `fopen(..., "wb")`, enabling clobber of symlink targets when run with elevated permissions.
- Files: `src/main.c`
- Current mitigation: Parent directory creation only (`create_parent_dirs`), no anti-symlink protections.
- Recommendations: Add safe-open mode using `open` + `O_NOFOLLOW`/`O_EXCL` and explicit overwrite policy.

## Performance Bottlenecks

**Strategy Selection is O(Instructions × Strategy Count):**
- Problem: Every bad-byte instruction performs a full scan over registered strategies, repeated across multiple passes.
- Files: `src/core.c`, `src/strategy_registry.c`
- Cause: Linear strategy matching (`for (i = 0; i < strategy_count; i++)`) with repeated calls in sizing/generation/diagnostics.
- Improvement path: Pre-index strategies by opcode/operand class and cache matches per instruction signature.

**Pass 1 Obfuscation Frequently Does Wasted Work:**
- Problem: Highest-priority obfuscation path can match everything, generate invalid output for constraints, then rollback repeatedly.
- Files: `src/obfuscation_strategy_registry.c`, `src/runtime_selfmod_obfuscation.c`, `src/core.c`
- Cause: `can_handle` returns universal match while generated bytes include bad bytes that trigger rollback logic.
- Improvement path: Tighten `can_handle`, disable incomplete strategies by default, and avoid expensive generate-then-rollback loops.

**Batch Statistics Double-Read Input Files:**
- Problem: Batch mode reopens and rereads each input file after processing for stats computation.
- Files: `src/main.c`, `src/batch_processing.c`
- Cause: Stats gathering is decoupled from the primary processing read path.
- Improvement path: Return instruction/bad-byte metrics from `process_single_file` and record stats in one pass.

## Fragile Areas

**Core Transformation Pipeline:**
- Files: `src/core.c`, `src/core.h`
- Why fragile: A single large module mixes disassembly, sizing, transformation, fallback logic, verification, and diagnostics with global state.
- Safe modification: Isolate jump rewriting, fallback generation, and verification into separately testable units before behavioral changes.
- Test coverage: No direct unit tests target `process_relative_jump`, fallback branches, or rollback edge cases.

**Registry and Priority Orchestration:**
- Files: `src/strategy_registry.c`, `src/obfuscation_strategy_registry.c`
- Why fragile: Manual registration order and many priority interactions make small changes ripple unpredictably.
- Safe modification: Move registration metadata to a manifest/codegen path and add snapshot tests for strategy ordering by opcode.
- Test coverage: No automated regression asserting selected top strategy per representative instruction set.

**TUI Screen State Handling:**
- Files: `src/tui/tui_screens.c`, `src/tui/tui_menu.c`, `src/tui/tui_file_browser.c`
- Why fragile: Large stateful ncurses flow with placeholder sections and mixed rendering/input/business logic.
- Safe modification: Split per-screen controllers and centralize state transitions; avoid direct mutation in render functions.
- Test coverage: No automated TUI interaction tests.

## Scaling Limits

**Strategy Registry Capacity:**
- Current capacity: Hard cap `MAX_STRATEGIES = 400`.
- Limit: Additional strategy growth risks registration overflow and dropped strategies.
- Scaling path: Replace fixed-size arrays with dynamic storage and fail CI when capacity thresholds are exceeded.

**Global Mutable Context Model:**
- Current capacity: One active transformation context per process (`g_bad_byte_context`, `g_batch_stats_context`, offset hash globals).
- Limit: Unsafe for concurrent processing and difficult to embed in multi-threaded callers.
- Scaling path: Pass context objects explicitly and remove global mutable state from core APIs.

## Dependencies at Risk

**Capstone (system package):**
- Risk: Current build flags (`-std=c99 -pedantic`) generate very high warning volume with modern Capstone headers.
- Impact: Warning fatigue masks real project defects and complicates compiler/toolchain upgrades.
- Migration plan: Pin tested Capstone versions, switch project standard to C11 (or isolate/suppress third-party header warnings), and keep CI warning budgets strict for project code.

**LLM Provider SDK Stack (`anthropic`/`httpx`/`tenacity`) for agent pipeline:**
- Risk: Network-dependent unpinned code generation path writes directly to `src/` and patches registry.
- Impact: Nondeterministic code quality and potential regressions entering main code paths.
- Migration plan: Pin package versions, require generated-code review gates, and enforce deterministic validation suites before merge.

## Missing Critical Features

**Reliable Target-Aware Conditional Jump Rewriting:**
- Problem: Multiple jump strategies still use placeholder displacements or simplified control-flow substitutions.
- Blocks: Trustworthy semantic preservation for branch-heavy shellcode in `src/short_conditional_jump_strategies.c` and `src/setcc_jump_elimination_strategies.c`.

**Complete ARM64 Transformation Coverage:**
- Problem: ARM64 strategy registration is mostly skeletal (arithmetic/memory/branch TODOs).
- Blocks: Production-grade ARM64 bad-byte elimination from `src/arm64_strategies.c`.

**Deterministic Semantic Regression Harness in CI:**
- Problem: Existing CI/test flow focuses on build/smoke and fixture runs, not strategy-level semantic invariants.
- Blocks: Safe refactoring of core strategy/fallback code in `src/core.c` and `src/strategy_registry.c`.

## Test Coverage Gaps

**Core Fallback and Jump Rewriting Paths:**
- What's not tested: Semantic correctness of fallback branches and relative/conditional jump rewrites under bad-byte pressure.
- Files: `src/core.c`, `src/short_conditional_jump_strategies.c`, `src/setcc_jump_elimination_strategies.c`
- Risk: Silent control-flow corruption in transformed shellcode.
- Priority: High

**Config Parsing Ownership and Error Paths:**
- What's not tested: `--config` memory ownership transitions and mixed source-of-truth interactions between defaults, config file, and CLI flags.
- Files: `src/cli.c`
- Risk: Crashes or undefined behavior in real-world config-driven runs.
- Priority: High

**Build Matrix and TUI Compilation Path:**
- What's not tested: Clean-room default build behavior when TUI sources are compiled with generated object paths.
- Files: `Makefile`, `src/tui/tui_config_builder.c`, `src/tui/tui_screens.c`
- Risk: Broken default build and blocked release pipeline.
- Priority: High

**Library API Compatibility Surface:**
- What's not tested: Public API compile/link/runtime path for library consumers.
- Files: `src/lib_api.c`, `src/byvalver.h`, `src/core.h`
- Risk: External integrations break immediately if library path is enabled.
- Priority: Medium

---

*Concerns audit: 2026-02-25*
