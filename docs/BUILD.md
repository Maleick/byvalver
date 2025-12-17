# BYVALVER Build Guide

**Version:** 3.0.0

## System Requirements

### Operating Systems
- **Linux** (Ubuntu, Debian, Fedora, etc.)
- **macOS** with Homebrew package manager
- **Windows** with WSL or MSYS2 (not directly supported but possible)

### Minimum System Specifications
- **CPU**: x86/x64 processor with support for modern instruction sets
- **RAM**: 1GB free memory (for processing typical shellcode files)
- **Disk Space**: 50MB free space for build and dependencies
- **Build Tools**: Standard C development environment

## Dependencies

### Core Dependencies
- **C Compiler**: GCC or Clang (C99 compliant)
- **Make**: GNU Make for build automation
- **Git**: For version control (recommended)

### Required Libraries and Tools
- **Capstone Disassembly Framework**: Version 4.0 or higher
- **NASM Assembler**: Version 2.13 or higher for decoder stub generation
- **xxd utility**: Part of Vim package, for binary-to-hex conversion

### Optional Dependencies for Advanced Features
- **Clang-Format**: For automatic code formatting
- **Cppcheck**: For static analysis
- **Valgrind**: For memory leak detection (in debug builds)

## Installation of Dependencies

### On Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential nasm xxd pkg-config libcapstone-dev
```

Optional packages:
```bash
sudo apt install clang-format cppcheck valgrind
```

### On macOS with Homebrew
```bash
brew install capstone nasm
```

Note: On macOS, `xxd` is usually available as part of the `vim` package. If not, you can install it with:
```bash
brew install vim
```

### On Windows (WSL)
```bash
sudo apt update
sudo apt install build-essential nasm xxd pkg-config libcapstone-dev
```

## Build Process

### Default Build
To build the main executable with standard optimizations:
```bash
make
```

This will:
- Create the `bin/` directory if it doesn't exist
- Assemble the decoder stub (`decoder.asm`)
- Generate the header file (`decoder.h`) from the binary
- Compile all source files
- Link the final executable (`bin/byvalver`)

The build process follows this sequence:
1. **Decoder Generation**: `decoder.asm` is assembled to `decoder.bin` using NASM
2. **Header Generation**: `decoder.bin` is converted to C header `decoder.h` using xxd
3. **Source Compilation**: All C source files in `src/` are compiled to object files
4. **Linking**: Object files are linked together with Capstone library to create the final executable

### Build Variants

#### Debug Build
For development and debugging with debug symbols and sanitizers:
```bash
make debug
```

This includes:
- Debug symbols (`-g`)
- No optimizations (`-O0`)
- AddressSanitizer and UndefinedBehaviorSanitizer (`-fsanitize=address -fsanitize=undefined`)
- Debug mode defines (`-DDEBUG`)
- No optimizations to preserve variable information

#### Release Build
For optimized production builds:
```bash
make release
```

This includes:
- Maximum optimizations (`-O3`)
- Native architecture optimizations (`-march=native`)
- Release mode defines (`-DNDEBUG`)
- All optimizations enabled for performance

#### Static Build
To create a statically linked executable:
```bash
make static
```

This links all dependencies statically, creating a self-contained executable that does not require external libraries at runtime.

### Build ML Training Utility
To build the standalone ML model training utility:
```bash
make train
```

This creates `bin/train_model` which includes:
- All necessary object files except the main executable
- Training pipeline functionality
- ML strategist implementation
- Neural network training capabilities

The training utility can be run independently to train new ML models on custom datasets.

### Clean Build
To remove all generated files:
```bash
make clean
```

This removes:
- All object files in `bin/`
- Generated `decoder.bin` and `decoder.h`
- Preserves the source code and build configuration

To remove everything including the bin directory:
```bash
make clean-all
```

## Global Installation

### Install to System
After building, you can install byvalver globally:

```bash
# Install the binary to /usr/local/bin
sudo make install

# Install the man page to /usr/local/share/man/man1
sudo make install-man
```

### Uninstall
To remove the globally installed binary:

```bash
sudo make uninstall
```

## Build Configuration

### Makefile Structure
The build system is configured through the Makefile with the following detailed components:

#### Compiler and Flags
```makefile
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -O2
LDFLAGS = -lcapstone
```

- `-Wall -Wextra`: Enable all warnings to catch potential issues
- `-pedantic`: Strict adherence to C99 standard
- `-std=c99`: Use C99 standard for maximum portability
- `-O2`: Optimize for performance (changed to `-O0` for debug builds)

#### Source Management
The Makefile automatically includes all `.c` files in `src/` with specific exclusions:
- Obsolete files: `lib_api.c`, `fix_*.c`, `conservative_mov_original.c`
- Duplicate implementations: `arithmetic_substitution_strategies.c`
- Test-only code: `test_strategies.c`
- Training utility: `train_model.c` (excluded from main build)

The system filters these out to avoid linking conflicts and maintain build consistency.

#### Training Utility Build
The training utility target is specifically configured to:
- Include `train_model.c` as the main entry point
- Exclude `main.c` to avoid multiple main function conflicts
- Include all other source files for ML functionality
- Properly link with Capstone and math libraries

#### Obfuscation Modules
For the biphasic architecture, specific obfuscation modules are included:
- `obfuscation_strategy_registry.c`
- `obfuscation_strategies.c`

These are used during Pass 1 of the processing pipeline.

#### CLI Module
The new CLI functionality is included from:
- `cli.c`
- `cli.h`

These provide the enhanced command-line interface with proper argument parsing.

#### Dependency Generation
All source files that depend on the decoder stub automatically have `decoder.h` as a dependency, ensuring proper rebuild when the decoder changes.

### Build Information
To view current build configuration details:
```bash
make info
```

This displays:
- Compiler and flags being used
- Target executable path
- Number of source files being compiled
- Number of object files to be generated
- Strategy modules included
- Excluded files count

### Custom Build Parameters
The build system can be customized by setting environment variables:

```bash
# Use a different compiler
make CC=clang

# Add custom flags
make CFLAGS="-O2 -march=native -Wall -Werror"

# Set custom output directory
make BIN_DIR=custom_bin
```

## Windows Shellcode Strategy Analysis

### Analyzing Windows Shellcode for New Strategies

BYVALVER now includes 10 new Windows-specific denull strategies identified through analysis of real Windows shellcode patterns in the `shellcodes/` directory:

#### Strategy Discovery Process
- **Analysis**: Examined 100+ Windows shellcode files in `shellcodes/windows*` directories
- **Pattern Recognition**: Identified common null-byte elimination techniques used in real shellcode
- **Implementation**: Converted discovered patterns into automated transformation strategies
- **Integration**: Added strategies to the existing priority-based selection system

#### Key Windows Techniques Identified
1. **CALL/POP for Immediate Loading**: Using CALL/PUSH/POP sequences to load immediate values
2. **PEB Traversal**: Using Process Environment Block to find kernel32.dll dynamically
3. **SALC Usage**: Using SALC instruction for efficient AL register zeroing
4. **LEA for Arithmetic**: Using LEA for arithmetic operations to avoid immediate nulls
5. **Shift Operations**: Using bit shifts to build complex values from smaller parts
6. **Stack String Construction**: Building strings on stack with multiple PUSH operations
7. **String Instructions**: Using STOSB/STOSD for byte-level construction
8. **XCHG Operations**: Using register exchanges for value loading
9. **Complex Displacement**: Using LEA with complex addressing modes
10. **Byte-Level Operations**: Building 32-bit values from byte components

#### Build Integration
The new strategies are automatically integrated into the build process and registered in the strategy registry system. No additional build configuration is required.

## ML Training Workflow

### Building the Training Utility
The training utility is built separately from the main executable:

```bash
# Build the training utility
make train
```

### Training Process
1. Place shellcode files in the `./shellcodes/` directory (or customize the path in the training configuration)
2. Run the training utility:
   ```bash
   ./bin/train_model
   ```
3. The utility will process the shellcodes, train the neural network, and save the resulting model
4. The trained model will be saved to the configured path (typically `./ml_models/byvalver_ml_model.bin`)

### Model Integration
After training a new model:
1. The main `byvalver` executable will automatically use the trained model when the `--ml` option is enabled
2. The application uses dynamic path resolution to locate the model file relative to the executable location
3. If the model file is not found, the application falls back to default weights

## Troubleshooting Common Build Issues

### Missing Dependencies
If you encounter an error about missing libcapstone-dev:
```bash
# On Ubuntu/Debian
sudo apt install libcapstone-dev

# On macOS
brew install capstone

# Verify installation
pkg-config --exists capstone && echo "Found" || echo "Missing"
```

### NASM Issues
If you get NASM-related errors:
```bash
# Verify NASM installation
nasm --version

# On some systems, you might need nasm specifically
sudo apt install nasm  # Ubuntu/Debian
brew install nasm     # macOS

# Check that NASM is in your PATH
which nasm
```

### PATH_MAX and System Headers
If you encounter errors related to `PATH_MAX` or `readlink`:
- Ensure the `_GNU_SOURCE` macro is defined (it's included in the main files)
- Check that `limits.h` and `unistd.h` are properly included
- On some systems, you may need to install additional development packages

### ML Training Utility Build Issues
If the training utility build fails:
1. Ensure the main byvalver executable builds successfully first
2. Verify that all ML-related source files are present in the `src/` directory
3. Check that the training utility Make target correctly excludes `main.c` to avoid duplicate main function errors

### Makefile Integration Issues
The training utility Make target:
- Uses `$(filter-out $(SRC_DIR)/main.c, $(SRCS))` to exclude the main executable source
- Links with the same libraries as the main executable
- Creates a standalone binary with training capabilities
- Maintains compatibility with existing build system structure