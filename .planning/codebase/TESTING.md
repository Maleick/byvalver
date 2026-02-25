# Testing Patterns

**Analysis Date:** 2026-02-25

## Test Framework

**Runner:**
- Custom script-based harness (Bash + Python, no single test framework/version) centered on `tests/run_tests.sh`
- Config: `tests/run_tests.sh` (invoked in CI from `.github/workflows/ci.yml`)

**Assertion Library:**
- No standalone assertion library; assertions are exit-code checks and explicit conditions in scripts (`tests/run_tests.sh`, `assets/tests/test_new_strategies.py`, `AjintK/test_integration.py`, `verify_denulled.py`)

**Run Commands:**
```bash
bash tests/run_tests.sh                                # Run all tests
# Not applicable: no watch-mode runner is configured   # Watch mode
# Not applicable: coverage command is not configured    # Coverage
```

## Test File Organization

**Location:**
- Keep the primary CI smoke/integration path in `tests/` (`tests/run_tests.sh`, `tests/README.md`)
- Keep deep/experimental regression material in `assets/tests/` (`assets/tests/test_bad_bytes.sh`, `assets/tests/test_http_whitespace_profile.sh`, `assets/tests/.tests/test_movzx.py`)
- Keep verification utilities at repo root (`verify_denulled.py`, `verify_functionality.py`, `verify_semantic.py`) and framework integration checks in `AjintK/test_integration.py`

**Naming:**
- Use `test_*` naming for scripts and helper programs (`assets/tests/test_new_strategies.py`, `assets/tests/test_arch.c`, `assets/tests/.tests/test_loop.py`, `assets/tests/test_bad_bytes.sh`)

**Structure:**
```
tests/run_tests.sh
assets/tests/test_*.sh
assets/tests/test_*.py
assets/tests/.tests/test_*.py
assets/tests/test_*.c
AjintK/test_integration.py
verify_*.py
```

## Test Structure

**Suite Organization:**
```typescript
def main():
    results = []
    results.append(("PUSHW 16-bit Immediate", test_pushw_strategy()))
    results.append(("CLTD Zero Extension", test_cltd_strategy()))
    results.append(("Combined Strategies", test_combined_strategies()))
    return 0 if all(result for _, result in results) else 1

if __name__ == '__main__':
    sys.exit(main())
```

**Patterns:**
- Setup pattern: create temp paths/dirs and seed binary fixtures (`mktemp -d` + trap in `tests/run_tests.sh`, `/tmp/test_*` inputs in `assets/tests/test_new_strategies.py`, `.test_bins` in `assets/tests/.tests/test_getpc.py`)
- Teardown pattern: explicit shell cleanup hooks (`trap cleanup EXIT` in `tests/run_tests.sh`) and per-script overwrite/ephemeral temp files in Python tests (`assets/tests/test_new_strategies.py`)
- Assertion pattern: command return-code checks plus byte-level validation (`python3 verify_denulled.py ...` in `tests/run_tests.sh`, `if b'\\x00' in output: return False` in `assets/tests/test_new_strategies.py`, expected-error validation in `AjintK/test_integration.py`)

## Mocking

**Framework:** None

**Patterns:**
```typescript
result = subprocess.run(
    ['./bin/byvalver', '-v', input_file, output_file],
    capture_output=True,
    text=True
)
if result.returncode != 0:
    return False
```

**What to Mock:**
- If adding fast unit checks, mock only expensive external process boundaries (`subprocess.run(...)` wrappers around `./bin/byvalver`, `nasm`, `objdump`) used in `assets/tests/.tests/test_indirect_call.py` and `verify_functionality.py`

**What NOT to Mock:**
- Do not mock the core binary path used by CI; keep real build + execution + verification (`make` and `bash tests/run_tests.sh` in `.github/workflows/ci.yml`, output-byte verification in `verify_denulled.py`)

## Fixtures and Factories

**Test Data:**
```typescript
test_shellcode = bytes([0x68, 0x5C, 0x11, 0x00, 0x00])
with open('/tmp/test_pushw_input.bin', 'wb') as f:
    f.write(test_shellcode)
```

**Location:**
- Primary fixture convention is `tests/fixtures/<arch>/` as consumed by `tests/run_tests.sh` and documented in `tests/README.md`
- Additional fixture generators and scratch outputs are under `assets/tests/.tests/`, `assets/tests/test_outputs_bad_chars/`, and `/tmp` file paths used by `assets/tests/test_new_strategies.py`

## Coverage

**Requirements:** None enforced

**View Coverage:**
```bash
# Not applicable: no coverage toolchain/config is defined in `Makefile` or `.github/workflows/ci.yml`
bash tests/run_tests.sh
```

## Test Types

**Unit Tests:**
- Lightweight encoding/parsing checks in focused C/Python programs (`assets/tests/test_arch_parsing.c`, `assets/tests/test_arm_encoding.c`, `assets/tests/.tests/test_loop.py`)

**Integration Tests:**
- Build + CLI + transformation verification in `tests/run_tests.sh`; profile/regression scripts in `assets/tests/test_bad_bytes.sh` and `assets/tests/test_http_whitespace_profile.sh`; async provider/framework integration in `AjintK/test_integration.py`

**E2E Tests:**
- Not used as a dedicated browser/service E2E framework; closest E2E path is binary-level CLI execution on fixture corpora (`tests/run_tests.sh`, `assets/tests/test_new_strategies.py`)

## Common Patterns

**Async Testing:**
```typescript
async def run_tests():
    results = [
        await test_provider_creation(),
        await test_base_agent_update(),
        await test_example_agents(),
        await test_orchestrator()
    ]
    return 0 if all(results) else 1

if __name__ == "__main__":
    sys.exit(asyncio.run(run_tests()))
```

**Error Testing:**
```typescript
result = subprocess.run(cmd, capture_output=True, text=True)
if result.returncode != 0:
    print(f"FAIL: {result.stderr}")
    return False
```

---

*Testing analysis: 2026-02-25*
