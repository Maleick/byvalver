<div align="center">
  <h1>byvalver</h1>
  <p><b>NULL-BYTE ELIMINATION FRAMEWORK</b></p>

  <img src="./images/byvalver_logo.png" alt="byvalver logo" width="400">
</div>

<div align="center">

  ![C](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white)
  &nbsp;
  ![Shellcode](https://img.shields.io/badge/Shellcode-Analysis-%23FF6B6B.svg?style=for-the-badge)
  &nbsp;
  ![Cross-Platform](https://img.shields.io/badge/Cross--Platform-Windows%20%7C%20Linux%20%7C%20macOS-%230071C5.svg?style=for-the-badge)
  &nbsp;
  ![Security](https://img.shields.io/badge/Security-Hardened-%23000000.svg?style=for-the-badge)

</div>

<p align="center">
  <a href="#overview">Overview</a> •
  <a href="#features">Features</a> •
  <a href="#building-and-setup">Setup</a> •
  <a href="#usage-guide">Usage</a> •
  <a href="#architecture">Architecture</a> •
  <a href="#development">Development</a> •
  <a href="#troubleshooting">Troubleshooting</a>
</p>

<hr>

<br>

## OVERVIEW

**byvalver** is an advanced C-based command-line tool designed for automated removal of null bytes from shellcode while preserving functional equivalence. The tool leverages the Capstone disassembly framework to analyze x86/x64 assembly instructions and applies sophisticated transformation strategies to replace null-containing instructions with functionally equivalent alternatives.

**Primary Function:** Remove null bytes (`\\x00`) from binary shellcode that would otherwise cause issues with string-based operations or memory management routines.

**Technology Stack:**
- C language implementation for performance and low-level control
- Capstone Disassembly Framework for instruction analysis
- NASM assembler for building decoder stubs
- x86/x64 assembly and instruction set knowledge
- Modular strategy pattern for extensible transformations

---

**byvalver**:

1. **ANALYZES** x86/x64 assembly instructions with Capstone
2. **IDENTIFIES** instructions containing null bytes
3. **TRANSFORMS** instructions to null-byte-free equivalents
4. **OUTPUTS** clean shellcode in multiple formats (binary, XOR-encoded, PIC)

`byvalver` tool prioritizes `security`, `robustness`, and `portability`, running seamlessly on Windows, Linux, and macOS.

<br>

## NEW STRATEGIES

### MOV reg, [reg] Null-Byte Elimination Strategy

**New in v2.1**: BYVALVER now includes specialized transformation strategies for the common `mov reg, [reg]` pattern that produces null bytes, such as `mov eax, [eax]` (opcode `8B 00`). This instruction pattern creates null bytes in the ModR/M byte, which our new strategy eliminates by using a temporary register with displacement arithmetic:

```assembly
push temp_reg      ; Save temporary register
lea temp_reg, [src_reg - 1]  ; Load effective address with non-null displacement
mov dest_reg, [temp_reg + 1] ; Dereference the correct address
pop temp_reg       ; Restore temporary register
```

This transformation preserves all registers (except flags) and eliminates the null-byte ModR/M encoding.

### ADD [mem], reg8 Null-Byte Elimination Strategy

**New in v2.1**: BYVALVER now handles the `add [mem], reg8` pattern that creates null bytes, such as `add [eax], al` (opcode `00 00`). This instruction encodes null bytes in both the opcode and ModR/M byte. The new strategy replaces it with a null-byte-free sequence:

```assembly
push temp_reg                 ; Save temporary register
movzx temp_reg, byte ptr [mem] ; Load the byte from memory into temp register
add temp_reg, src_reg8        ; Perform the addition
mov byte ptr [mem], temp_reg  ; Store the result back into memory
pop temp_reg                  ; Restore temporary register
```

This transformation uses null-byte-free instructions to achieve the same result.

### Disassembly Validation Enhancement

**New in v2.1**: Added robust validation to detect invalid shellcode input. If Capstone disassembler returns zero instructions, BYVALVER now provides a clear error message instead of proceeding with invalid data.

### Automatic Output Directory Creation

**New in v2.1.1**: BYVALVER now automatically creates parent directories for output files, similar to `mkdir -p` behavior. This eliminates "No such file or directory" errors when specifying output paths with non-existent directories.

**Features:**
- **Automatic Creation**: Parent directories are created recursively as needed
- **Deep Nesting**: Supports deeply nested directory structures (e.g., `results/2025/december/processed/output.bin`)
- **Improved Error Messages**: Clear, detailed error messages showing the exact file path and reason for failures
- **Zero Configuration**: No manual directory creation required before processing

**Example:**
```bash
# Automatically creates results/processed/ directory structure
byvalver input.bin results/processed/output.bin

# Deep nesting also works
byvalver input.bin data/experiments/2025/run_001/output.bin
```

This quality-of-life improvement streamlines batch processing workflows and eliminates manual directory setup steps.

## BATCH DIRECTORY PROCESSING

**New in v2.2**: BYVALVER now includes comprehensive batch directory processing with full compatibility for all existing options.

### New Command-Line Options

- `-r, --recursive` - Process directories recursively
- `--pattern PATTERN` - File pattern to match (default: *.bin)
- `--no-preserve-structure` - Flatten output (don't preserve directory structure)
- `--continue-on-error` - Continue processing even if some files fail

### Auto-Detection

Batch mode is automatically enabled when the input is a directory:

```bash
# Single file mode (automatic)
byvalver input.bin output.bin

# Batch mode (automatic)
byvalver input_dir/ output_dir/
```

### Compatibility with All Existing Options

All options work seamlessly with batch processing:
- `--biphasic` - Applies biphasic processing to all files
- `--pic` - Generates PIC code for all files
- `--ml` - Uses ML strategy selection for all files
- `--xor-encode KEY` - XOR encodes all processed files
- `--metrics, --metrics-json, --metrics-csv` - Aggregates metrics across all files
- `--quiet, --verbose` - Controls output level for batch operations
- `--dry-run` - Validates all files without processing

### Usage Examples

Process all .bin files in a directory (non-recursive):
```bash
byvalver shellcodes/ output/
```

Process recursively with all subdirectories:
```bash
byvalver -r shellcodes/ output/
```

Process only .txt files recursively:
```bash
byvalver -r --pattern "*.txt" input/ output/
```

Process with biphasic mode and XOR encoding:
```bash
byvalver -r --biphasic --xor-encode 0x12345678 input/ output/
```

Flatten output (don't preserve directory structure):
```bash
byvalver -r --no-preserve-structure input/ output/
```

Continue processing even if some files fail:
```bash
byvalver -r --continue-on-error input/ output/
```

### Implementation Details

**New Files:**
- `src/batch_processing.h` - Batch processing API
- `src/batch_processing.c` - Directory traversal, file discovery, and statistics

**Key Features:**
- **Automatic directory creation** - Parent directories created automatically (preserves existing functionality)
- **Pattern matching** - Uses fnmatch for flexible file pattern matching
- **Directory structure preservation** - Optional preservation of input directory hierarchy
- **Comprehensive statistics** - Tracks total files, processed, failed, skipped, and size metrics
- **Progress reporting** - Shows progress as [N/Total] filename
- **Error handling** - Configurable error handling (stop or continue on errors)

### Tested Scenarios

✅ Non-recursive batch processing
✅ Recursive batch processing
✅ Directory structure preservation
✅ Flattened output (--no-preserve-structure)
✅ Custom file patterns (--pattern)
✅ Compatibility with --biphasic
✅ Compatibility with --xor-encode
✅ Error handling with --continue-on-error
✅ Empty file handling

All existing single-file functionality remains unchanged and fully compatible!

<br>

## ML METRICS ENHANCEMENT

**New in v2.2.1**: BYVALVER now properly records ML predictions in metrics, fixing the "Predictions Made: 0" issue.

### Problem Fixed

Previous versions showed "Predictions Made: 0" in ML metrics even when processing thousands of instructions, because the neural network was used for strategy reprioritization but predictions weren't being recorded.

### Solution Implemented

- **Prediction Recording**: `ml_get_strategy_recommendation` now properly calls `ml_metrics_record_prediction`
- **Metrics Tracking**: Each ML-based strategy selection is now recorded as a prediction
- **Accuracy Reporting**: Metrics now accurately reflect ML model usage and performance
- **Performance Tracking**: Proper tracking of ML feedback iterations and learning progress

### Before vs After

**Before Fix:**
```
=== ML STRATEGIST PERFORMANCE SUMMARY ===
Instructions Processed: 14623
Strategies Applied: 13025
Null Bytes Eliminated: 12518 / 14623 (0.00%)

--- Model Performance ---
Predictions Made: 0
Current Accuracy: 0.00%
```

**After Fix:**
```
=== ML STRATEGIST PERFORMANCE SUMMARY ===
Instructions Processed: 14623
Strategies Applied: 13025
Null Bytes Eliminated: 12518 / 14623 (0.00%)

--- Model Performance ---
Predictions Made: 14623
Current Accuracy: 94.23%
```

The ML strategist now properly tracks and reports prediction metrics for enhanced analysis and debugging.

<br>

## IMPROVED ML PREDICTION ACCURACY

**New in v2.3**: BYVALVER now accurately tracks ML prediction outcomes, fixing issues with prediction confidence and accuracy reporting.

### Problem Fixed

Previous versions inaccurately reported prediction accuracy as 100% with 0.0000 confidence because predictions were recorded as "successful" when made, not when outcomes were known.

### Solution Implemented

- **Outcome-Based Recording**: ML predictions are now tracked when made and resolved when outcomes are known
- **Accurate Confidence Calculation**: Average prediction confidence now reflects actual ML model confidence values
- **Real Accuracy Metrics**: Prediction accuracy now reflects true success rate of ML-recommended strategies
- **Proper Learning Feedback**: ML model receives accurate feedback about prediction success/failure

### Implementation Details

- **Prediction Tracking**: When ML makes a recommendation, it's stored in a pending prediction buffer
- **Outcome Resolution**: When strategy result is known, the corresponding prediction is resolved with actual outcome
- **Metrics Updates**: Prediction accuracy and confidence are calculated based on actual results, not assumptions

### Before vs After

**Before Fix:**
```
=== ML STRATEGIST PERFORMANCE SUMMARY ===
--- Model Performance ---
Current Accuracy: 100.00%
Accuracy Improvement: +0.00%
Avg Prediction Confidence: 0.0000
```

**After Fix:**
```
=== ML STRATEGIST PERFORMANCE SUMMARY ===
--- Model Performance ---
Current Accuracy: 97.68%
Accuracy Improvement: -2.32%
Avg Prediction Confidence: 0.7421
```

The ML strategist now provides realistic metrics that accurately reflect model performance and improvement over time.

<br>

## BUILDING AND SETUP

### DEPENDENCIES

- A C compiler (`gcc`, `clang`, or MSVC)
- [Capstone disassembly library](http://www.capstone-engine.org/) with development headers
- NASM assembler (`nasm`)
- `xxd` utility (usually part of `vim-common` package)
- Make

### INSTALLATION OF DEPENDENCIES

```bash
# Ubuntu/Debian
sudo apt install build-essential nasm xxd pkg-config libcapstone-dev

# macOS with Homebrew
brew install capstone nasm

# Or manually install Capstone from https://github.com/capstone-engine/capstone
```

### BUILDING

**RECOMMENDED: Makefile Build**

```bash
# Build the main executable (default)
make

# Build with debug symbols and sanitizers
make debug

# Build optimized release version
make release

# Build static executable
make static

# Clean build artifacts
make clean

# Show build information
make info
```

### GLOBAL INSTALLATION

**1. QUICK INSTALLATION**

```bash
# Install using the installation script
curl -sSL https://raw.githubusercontent.com/mrnob0dy666/byvalver/main/install.sh | bash

# Or download and run the script manually
curl -O https://raw.githubusercontent.com/mrnob0dy666/byvalver/main/install.sh
chmod +x install.sh
./install.sh
```

**2. FROM SOURCE**

```bash
# Clone the repository