# byvalver (·𐑚𐑲𐑝𐑨𐑤𐑝𐑼) - Project Documentation

## Project Overview
`byvalver` is a professional-grade CLI tool and framework written in C, designed for the automated elimination of "bad-bytes" (primarily null bytes) from shellcode. It supports x86, x64, ARM, and ARM64 architectures, maintaining functional equivalence through an extensive library of 170+ transformation strategies.

### Key Technologies
- **Language:** Pure C (C99 standard)
- **Disassembly:** [Capstone Framework](http://www.capstone-engine.org/)
- **Assembler:** [NASM](https://www.nasm.us/) (for decoder stubs)
- **UI:** Ncurses (for interactive TUI mode)
- **Intelligence:** Custom Feedforward Neural Network for strategy selection optimization
- **Verification:** Python-based testing suite for functional and semantic equivalence

## Architecture
`byvalver` employs a modular strategy-pattern architecture:
1.  **Pass 1: Obfuscation (Optional):** Enabled via `--biphasic`. Applies anti-analysis techniques like control-flow flattening, register remapping, and dead-code insertion.
2.  **Pass 2: Denullification/Banishment:** The core engine. It disassembles the input, identifies instructions containing bad bytes, and selects the most efficient transformation strategy to replace them.
3.  **ML Strategist:** A neural network that predicts the most successful strategy based on the instruction context (opcode, operands, and neighboring instructions).
4.  **Batch Processor:** Handles recursive directory traversal for large-scale shellcode scrubbing.

## Building and Running

### Prerequisites
- GCC/Clang
- `libcapstone-dev`
- `nasm`
- `libncurses-dev` (optional, for TUI)
- `pkg-config`

### Build Commands
- **Standard Build:** `make`
- **TUI Enabled Build:** `make with-tui`
- **Debug Build:** `make debug` (includes symbols and sanitizers)
- **Static Build:** `make static` (self-contained binary)
- **Train ML Model:** `make train` (builds the `bin/train_model` utility)
- **Clean Project:** `make clean`

### Installation
- **Install Globally:** `sudo make install`
- **Install Man Page:** `sudo make install-man`
- **Uninstall:** `sudo make uninstall`

## Basic Usage
```bash
# Banish null bytes (default)
byvalver input.bin output.bin

# Use a specific profile (e.g., HTTP newline-safe)
byvalver --profile http-newline input.bin output.bin

# Specify custom bad bytes
byvalver --bad-bytes "00,0a,0d" input.bin output.bin

# Enable Biphasic mode (Obfuscation + Denullification)
byvalver --biphasic input.bin output.bin

# Enable ML-powered strategy selection
byvalver --ml input.bin output.bin

# Launch Interactive TUI
byvalver --menu
```

## Project Structure
- `src/`: Core logic, CLI implementation, and TUI modules.
  - `*_strategies.c`: Individual transformation implementations.
  - `core.c`: The main processing engine and strategy selection logic.
  - `ml_strategist.c`: Neural network implementation and inference.
  - `tui/`: Ncurses-based terminal user interface.
- `ml_models/`: Directory for pre-trained binary model files.
- `assets/`: Project artifacts, documentation, and test shellcodes.
- `docs/`: Detailed deep-dives into obfuscation and denullification strategies.
- `verify_*.py`: Python scripts for post-processing validation.

## Development Conventions
- **Language Standard:** C99 for maximum portability and compatibility with low-level systems.
- **Strategy Pattern:** All transformations must implement the `strategy_t` interface defined in `strategy.h`.
- **Memory Management:** Strictly avoid leaks. Use `buffer_init`, `buffer_append`, and `buffer_free` for dynamic shellcode manipulation.
- **Error Handling:** Fail gracefully. Strategies that introduce bad bytes must be rolled back by the core engine.
- **Testing:** New features or strategies should be verified against the test corpus using `verify_denulled.py` and `verify_functionality.py`.
- **Formatting:** Mimic existing style—professional, direct, and avoiding unnecessary boilerplate.
