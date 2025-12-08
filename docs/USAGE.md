# BYVALVER Usage Guide

## Overview

BYVALVER is an advanced command-line tool for automated removal of null bytes from shellcode while preserving functional equivalence. The tool leverages the Capstone disassembly framework to analyze x86/x64 assembly instructions and applies sophisticated transformation strategies.

## What's New in v2.1.1

### Automatic Output Directory Creation

BYVALVER now automatically creates parent directories for output files, eliminating the need for manual directory setup:

**Features:**
- **Recursive Creation**: Automatically creates entire directory paths as needed
- **Deep Nesting Support**: Handles complex directory structures like `data/experiments/2025/batch_001/output.bin`
- **mkdir -p Behavior**: Works similar to Unix `mkdir -p` command
- **Improved Error Messages**: Shows exact file paths and specific error reasons when failures occur

**Example:**
```bash
# These commands now work without pre-creating directories
byvalver input.bin results/processed/output.bin
byvalver input.bin experiments/2025/december/run_042/shellcode.bin
```

**Benefits:**
- No more "No such file or directory" errors for output files
- Streamlines batch processing workflows
- Reduces manual directory management
- Ideal for automated scripts and pipelines

## What's New in v2.2

### Batch Directory Processing

**New in v2.2**: BYVALVER now includes comprehensive batch directory processing with full compatibility for all existing options.

#### New Command-Line Options

- `-r, --recursive` - Process directories recursively
- `--pattern PATTERN` - File pattern to match (default: *.bin)
- `--no-preserve-structure` - Flatten output (don't preserve directory structure)
- `--no-continue-on-error` - Stop processing on first error (default is to continue)

#### Auto-Detection

Batch mode is automatically enabled when the input is a directory:

```bash
# Single file mode (automatic)
byvalver input.bin output.bin

# Batch mode (automatic)
byvalver input_dir/ output_dir/
```

#### Compatibility with All Existing Options

All options work seamlessly with batch processing:
- `--biphasic` - Applies biphasic processing to all files
- `--pic` - Generates PIC code for all files
- `--ml` - Uses ML strategy selection for all files
- `--xor-encode KEY` - XOR encodes all processed files
- `--metrics, --metrics-json, --metrics-csv` - Aggregates metrics across all files
- `--quiet, --verbose` - Controls output level for batch operations
- `--dry-run` - Validates all files without processing

#### Usage Examples

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

Stop processing on first error (default is to continue):
```bash
byvalver -r --no-continue-on-error input/ output/
```

#### Implementation Details

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

#### Tested Scenarios

✅ Non-recursive batch processing
✅ Recursive batch processing
✅ Directory structure preservation
✅ Flattened output (--no-preserve-structure)
✅ Custom file patterns (--pattern)
✅ Compatibility with --biphasic
✅ Compatibility with --xor-encode
✅ Error handling with --no-continue-on-error
✅ Empty file handling

All existing single-file functionality remains unchanged and fully compatible!

## What's New in v2.2.1

### ML Prediction Tracking System

**New in v2.2.1**: BYVALVER now properly tracks and records ML predictions with outcome-based accuracy metrics.

#### Problem Fixed

Previous versions showed "Predictions Made: 0" in ML metrics even when the neural network was actively reprioritizing strategies. The ML model was performing inference and learning from feedback, but predictions were never being recorded in the metrics system.

**Root Cause:** The `ml_reprioritize_strategies()` function performed neural network inference to reorder strategies but never called `ml_metrics_record_prediction()`. Learning happened (feedback iterations were tracked), but prediction counts remained at zero.

#### Solution Implemented

A comprehensive three-component prediction tracking system:

##### 1. **Prediction State Tracking**
Global state variables track the ML model's prediction for each instruction:
```c
static strategy_t* g_last_predicted_strategy = NULL;
static double g_last_prediction_confidence = 0.0;
```

##### 2. **Prediction Storage**
When the ML model reprioritizes strategies (`ml_reprioritize_strategies()`):
- The top-ranked strategy (index 0) is stored as the prediction
- The ML confidence score for that strategy is saved
- Prediction is held in pending state until outcome is known

##### 3. **Outcome-Based Recording**
When strategy application completes (`ml_provide_feedback()`):
- Compares applied strategy against stored prediction
- Determines if prediction was correct:
  - **Correct**: Predicted strategy was used AND succeeded
  - **Incorrect**: Different strategy used OR predicted strategy failed
- Records prediction with actual outcome via `ml_metrics_record_prediction()`
- Clears prediction state for next instruction

#### Implementation Architecture

**Key Files Modified:**
- `src/ml_strategist.c:39-41` - Added prediction state variables
- `src/ml_strategist.c:398-407` - Store predictions in `ml_reprioritize_strategies()`
- `src/ml_strategist.c:537-562` - Verify and record predictions in `ml_provide_feedback()`

**Processing Flow:**
```
Instruction Analysis
       ↓
ML Neural Network Inference (reprioritize)
       ↓
Store Top Strategy + Confidence
       ↓
Apply Strategy
       ↓
Compare Outcome vs Prediction
       ↓
Record Prediction (correct/incorrect)
       ↓
Update Accuracy Metrics
```

#### Before vs After

**Before Fix:**
```
=== ML STRATEGIST PERFORMANCE SUMMARY ===
Instructions Processed: 3702
Strategies Applied: 3693
Null Bytes Eliminated: 3693 / 3702 (99.76%)

--- Model Performance ---
Predictions Made: 0          ← No predictions recorded
Current Accuracy: 0.00%      ← No accuracy tracking
Avg Prediction Confidence: 0.0000

--- Learning Progress ---
Learning Enabled: YES
Total Feedback Iterations: 7395  ← Learning WAS happening
Positive Feedback: 3693
```

**After Fix:**
```
=== ML STRATEGIST PERFORMANCE SUMMARY ===
Instructions Processed: 4
Strategies Applied: 4
Null Bytes Eliminated: 4 / 4 (100.00%)

--- Model Performance ---
Predictions Made: 4          ← Predictions now tracked
Current Accuracy: 100.00%    ← Real accuracy metrics
Avg Prediction Confidence: 0.0062  ← Actual confidence values

--- Learning Progress ---
Learning Enabled: YES
Total Feedback Iterations: 8
Positive Feedback: 4
```

#### Benefits

- **Accurate Metrics**: Prediction counts now reflect actual ML model usage
- **Real Accuracy**: Accuracy percentages based on actual prediction outcomes
- **Confidence Tracking**: Average confidence reflects true ML model confidence scores
- **Learning Validation**: Confirms ML model is making predictions, not just learning
- **Performance Analysis**: Enables evaluation of ML model effectiveness over time
- **Debugging Support**: Provides visibility into ML decision-making process

## What's New in v2.5

### Windows-Specific Denull Strategies

**New in v2.5**: BYVALVER now includes 10 new Windows-specific denull strategies identified from analysis of real Windows shellcode patterns:

#### CALL/POP Immediate Loading Strategy
- **Description**: Use the CALL/POP technique to load immediate values that contain null bytes by pushing the value onto the stack and retrieving it without directly encoding the nulls.
- **Usage**: Automatically applied to MOV reg, imm32 where immediate contains null bytes.
- **Example**: MOV EAX, 0x00123456 → PUSH 0x00123456; POP EAX (with null-free construction)

#### PEB API Hashing Strategy
- **Description**: Use PEB (Process Environment Block) traversal to find kernel32.dll base address, then use hash-based API resolution to call functions without hardcoded addresses containing nulls.
- **Usage**: Automatically applied to CALL immediate_address where address contains null bytes.
- **Example**: CALL 0x7C86114D → PEB traversal + hash-based resolution + call via register

#### SALC + Conditional Flag Manipulation Strategy
- **Description**: Use SALC (Set AL on Carry) instruction combined with flag manipulation to set AL register efficiently, avoiding MOV AL, 0x00.
- **Usage**: Automatically applied to MOV AL, 0x00 or MOV AL, 0xFF patterns.
- **Example**: MOV AL, 0x00 → CLC; SALC (F8 D6 - no nulls)

#### LEA Arithmetic Substitution Strategy
- **Description**: Use LEA (Load Effective Address) instruction to perform arithmetic operations like addition and multiplication without using immediate values that contain nulls.
- **Usage**: Automatically applied to ADD reg, imm32 / SUB reg, imm32 where immediate contains null bytes.
- **Example**: ADD EAX, 0x00000040 → LEA EAX, [EAX + 0x40]

#### Shift-Based Value Construction Strategy
- **Description**: Use bit shift operations combined with arithmetic to construct values that contain null bytes in their direct encoding.
- **Usage**: Automatically applied to MOV reg, imm32 where immediate contains null bytes that could be constructed via shifts.
- **Example**: MOV EAX, 0x00200000 → XOR EAX, EAX; MOV AL, 0x20; SHL EAX, 12

#### Stack-Based String Construction Strategy
- **Description**: Construct strings on the stack using multiple PUSH operations with non-null byte chunks, avoiding direct string literals.
- **Usage**: Automatically applied to PUSH operations with immediate values representing string constants containing null bytes.
- **Example**: PUSH 0x00646D63 (pushing "cmd\0") → multiple PUSH operations of non-null chunks

#### Enhanced Immediate Encoding Strategies
- **Description**: Multiple new approaches for immediate value construction including byte-by-byte construction and alternative displacement handling.
- **Usage**: Applied to various MOV and arithmetic operations with null-containing immediate values.

#### Register Swapping with Immediate Loading Strategy
- **Description**: Use register exchange operations (XCHG) to load immediate values by first loading null-free partial values and then exchanging them.
- **Usage**: Applied contextually when register swapping is appropriate.

#### Alternative LEA Complex Displacement Strategy
- **Description**: Alternative approach to handle LEA and MOV instructions with null-containing displacements using complex addressing.
- **Usage**: Applied to MOV reg, [disp32] where displacement contains null bytes.

#### String Instruction Byte Construction Strategy
- **Description**: Use STOSB, STOSD, or similar string instructions with loops to construct immediate values containing nulls in memory rather than through direct immediate encoding.
- **Usage**: Applied to MOV reg, imm32 where immediate contains null bytes.

## What's New in v2.4

### Comprehensive Strategy Repair

**New in v2.4**: BYVALVER now includes comprehensive fixes for critical bugs across 15+ transformation strategies that showed 0% success rates despite high attempt counts.

#### The Critical Root Causes and Fixes:

**Issue 1: Register Indexing Problems Across Multiple Files**
- **Problem**: Many strategies used `reg - X86_REG_EAX` instead of `get_reg_index(reg)` causing improper register encoding
- **Impact**: Strategies like `generic_mem_null_disp`, `mov_mem_disp_null`, and others failed due to incorrect MOD/RM byte construction
- **Fix**: Replaced all occurrences with proper `get_reg_index()` function
- **Files Affected**: `src/jump_strategies.c`, `src/memory_displacement_strategies.c`, `src/cmp_strategies.c`, `src/syscall_number_strategies.c`

## What's New in v2.5

### ML Training Integration

**New in v2.5**: BYVALVER now includes a dedicated training utility and enhanced ML integration with path resolution capabilities.

#### Training Utility

A standalone `train_model` utility has been added to train the ML model on custom shellcode datasets:

```bash
# Build the training utility
make train

# Run training (defaults to shellcodes/ directory and ml_models/ output)
./bin/train_model
```

The training utility includes:
- Data generation from shellcode files in the `./shellcodes/` directory
- Neural network training with configurable parameters
- Model evaluation and performance validation
- Statistics reporting and export

#### Enhanced Path Resolution

The ML model loading now uses enhanced path resolution that dynamically determines the model file location based on the executable path:

- Uses `readlink` on `/proc/self/exe` to determine the executable location
- Constructs the model path relative to the executable location
- Includes fallback to default path if path resolution fails
- Works correctly regardless of where the executable is moved

#### Training Configuration

The training process includes configurable parameters:
- **Training Data Directory**: Defaults to `./shellcodes/`
- **Model Output Path**: Defaults to `./ml_models/byvalver_ml_model.bin`
- **Max Training Samples**: Configurable, default 10,000
- **Training Epochs**: Configurable, default 50
- **Validation Split**: Configurable, default 20% for validation
- **Learning Rate**: Configurable, default 0.001
- **Batch Size**: Configurable, default 32

#### Usage Examples

Build the main executable and training utility:
```bash
# Build main executable
make

# Build training utility
make train
```

Train the ML model:
```bash
# Run training with default configuration
./bin/train_model

# The training utility will:
# - Process shellcode files from ./shellcodes/
# - Train the neural network with 50 epochs
# - Validate on 20% of the data
# - Save the trained model to ./ml_models/
# - Generate training statistics report
```

#### ML Model Integration

The main `byvalver` executable now includes:
- Dynamic model path resolution at runtime
- Fallback to default weights if model file is not found
- Enhanced error reporting for model loading issues
- Path-independent operation regardless of executable location

The ML functionality can be enabled with the `--ml` option:
```bash
# Enable ML-powered strategy selection
byvalver --ml input.bin output.bin

# Works with all other options including batch processing
byvalver -r --ml --biphasic input/ output/
```