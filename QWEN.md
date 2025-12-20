# byvalver Project Context

## Project Overview

**byvalver** is a sophisticated CLI tool built in C for automatically eliminating null-bytes (`\x00`) and other bad characters from x86/x64 shellcode while maintaining complete functional equivalence. The tool uses the Capstone disassembly framework to analyze instructions and applies over 122 ranked transformation strategies to replace null-containing code with equivalent alternatives.

### Key Features

- **Null-byte elimination**: The core functionality with 100% success rate on diverse test corpus
- **Generic bad character elimination**: v3.0+ feature supporting arbitrary bad character sets
- **Bad character profiles**: Pre-configured sets for common exploit scenarios (HTTP, SQL injection, etc.)
- **Biphasic processing**: Obfuscation followed by denullification
- **ML-powered strategy selection**: Experimental neural network for intelligent strategy selection
- **Batch processing**: Recursive directory traversal with custom file patterns
- **Multiple output formats**: Raw binary, C array, Python bytes, hex string
- **XOR encoding**: With decoder stub generation
- **Cross-platform**: Supports Windows, Linux, and macOS

## Architecture

The project follows a modular strategy-pattern design with:

1. **Pass 1**: (Optional) Obfuscation for anti-analysis using `--biphasic` mode
2. **Pass 2**: Denullification for null-byte removal using 122+ transformation strategies
3. **ML layer**: For strategy optimization (experimental)
4. **Batch system**: For scalable processing

### Core Components

- **Strategy Registry**: Over 122 transformation strategies for different instruction types
- **ML System**: Neural network for intelligent strategy selection (v2.0 architecture)
- **Bad Character Context**: O(1) bitmap lookup for bad character identification
- **Batch Processing**: Directory traversal and file processing system

## Building and Running

### System Requirements
- **OS**: Linux (Ubuntu/Debian/Fedora), macOS (with Homebrew), Windows (via WSL/MSYS2)
- **CPU**: x86/x64 with modern instructions
- **RAM**: 1GB free
- **Disk**: 50MB free
- **Tools**: C compiler, Make, Git

### Dependencies
- **Core**: GCC/Clang, GNU Make, Capstone (v4.0+), NASM (v2.13+), xxd
- **Optional**: Clang-Format, Cppcheck, Valgrind

### Installation Commands
**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential nasm xxd pkg-config libcapstone-dev clang-format cppcheck valgrind
```

**macOS (Homebrew):**
```bash
brew install capstone nasm pkg-config
```

### Building
Use the Makefile for builds:
- Default: `make` (optimized executable)
- Debug: `make debug` (symbols, sanitizers)
- Release: `make release` (-O3, native)
- Static: `make static` (self-contained)
- ML Trainer: `make train` (bin/train_model)
- Clean: `make clean` or `make clean-all`

### Installation
Global install:
```bash
sudo make install
sudo make install-man
```

## Usage

```bash
byvalver [OPTIONS] <input> [output]
```

### Key Options
- `-h, --help`: Help
- `-v, --version`: Version
- `-V, --verbose`: Verbose
- `-q, --quiet`: Quiet
- `--bad-chars BYTES`: Comma-separated hex bytes to eliminate (default: "00")
- `--profile NAME`: Use predefined bad-character profile (e.g., http-newline, sql-injection)
- `--list-profiles`: List all available bad-character profiles
- `--biphasic`: Obfuscate + denull
- `--pic`: Position-independent
- `--ml`: ML strategy selection
- `--xor-encode KEY`: XOR with stub
- `--format FORMAT`: raw|c|python|hexstring
- `-r, --recursive`: Recursive batch
- `--pattern PATTERN`: File glob
- `--no-preserve-structure`: Flatten output
- `--no-continue-on-error`: Stop on error

### Examples
```bash
# Default: eliminate null bytes only (well-tested, recommended)
byvalver shellcode.bin clean.bin

# v3.0 NEW: List available bad-character profiles
byvalver --list-profiles

# v3.0 NEW: Use predefined profile for HTTP contexts (eliminates 0x00, 0x0A, 0x0D)
byvalver --profile http-newline shellcode.bin clean.bin

# v3.0 NEW: Use profile for SQL injection contexts
byvalver --profile sql-injection shellcode.bin clean.bin

# v3.0 NEW: Manual bad-char specification (experimental - not extensively tested)
byvalver --bad-chars "00,0a,0d" shellcode.bin clean.bin

# Combined with other features
byvalver --profile http-newline --biphasic --ml input.bin output.bin

# Batch processing with profile
byvalver -r --profile http-whitespace --pattern "*.bin" shellcodes/ output/
```

## Development Conventions

### Code Structure
- **src/**: All C source and header files organized by functionality
- **Strategy files**: Named with `_strategies.c` suffix for transformation strategies
- **Obfuscation files**: Named with `_obfuscation.c` suffix
- **ML files**: Named with `ml_` prefix for machine learning components

### Strategy Pattern
The project uses a modular strategy pattern where each strategy implements:
- `can_handle()`: Check if strategy can handle an instruction
- `get_size()`: Calculate new instruction size
- `generate()`: Generate new code
- `priority`: Strategy selection priority

### ML Architecture v2.0
- One-hot instruction encoding (51 dims) replaces scalar instruction IDs
- Context window with sliding buffer of 4 instructions
- Fixed feature extraction with stable 84-dimensional layout
- 3-layer feedforward neural network (336→512→200)
- Output masking filters invalid strategies before softmax

### Testing and Verification
- `verify_denulled.py`: Ensures zero bad characters
- `verify_functionality.py`: Checks execution patterns
- `verify_semantic.py`: Validates equivalence
- Batch processing with continue-on-error or strict modes

## File Structure
```
byvalver/
├── bin/                    # Compiled binaries
├── docs/                   # Documentation
├── images/                 # Project images and diagrams
├── ml_models/              # ML model files
├── scripts/                # Utility scripts
├── shellcodes/             # Test shellcode samples
├── src/                    # Source code files
├── tests/                  # Test files
├── training/               # ML training data
├── verify_semantic/        # Semantic verification tools
├── decoder.asm             # Assembly decoder stub
├── install.sh              # Installation script
├── Makefile                # Build configuration
├── README.md               # Main documentation
├── verify_denulled.py      # Bad character verification
├── verify_functionality.py # Functional verification
└── verify_semantic.py      # Semantic verification
```

## Important Notes

1. **Null-byte elimination** (default behavior) is well-tested with 100% success rate
2. **Generic bad-character elimination** is functional but experimental in v3.0
3. **ML mode** is experimental and requires retraining with diverse datasets
4. Always verify transformed shellcode using the provided verification tools
5. The project includes comprehensive batch processing capabilities for large shellcode collections
6. The strategy system is extensible, allowing easy addition of new transformation techniques