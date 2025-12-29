#!/bin/bash

# Script to extract shellcode from various formats and convert to binary files

SHELLCODES_DIR="/home/mrnob0dy666/byvalver_PUBLIC/shellcodes"
OUTPUT_DIR="/home/mrnob0dy666/byvalver_PUBLIC/5050"

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# Function to convert hex string to binary
hex_to_bin() {
    local hex_string="$1"
    local output_file="$2"
    
    # Remove any whitespace and common prefixes
    hex_string=$(echo "$hex_string" | tr -d ' \t\n\r' | sed 's/0x//g' | sed 's/\\x//g' | sed 's/\\//g')
    
    # Convert hex to binary
    echo "$hex_string" | xxd -r -p > "$output_file"
}

# Function to extract shellcode from a file and convert to binary
extract_shellcode() {
    local input_file="$1"
    local output_file="$2"
    
    # Try multiple methods to extract shellcode
    
    # Method 1: Look for raw shellcode hex bytes in format \x##
    if grep -oE '\\x[0-9a-fA-F]{2}' "$input_file" | head -1 > /dev/null; then
        # Extract and convert hex-escaped bytes
        grep -oE '\\x[0-9a-fA-F]{2}' "$input_file" | tr -d '\n' | sed 's/\\x//g' | xxd -r -p > "$output_file"
        return 0
    fi
    
    # Method 2: Look for raw hex bytes without escapes
    if grep -oE '[0-9a-fA-F]{2}' "$input_file" | head -1 > /dev/null; then
        # Extract and convert raw hex bytes
        grep -oE '[0-9a-fA-F]{2}' "$input_file" | tr -d '\n' | xxd -r -p > "$output_file"
        return 0
    fi
    
    # Method 3: Look for shellcode in a char array format
    if grep -oE '"[^"]*"' "$input_file" | grep -E '\\x[0-9a-fA-F]{2}' | head -1 > /dev/null; then
        local shellcode_line=$(grep -oE '"[^"]*"' "$input_file" | grep -E '\\x[0-9a-fA-F]{2}' | head -1)
        echo "$shellcode_line" | sed 's/"//g' | sed 's/\\x//g' | xxd -r -p > "$output_file"
        return 0
    fi
    
    # Method 4: Look for shellcode in C-style format
    if grep -oE '\\\[[0-9a-fA-Fx]*\]' "$input_file" | head -1 > /dev/null; then
        grep -oE '\\\[[0-9a-fA-Fx]*\]' "$input_file" | sed 's/\\\[//g' | sed 's/\]//g' | sed 's/x//g' | tr -d '\n' | xxd -r -p > "$output_file"
        return 0
    fi
    
    # Method 5: Look for objdump output format
    if grep -oE '[0-9a-fA-F ]\{2,\}' "$input_file" | grep -E '^[0-9a-fA-F ]\{2,\}' | head -1 > /dev/null; then
        # Extract hex from objdump format (second column typically)
        grep -E '^[0-9a-fA-F]*.*[0-9a-fA-F ]\{2,\}' "$input_file" | awk '{for(i=2; i<=NF; i++) if(length($i)==2 || length($i)==4) print $i}' | tr -d '\n' | xxd -r -p > "$output_file"
        return 0
    fi
    
    # If no shellcode found, create empty file
    echo "No shellcode found in $input_file" > /dev/null
    touch "$output_file"
    return 1
}

# List of 50 challenging shellcodes to extract
shellcode_files=(
    "windows_x86/13507.txt"
    "windows/13560.txt"
    "windows_x86-64/35794.txt"
    "linux_x86/13333.txt"
    "linux_x86-64/38150.txt"
    "windows_x86/13699.txt"
    "windows_x86/15879.txt"
    "windows_x86/39754.txt"
    "windows_x86/43763.txt"
    "windows_x86/47980.txt"
    "linux_x86/13366.txt"
    "linux_x86/13416.txt"
    "linux_x86/13577.txt"
    "linux_x86/13661.txt"
    "linux_x86/37285.txt"
    "linux_x86/37289.txt"
    "linux_x86/37297.txt"
    "linux_x86/40026.txt"
    "linux_x86/41635.txt"
    "linux_x86/41757.txt"
    "linux_x86/43758.txt"
    "linux_x86/45538.txt"
    "linux_x86/46704.txt"
    "linux_x86/46801.txt"
    "linux_x86/46994.txt"
    "linux_x86/47108.txt"
    "linux_x86/47530.txt"
    "linux_x86/48243.txt"
    "linux_x86/51189.txt"
    "windows_x86/13524.txt"
    "windows_x86/13647.txt"
    "windows_x86/35793.txt"
    "windows_x86-64/48229.txt"
    "windows_x86-64/48252.txt"
    "arm/14097.txt"
    "arm/15314.asm"
    "arm/21252.asm"
    "arm/43497.asm"
    "osx/38065.txt"
    "macos/51177.txt"
    "macos/51178.txt"
    "bsd_x86/13262.txt"
    "bsd_x86/13263.txt"
    "solaris_x86/47784.txt"
    "linux_mips/27132.txt"
    "linux_sparc/41883.txt"
    "generator/13284.txt"
    "generator/46746.txt"
    "multiple/46800.txt"
    "hardware/46789.txt"
)

# Extract each shellcode
for i in "${!shellcode_files[@]}"; do
    input_path="$SHELLCODES_DIR/${shellcode_files[i]}"
    output_path="$OUTPUT_DIR/shellcode_$(printf "%02d" $((i+1))).bin"
    
    if [ -f "$input_path" ]; then
        echo "Extracting shellcode from: ${shellcode_files[i]}"
        extract_shellcode "$input_path" "$output_path"
        if [ -s "$output_path" ]; then
            echo "  -> Saved to: $output_path ($(stat -c%s "$output_path") bytes)"
        else
            echo "  -> Failed to extract shellcode from: ${shellcode_files[i]}"
            # Create an empty file to maintain count
            touch "$output_path"
        fi
    else
        echo "File not found: $input_path"
        # Create an empty file to maintain count
        touch "$output_path"
    fi
done

echo "Extraction complete. Files saved to: $OUTPUT_DIR"