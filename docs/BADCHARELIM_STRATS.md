# Generic Bad-Character Elimination Framework (v3.0)

## Overview

**Version:** 3.0.0
**Status:** Functional, newly implemented (experimental for non-null characters)

BYVALVER v3.0 introduces a generic bad-character elimination framework that extends the tool's capabilities beyond null-byte removal. Users can now specify arbitrary bytes to eliminate via the `--bad-chars` command-line option.

**Important:** The 122+ transformation strategies were originally designed, tested, and optimized specifically for null-byte elimination. While they now support generic bad characters at the implementation level, their effectiveness for non-null byte scenarios has not been comprehensively validated.

## Architecture

### Core Data Structures

#### Bad Character Configuration (`bad_char_config_t`)

```c
typedef struct {
    uint8_t bad_chars[256];      // Bitmap: 1=bad, 0=ok (O(1) lookup)
    int bad_char_count;           // Number of bad characters
    uint8_t bad_char_list[256];   // List of bad byte values
} bad_char_config_t;
```

**Design Rationale:**
- **Bitmap Array**: O(1) constant-time lookup for any byte value
- **256-byte Size**: Covers entire byte space (0x00-0xFF)
- **Memory Efficient**: Only 512 bytes total (256 bitmap + 256 list)
- **Cache Friendly**: Fits in L1 cache for optimal performance

#### Global Context (`bad_char_context_t`)

```c
typedef struct {
    bad_char_config_t config;
    int initialized;
} bad_char_context_t;

extern bad_char_context_t g_bad_char_context;
```

**Design Rationale:**
- **Global Access**: Avoids threading configuration through 100+ functions
- **Zero Overhead**: No function parameter changes required
- **Initialization Flag**: Tracks whether context has been set
- **Single Source of Truth**: All strategies reference same configuration

### API Functions

#### Core Checking Functions

```c
// Check if single byte is free of bad characters
int is_bad_char_free_byte(uint8_t byte);

// Check if 32-bit value contains bad characters
int is_bad_char_free(uint32_t val);

// Check if buffer contains bad characters
int is_bad_char_free_buffer(const uint8_t *data, size_t size);
```

**Implementation:**
```c
int is_bad_char_free_byte(uint8_t byte) {
    if (!g_bad_char_context.initialized) {
        return byte != 0x00;  // Default: null-only
    }
    return g_bad_char_context.config.bad_chars[byte] == 0;
}
```

#### Context Management Functions

```c
// Initialize bad character context with configuration
void init_bad_char_context(bad_char_config_t *config);

// Reset context to default state
void reset_bad_char_context(void);

// Get current bad character configuration
bad_char_config_t* get_bad_char_config(void);
```

### Configuration Flow

**Propagation Path:**
1. `main()` → `parse_arguments()` → populates `config->bad_chars`
2. `main()` → `process_single_file()` → calls `init_bad_char_context(config->bad_chars)`
3. All strategies access via global context: `is_bad_char_free()`

**Initialization:**
```c
// In core.c
void init_bad_char_context(bad_char_config_t *config) {
    if (config) {
        memcpy(&g_bad_char_context.config, config, sizeof(bad_char_config_t));
        g_bad_char_context.initialized = 1;
    } else {
        // Default: null byte only
        memset(&g_bad_char_context, 0, sizeof(bad_char_context_t));
        g_bad_char_context.config.bad_chars[0x00] = 1;
        g_bad_char_context.config.bad_char_list[0] = 0x00;
        g_bad_char_context.config.bad_char_count = 1;
        g_bad_char_context.initialized = 1;
    }
}
```

## Command-Line Interface

### `--bad-chars` Option

**Syntax:**
```bash
byvalver --bad-chars "XX,YY,ZZ" input.bin output.bin
```

**Format:**
- Comma-separated hexadecimal byte values
- Each value must be 2 hex digits (00-FF)
- No `0x` prefix required
- Whitespace is trimmed

**Examples:**
```bash
# Null bytes only (default)
byvalver input.bin output.bin
byvalver --bad-chars "00" input.bin output.bin

# Null + newlines (network protocols)
byvalver --bad-chars "00,0a,0d" input.bin output.bin

# Null + space + tab (string safety)
byvalver --bad-chars "00,20,09" input.bin output.bin

# Multiple bad characters
byvalver --bad-chars "00,0a,0d,20,09" input.bin output.bin
```

### Parsing Implementation

**File:** `src/cli.c`

```c
bad_char_config_t* parse_bad_chars_string(const char *input) {
    // Parse comma-separated hex: "00,0a,0d" → {0x00, 0x0a, 0x0d}
    // Build bitmap and list
    // Default to {0x00} if empty
    // Validate: no duplicates, valid hex, 0x00-0xFF range
}
```

**Validation:**
- Checks for valid hex format
- Validates range (0x00-0xFF)
- Automatically deduplicates values
- Returns NULL on parse error

**Error Handling:**
```c
if (!config->bad_chars) {
    fprintf(stderr, "Error: Invalid --bad-chars format: %s\n", optarg);
    fprintf(stderr, "Expected format: \"00,0a,0d\" (comma-separated hex bytes)\n");
    return EXIT_INVALID_ARGUMENTS;
}
```

## Strategy Integration

### Strategy Updates

All 122+ strategies have been updated to use the generic API:

**Before (v2.x):**
```c
// Null-specific checking
if (insn->bytes[i] == 0x00) {
    // handle null byte
}

if (has_null_bytes(insn)) {
    // instruction contains nulls
}
```

**After (v3.0):**
```c
// Generic bad-character checking
if (!is_bad_char_free_byte(insn->bytes[i])) {
    // handle bad character
}

if (has_bad_chars_insn(insn)) {
    // instruction contains bad characters
}
```

### Backward Compatibility

**Deprecated Wrappers:**
```c
// Kept for compatibility with legacy code
int is_null_free_byte(uint8_t byte) {
    return is_bad_char_free_byte(byte);
}

int is_null_free(uint32_t val) {
    return is_bad_char_free(val);
}
```

**Function Rename:**
```c
// In strategy_registry.c
int has_null_bytes(cs_insn *insn) {
    // Updated in v3.0: Now checks for generic bad characters
    // Function name kept for backward compatibility with 100+ strategy files
    return !is_bad_char_free_buffer(insn->bytes, insn->size);
}
```

## How It Differs from Null-Byte Elimination

### Conceptual Differences

| Aspect | Null-Byte Elimination (v2.x) | Generic Bad-Character (v3.0) |
|--------|------------------------------|------------------------------|
| **Target** | Single byte: 0x00 | Arbitrary set of bytes |
| **Configuration** | Hardcoded | User-specified via `--bad-chars` |
| **Default** | Always null-only | Null-only if not specified |
| **Testing** | Extensively tested, 100% success | Functional but not validated |
| **Optimization** | Strategies optimized for null patterns | Strategies apply generically |
| **Use Case** | String safety, buffer overflows | Network protocols, input filters |

### Technical Differences

**Null-Byte Elimination:**
- Hardcoded check: `if (byte == 0x00)`
- Single byte to avoid
- Well-defined patterns (trailing zeros, ModR/M null bytes, etc.)
- Predictable transformation requirements

**Generic Bad-Character Elimination:**
- Bitmap check: `if (bad_chars[byte] == 1)`
- Configurable set of bytes to avoid
- Patterns vary based on bad character set
- Transformation requirements depend on character distribution

### Strategy Applicability

**Strategies Designed for Null Bytes:**
- MOV reg, 0x00000000 → strategies optimize for trailing zeros
- ModR/M byte 0x00 → strategies handle register encoding nulls
- Displacement 0x00000000 → strategies use SIB or register-based addressing

**Generic Bad Characters:**
- MOV reg, 0x0A0D0000 → may require different transformations
- ModR/M byte 0x0A → different register encoding constraints
- Displacement 0x0A0D0000 → may need different addressing strategies

## Use Cases

### Network Protocol Safety

Eliminate bytes that terminate network input:

```bash
# TCP protocol (null, newline, carriage return)
byvalver --bad-chars "00,0a,0d" payload.bin output.bin

# HTTP headers (null, newline, carriage return, space)
byvalver --bad-chars "00,0a,0d,20" payload.bin output.bin
```

**Common Bad Characters:**
- `0x00` - Null terminator
- `0x0a` - Line feed (LF, \n)
- `0x0d` - Carriage return (CR, \r)
- `0x20` - Space character

### C String Safety

Eliminate characters that C input functions treat specially:

```bash
# gets() safety (null, newline)
byvalver --bad-chars "00,0a" payload.bin output.bin

# scanf() safety (null, whitespace)
byvalver --bad-chars "00,20,09,0a,0d" payload.bin output.bin
```

### Custom Input Filters

Eliminate bytes filtered by custom input validation:

```bash
# Alphanumeric-only filter (eliminate non-alphanum)
# (Note: This would require listing all non-alphanum bytes)

# Custom application filter
byvalver --bad-chars "00,0a,0d,1a,00" payload.bin output.bin
```

## Current Limitations

### Strategy Optimization

**Null-Byte Patterns:**
- Strategies check for trailing zeros in immediate values
- Optimizations for common null patterns (0x00000000, 0x00000001, etc.)
- Special handling for ModR/M byte 0x00

**Generic Patterns:**
- Strategies apply same transformations regardless of which byte
- May not optimize for non-null specific patterns
- May generate longer output for some bad character combinations

### Testing Coverage

**Null-Byte Elimination:**
- 100% success rate on test suite (19/19 files)
- Tested across 116+ shellcode samples
- Validated against diverse real-world payloads
- Comprehensive edge case coverage

**Generic Bad-Character Elimination:**
- Framework is functional and operational
- Strategies updated to use generic API
- Limited testing with non-null bad character sets
- Real-world effectiveness not comprehensively validated

### ML Model Training

**Current State:**
- ML model trained exclusively on null-byte elimination data
- Feature extraction updated to track generic bad characters
- Model has not been retrained with diverse bad character datasets

**Impact:**
- ML mode may not perform optimally for non-null characters
- Strategy selection based on null-byte patterns
- Confidence scores calibrated for null elimination

**Recommendation:**
- Use standard mode (without `--ml`) for generic bad characters
- ML mode should only be used with default null-byte elimination

## Performance Characteristics

### Memory Usage

**Per-Process Overhead:**
- 512 bytes for bad_char_config_t (256 bitmap + 256 list)
- Negligible compared to typical shellcode processing

### Time Complexity

**Bad Character Checking:**
- O(1) for single byte check via bitmap
- O(n) for buffer check (n = buffer size)
- No degradation compared to null-byte checking

**Expected Performance Impact:**
- <5% overhead compared to null-byte only mode
- Actual measurements: 2-3% worst case
- Bitmap lookup is cache-friendly

### Output Size

**Null-Byte Elimination:**
- Typical expansion: 1.5x-3x original size
- Highly optimized transformations

**Generic Bad-Character Elimination:**
- Expansion depends on bad character distribution
- May be larger if many bad characters present
- Strategies not optimized for specific non-null patterns

## Validation and Verification

### C Verification

**File:** `src/core.c`

```c
int verify_bad_char_elimination(struct buffer *processed) {
    return is_bad_char_free_buffer(processed->data, processed->size);
}

// Backward compatibility
int verify_null_elimination(struct buffer *processed) {
    return verify_bad_char_elimination(processed);
}
```

### Python Verification

**File:** `verify_denulled.py`

```python
def parse_bad_chars(bad_chars_str):
    """Parse comma-separated hex string into set of bad byte values."""
    bad_chars = set()
    if not bad_chars_str:
        return {0x00}
    for part in bad_chars_str.split(','):
        part = part.strip()
        byte_val = int(part, 16)
        if 0 <= byte_val <= 255:
            bad_chars.add(byte_val)
    return bad_chars if bad_chars else {0x00}

def analyze_shellcode_for_bad_chars(shellcode_data, bad_chars=None):
    """
    Args:
        bad_chars: set of bad byte values (default: {0x00})
    Returns:
        {
            'total_bytes': total size,
            'bad_char_count': number of bad chars,
            'bad_char_percentage': percentage,
            'bad_char_positions': {byte_val: [positions]},
            'bad_char_sequences': [(start, length, bytes)],
            'bad_chars_used': bad_chars set
        }
    """
```

**Usage:**
```bash
python3 verify_denulled.py output.bin --bad-chars "00,0a,0d"
```

## Recommendations

### For Production Use

**✅ Recommended:**
- Use default mode (null-byte elimination only)
- No `--bad-chars` option or `--bad-chars "00"`
- Well-tested, 100% success rate
- Optimized transformations
- Proven effectiveness

**⚠️ Experimental:**
- Use `--bad-chars` with non-null values
- Test thoroughly before production deployment
- Validate output with verification tools
- Report any issues encountered

### For Testing

**Best Practices:**
1. Start with null-byte only mode to validate baseline
2. Add one bad character at a time
3. Verify output after each addition
4. Test with verification script
5. Compare output size and functionality

**Example Workflow:**
```bash
# Step 1: Baseline (null-only)
byvalver input.bin output1.bin
python3 verify_denulled.py output1.bin --bad-chars "00"

# Step 2: Add newline
byvalver --bad-chars "00,0a" input.bin output2.bin
python3 verify_denulled.py output2.bin --bad-chars "00,0a"

# Step 3: Add carriage return
byvalver --bad-chars "00,0a,0d" input.bin output3.bin
python3 verify_denulled.py output3.bin --bad-chars "00,0a,0d"
```

### For Development

**Contributing Strategy Improvements:**
1. Identify patterns specific to your bad character set
2. Design transformations optimized for those patterns
3. Implement as new strategies or enhance existing ones
4. Test with diverse shellcode samples
5. Submit pull requests with comprehensive documentation

## Future Enhancements

### Planned Improvements

1. **Strategy Optimization:**
   - Identify common non-null bad character patterns
   - Optimize transformations for newline elimination
   - Special handling for common bad character sets

2. **ML Model Retraining:**
   - Collect training data with varied bad character sets
   - Retrain neural network with diverse patterns
   - Improve strategy selection for generic cases

3. **Expanded Testing:**
   - Comprehensive test suite for non-null characters
   - Real-world payload validation
   - Edge case identification and coverage

4. **Performance Tuning:**
   - Profile performance with various bad character sets
   - Optimize hot paths for generic checking
   - Reduce output size expansion

### Research Directions

1. **Automated Strategy Discovery:**
   - Analyze shellcode for pattern-specific transformations
   - Automatically generate strategies for common bad characters
   - Machine learning for optimal strategy selection

2. **Hybrid Approaches:**
   - Combine multiple encoding techniques
   - Adaptive strategy selection based on bad character distribution
   - Context-aware transformations

## Technical Reference

### Files Modified in v3.0

**Core Infrastructure:**
- `src/cli.h` - Added bad_char_config_t structure
- `src/core.h` - Added global context declarations
- `src/core.c` - Implemented context management
- `src/utils.h` - Added generic function prototypes
- `src/utils.c` - Implemented checking functions
- `src/cli.c` - Added parsing logic
- `src/main.c` - Added context initialization
- `src/strategy_registry.c` - Updated has_null_bytes()

**Strategy Updates:**
- 122+ strategy files updated to use generic API
- Bulk update: `is_null_free()` → `is_bad_char_free()`
- All inline null checks updated to generic checks

**Verification:**
- `verify_denulled.py` - Complete rewrite for generic support

### Key Commits

Reference implementation commits:
- Infrastructure (Phase 1): Core data structures and API
- CLI Integration (Phase 2): Parsing and configuration
- Core System (Phase 3): Processing pipeline updates
- Strategy Updates (Phase 4): Bulk strategy refactoring
- Verification (Phase 5): Python tool updates
- ML Integration (Phase 6): Feature extraction updates

## Conclusion

The generic bad-character elimination framework in BYVALVER v3.0 extends the tool's capabilities beyond null-byte removal. While the framework is fully functional and operational, users should be aware that:

- **Null-byte elimination** remains the primary, well-tested use case
- **Generic bad-character elimination** is newly implemented and experimental
- **Strategies** were designed for null-byte patterns and may require optimization for other characters
- **Testing** is recommended before production use with non-null bad character sets

The framework provides a solid foundation for future enhancements and community contributions to improve support for diverse bad character elimination scenarios.
