# Contributing to byvalver

Thank you for your interest in contributing to byvalver. This document covers the process for reporting bugs, submitting changes, and extending the strategy system.

## Reporting Bugs

Open a [GitHub Issue](https://github.com/umpolungfish/byvalver/issues) with:

1. **Architecture and profile used** (e.g., `--arch x64 --profile http-newline`)
2. **Input shellcode** (hex dump or binary, if shareable)
3. **Expected vs actual behavior**
4. **Verbose output** (`--verbose` flag)
5. **Build environment** (OS, GCC version, Capstone version)

## Development Setup

### Dependencies

**Ubuntu/Debian:**
```bash
sudo apt install gcc make nasm xxd pkg-config libcapstone-dev libncurses-dev python3
```

**Fedora:**
```bash
sudo dnf install gcc make nasm vim-common pkgconf-pkg-config capstone-devel ncurses-devel python3
```

**macOS (Homebrew):**
```bash
brew install nasm capstone ncurses
```

**Arch Linux:**
```bash
sudo pacman -S gcc make nasm xxd pkg-config capstone ncurses python
```

### Building

```bash
make clean && make          # Standard build
make debug                  # Debug build with sanitizers
make release                # Optimized release build
```

### Running Tests

```bash
# Full test suite
bash tests/run_tests.sh

# Individual verification scripts
python3 verify_denulled.py output.bin
python3 verify_functionality.py input.bin output.bin
python3 verify_semantic.py input.bin output.bin
```

## Adding New Strategies

byvalver uses a modular strategy registry pattern. Each strategy is a function registered with a priority, architecture target, and capability description.

### 1. Create the Strategy Implementation

Create a new file `src/my_new_strategies.c`:

```c
#include "strategy.h"
#include "core.h"
#include <capstone/capstone.h>

static bool can_handle_my_transform(const cs_insn *insn, const uint8_t *bad_bytes,
                                     size_t bad_byte_count) {
    // Return true if this strategy can handle this instruction
    // Check: instruction type, operand types, presence of bad bytes
    return false;
}

static int apply_my_transform(const cs_insn *insn, uint8_t *output, size_t *output_size,
                               const uint8_t *bad_bytes, size_t bad_byte_count) {
    // Transform the instruction to eliminate bad bytes
    // Return 0 on success, -1 on failure
    return -1;
}
```

### 2. Register the Strategy

Add your strategy to the appropriate registry file (e.g., `src/strategy_registry.c`):

```c
register_strategy("my_transform_description", PRIORITY_LEVEL,
                   can_handle_my_transform, apply_my_transform,
                   BYVAL_ARCH_X86 | BYVAL_ARCH_X64);
```

**Priority levels** (lower = tried first):
- 1-50: Core transformations (MOV, arithmetic)
- 51-100: Standard transformations
- 101-150: Advanced/specialized transformations
- 151-200: Fallback/experimental transformations

See [docs/STRATEGY_HIERARCHY.md](docs/STRATEGY_HIERARCHY.md) for the full priority system.

### 3. Test Your Strategy

```bash
# Build with your new strategy
make

# Test against a shellcode sample with the relevant bad bytes
./bin/byvalver --verbose --arch x64 input.bin output.bin

# Verify bad bytes eliminated
python3 verify_denulled.py output.bin

# Verify semantic equivalence
python3 verify_semantic.py input.bin output.bin
```

## ML Model Retraining

If you've added new strategies or training data:

```bash
# Build the trainer
make train

# Run training (uses shellcodes in ./shellcodes/)
./bin/train_model

# Output: ./ml_models/byvalver_ml_model.bin
# Config: 10k samples, 50 epochs, 20% validation, LR 0.001, batch 32

# Test the updated model
./bin/byvalver --ml --verbose input.bin output.bin
```

## Code Style

This project uses `clang-format` with the configuration in `.clang-format`:

```bash
make format    # Format all source files
make lint      # Run cppcheck static analysis
```

Key conventions:
- C99 standard (`-std=c99`)
- 4-space indentation, no tabs
- 100 character column limit
- Linux-style brace placement
- No sorted includes (order is intentional)

## Pull Request Process

1. Fork the repository and create a feature branch
2. Ensure your changes build cleanly: `make clean && make`
3. Run the verification suite: `bash tests/run_tests.sh`
4. Format your code: `make format`
5. Write a clear commit message describing the change
6. Open a pull request against `main`

### PR Requirements

- [ ] Builds without warnings (`-Wall -Wextra -pedantic`)
- [ ] No new bad-byte elimination regressions
- [ ] Verification scripts pass on test fixtures
- [ ] New strategies include `can_handle` + `apply` functions
- [ ] Strategy registered with appropriate priority level
