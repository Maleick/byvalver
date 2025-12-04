# BYVALVER Build Guide

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

The system filters these out to avoid linking conflicts and maintain build consistency.

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

### xxd Utility Missing
If `xxd` is not found:
```bash
# Ubuntu/Debian
sudo apt install vim-common

# macOS (usually available with vim)
brew install vim

# Check availability
which xxd
```

### Capstone Installation Issues
If pkg-config can't find capstone:
```bash
# Check if capstone is properly installed
pkg-config --exists capstone && echo "Found" || echo "Not found"

# Check the library path
ldconfig -p | grep capstone

# On macOS, you might need to specify the library path explicitly:
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
```

## Verification

After building, verify the executable works:

```bash
# Test basic functionality
./bin/byvalver --version

# Test help system
./bin/byvalver --help

# Test with a sample file (if available)
./bin/byvalver --dry-run some_shellcode.bin
```

If installed globally:
```bash
byvalver --version
```

You can also check the manual page after installing:
```bash
man byvalver
```